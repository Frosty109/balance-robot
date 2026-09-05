#include "app_control.hpp"
#include <cstdio>

AppControl::AppControl(ISensorHal& sensor,
                       IMotorHal& motor,
                       IMonotonicClock& clock,
                       BalancePD balance,
                       VelocityPI velocity,
                       TurnPD turn)
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

void AppControl::update(float move_x, float move_z)
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
            motor_.setMotorPWM(0, 0);
            velocity_.reset();

            if (!stale_)
            {
                stale_ = true;
                printf("STALE last=%lu now=%lu\n", (unsigned long)last_fresh_ms_, (unsigned long)after);
            }
        }

        return;
    }

    last_fresh_ms_ = after;

    float angle     = sensor_.getAngle();
    float battery   = sensor_.getBattery();

    // || battery < 9.6f # Add this back in later

    static constexpr float FAULT_ANGLE {40.0f};
    static constexpr float CLEAR_ANGLE {10.0f};

    const float limit = faulted_ ? CLEAR_ANGLE : FAULT_ANGLE;

    if (angle < -limit || angle > limit)
    {
        motor_.setMotorPWM(0, 0);
        velocity_.reset();

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
    }

    if (stale_)
    {
        if (angle < -CLEAR_ANGLE || angle > CLEAR_ANGLE)
        {
            consecutive_fresh_ = 0;
            return;
        }

        if (++consecutive_fresh_ < RECOVERY_FRESH_SAMPLES)
        {
            return;
        }

        stale_ = false;
        consecutive_fresh_ = 0;
    }

    float gyro      = sensor_.getGyroBalance();
    float gyro_z    = sensor_.getGyroTurn();
    int   enc_l     = sensor_.getEncoderLeft();
    int   enc_r     = sensor_.getEncoderRight();

    int balance  = balance_.compute(angle, gyro);
    int velocity = velocity_.compute(enc_l, enc_r, move_x);
    int turn     = turn_.compute(gyro_z, move_z);

    int left = balance + velocity - turn;
    int right = balance + velocity + turn;
    motor_.setMotorPWM(left, right);

    if (++telemetry_tick_ >= TELEMETRY_DECIMATION)
    {
        telemetry_tick_ = 0;
        printf("angle=%d bal=%d L=%d R=%d battery=%d t=%lu poll=%lu\n",
             (int)(angle * 100), balance, left, right, int(battery * 100),
             (unsigned long)clock_.nowMs(),
             (unsigned long)max_poll_ms_);
    }

}
