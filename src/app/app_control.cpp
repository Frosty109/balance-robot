#include "app_control.hpp"
#include <cstdio>

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
            // Motors first: the print busy-waits on the USART for ~3 ms, so reporting
            // before stopping would timestamp a shutdown that has not happened yet.
            // stale branch,
            disarm("stale");
            rejectPendingArm("stale");

            if (!stale_)
            {
                stale_ = true;
                printf("STALE last=%lu now=%lu\n", (unsigned long)last_fresh_ms_, (unsigned long)after);
            }
        }

        return;
    }

    last_fresh_ms_ = after;

    // Encoder needs to be drained on every fresh sample so gated samples discard counts
    const int enc_l = sensor_.getEncoderLeft();
    const int enc_r = sensor_.getEncoderRight();

    float angle     = sensor_.getAngle();
    float battery   = sensor_.getBattery();

    // || battery < 9.6f # Add this back in later

    static constexpr float FAULT_ANGLE {40.0f};
    static constexpr float CLEAR_ANGLE {10.0f};

    const float limit = faulted_ ? CLEAR_ANGLE : FAULT_ANGLE;

    if (angle < -limit || angle > limit)
    {
        disarm("angle");
        rejectPendingArm("angle");
        consecutive_fresh_ = 0;

        if (!faulted_)
        {
            faulted_ = true;
            printf("FAULT angle=%d\n", (int)(angle * 100));
        }
        return;
    }

    if (faulted_)
    {
        faulted_ = false;
        printf("RECOVERED angle=%d\n", (int)(angle * 100));
        rejectPendingArm("recovering");
    }

    if (stale_)
    {
        if (angle < -CLEAR_ANGLE || angle > CLEAR_ANGLE)
        {
            consecutive_fresh_ = 0;
            rejectPendingArm("recovering");
            return;
        }

        if (++consecutive_fresh_ < RECOVERY_FRESH_SAMPLES)
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

        if (angle < -CLEAR_ANGLE || angle > CLEAR_ANGLE)
        {
            printf("ARM REJECTED reason=angle angle=%d\n", (int)(angle * 100));
        }
        else
        {
            velocity_.reset();
            resetTelemetryWindow();
            armed_ = true;
            printf("ARMED angle=%d\n", (int)(angle * 100));
        }
    }

    if (!armed_)
    {
        motor_.setMotorPWM(0, 0);

        if (++telemetry_tick_ >= TELEMETRY_DECIMATION)
        {
            telemetry_tick_ = 0;
            printf("state=DISARMED angle=%d gz=%d battery=%d t=%lu poll=%lu\n",
                 (int)(angle * 100), 
                 (int)(sensor_.getYawRateDps() * 10),
                 (int)(battery * 100),
                 (unsigned long)clock_.nowMs(),
                 (unsigned long)max_poll_ms_);
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

    if (++telemetry_tick_ >= TELEMETRY_DECIMATION)
    {
        telemetry_tick_ = 0;
        // poll= is the worst-case poll duration since boot. It stays in the line
        // because the stale deadline is only meaningful while poll() returns well
        // inside STALE_TIMEOUT_MS; this is the only thing watching that.
        // enc_* are summed over the whole window: a single 5 ms delta per 100 ms
        // line is too sparse to read a sign from by hand.
        // i= is the clamped integral state. It cannot be derived from the other
        // fields: at the limits used early in the Stage 7 walk, the error in
        // estimating the P term is as large as the whole integral contribution.
        printf("angle=%d bal=%d i=%d L=%d R=%d enc_l=%d enc_r=%d battery=%d t=%lu poll=%lu\n",
             (int)(angle * 100), balance, (int)velocity_.integral(),
             left, right, enc_l_sum_, enc_r_sum_, int(battery * 100),
             (unsigned long)clock_.nowMs(),
             (unsigned long)max_poll_ms_);
        enc_l_sum_ = 0;
        enc_r_sum_ = 0;
    }
}
