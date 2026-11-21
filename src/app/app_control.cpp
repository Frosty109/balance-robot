#include "app_control.hpp"
#include "motion_link.hpp"
#include <cmath>
#include <cstdio>

namespace
{
    // Telemetry prints fixed-point integers: hundredths for angles, tenths for rates.
    int hundredths(float value) { return (int)(value * 100); }
    int tenths(float value) { return (int)(value * 10); }

    bool beyond(float angle, float limit)
    {
        return angle < -limit || angle > limit;
    }
}

AppControl::AppControl(ISensorHal& sensor,
                       IMotorHal& motor,
                       IMonotonicClock& clock,
                       BalancePD balance,
                       VelocityPI velocity,
                       YawRateP turn)
    : sensor_(sensor),
      motor_(motor),
      clock_(clock),
      balance_(balance),
      velocity_(velocity),
      turn_(turn),
      last_fresh_ms_(clock.nowMs())
{}

void AppControl::reset()
{
    velocity_.reset();
}

void AppControl::requestArm()
{
    if (!armed_)
    {
        arm_requested_ = true;
    }
}

void AppControl::requestDisarm()
{
    arm_requested_ = false;
    disarm("operator");
}

void AppControl::disarm(const char* reason)
{
    motor_.setMotorPWM(0, 0);
    velocity_.reset();

    if (armed_)
    {
        armed_ = false;
        resetTelemetryWindow();
        printf("DISARMED reason=%s\n", reason);
    }
}

void AppControl::rejectPendingArm(const char* reason)
{
    if (arm_requested_)
    {
        arm_requested_ = false;
        printf("ARM REJECTED reason=%s\n", reason);
    }
}

void AppControl::resetTelemetryWindow()
{
    telemetry_tick_ = 0;
    enc_l_sum_ = 0;
    enc_r_sum_ = 0;
}

void AppControl::update(float move_x, float target_yaw_rate_dps)
{
    const std::uint32_t before = clock_.nowMs();
    const bool fresh = sensor_.poll();
    const std::uint32_t after = clock_.nowMs();

    const std::uint32_t poll_ms = after - before;
    if (poll_ms > max_poll_ms_) { max_poll_ms_ = poll_ms; }

    if (!fresh)
    {
        consecutive_fresh_ = 0;

        if (after - last_fresh_ms_ >= STALE_TIMEOUT_MS)
        {
            // Motors first: the print busy-waits ~3 ms, so printing first would
            // timestamp a shutdown that has not happened yet.
            disarm("stale");
            rejectPendingArm("stale");

            if (!stale_)
            {
                stale_ = true;
                printf("STALE last=%lu now=%lu\n",
                       (unsigned long)last_fresh_ms_,
                       (unsigned long)after);
            }
        }

        return;
    }

    if (have_fresh_sample_)
    {
        const std::uint32_t interval_ms = after - last_fresh_ms_;
        if (interval_ms > max_fresh_interval_ms_)
        {
            max_fresh_interval_ms_ = interval_ms;
        }
    }
    have_fresh_sample_ = true;
    last_fresh_ms_ = after;

    // Encoder needs to be drained on every fresh sample so gated samples discard counts
    const int enc_l = sensor_.getEncoderLeft();
    const int enc_r = sensor_.getEncoderRight();

    float angle     = sensor_.getAngle();
    float battery   = sensor_.getBattery();

    // || battery < 9.6f # Add this back in later

    static constexpr float FAULT_ANGLE {40.0f};
    static constexpr float CLEAR_ANGLE {10.0f};

    // Hysteresis: a fault trips past 40 degrees and clears only inside 10.
    const float limit = faulted_ ? CLEAR_ANGLE : FAULT_ANGLE;

    if (beyond(angle, limit))
    {
        disarm("angle");
        rejectPendingArm("angle");
        consecutive_fresh_ = 0;

        if (!faulted_)
        {
            faulted_ = true;
            printf("FAULT angle=%d\n", hundredths(angle));
        }
        return;
    }

    if (faulted_)
    {
        faulted_ = false;
        printf("RECOVERED angle=%d\n", hundredths(angle));
        rejectPendingArm("recovering");
    }

    if (stale_)
    {
        if (beyond(angle, CLEAR_ANGLE))
        {
            consecutive_fresh_ = 0;
            rejectPendingArm("recovering");
            return;
        }

        ++consecutive_fresh_;
        if (consecutive_fresh_ < RECOVERY_FRESH_SAMPLES)
        {
            rejectPendingArm("recovering");
            return;
        }

        stale_ = false;
        consecutive_fresh_ = 0;
    }

    if (arm_requested_)
    {
        arm_requested_ = false;

        if (beyond(angle, CLEAR_ANGLE))
        {
            printf("ARM REJECTED reason=angle angle=%d\n", hundredths(angle));
        }
        else
        {
            velocity_.reset();
            resetTelemetryWindow();
            armed_ = true;
            printf("ARMED angle=%d\n", hundredths(angle));
        }
    }

    if (!armed_)
    {
        motor_.setMotorPWM(0, 0);

        ++telemetry_tick_;
        if (telemetry_tick_ >= TELEMETRY_DECIMATION)
        {
            telemetry_tick_ = 0;
            printf("state=DISARMED angle=%d gz=%d battery=%d t=%lu poll=%lu dtmax=%lu\n",
                   hundredths(angle),
                   tenths(sensor_.getYawRateDps()),
                   hundredths(battery),
                   (unsigned long)clock_.nowMs(),
                   (unsigned long)max_poll_ms_,
                   (unsigned long)max_fresh_interval_ms_);
        }
        return;
    }

    float gyro      = sensor_.getGyroBalance();
    float yaw_rate_dps    = sensor_.getYawRateDps();

    enc_l_sum_ += enc_l;
    enc_r_sum_ += enc_r;

    int balance  = balance_.compute(angle, gyro);
    int velocity = velocity_.compute(enc_l, enc_r, move_x);
    int turn     = turn_.compute(target_yaw_rate_dps, yaw_rate_dps);

    int left = balance + velocity - turn;
    int right = balance + velocity + turn;
    motor_.setMotorPWM(left, right);

    ++telemetry_tick_;
    if (telemetry_tick_ >= TELEMETRY_DECIMATION)
    {
        telemetry_tick_ = 0;
        const long forward_mm_s = std::lround(move_x / MOVE_X_PER_MM_S);

        // poll= is the worst poll since boot: the stale deadline needs it well under STALE_TIMEOUT_MS.
        // enc_* sum the whole window: one 5 ms delta per 100 ms line is too sparse to read a sign from.
        // i= is the clamped integral; at early Stage 7 limits it cannot be estimated from the other fields.
        printf("angle=%d bal=%d i=%d tr=%d vx=%ld wz=%d gz=%d L=%d R=%d enc_l=%d enc_r=%d t=%lu poll=%lu\n",
               hundredths(angle),
               balance,
               (int)velocity_.integral(),
               turn,
               forward_mm_s,
               tenths(target_yaw_rate_dps),
               tenths(yaw_rate_dps),
               left,
               right,
               enc_l_sum_,
               enc_r_sum_,
               (unsigned long)clock_.nowMs(),
               (unsigned long)max_poll_ms_);
        enc_l_sum_ = 0;
        enc_r_sum_ = 0;
    }
}
