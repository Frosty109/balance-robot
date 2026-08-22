#include "app_control.hpp"
#include <cstdio>

AppControl::AppControl(ISensorHal& sensor, IMotorHal& motor, BalancePD balance,
                        VelocityPI velocity, TurnPD turn)
    : sensor_(sensor), motor_(motor), balance_(balance)
    , velocity_(velocity), turn_(turn)
{}

void AppControl::reset()
{
    velocity_.reset();
}

void AppControl::update(float move_x, float move_z)
{
    if (!sensor_.poll())
    {
        return;
    }

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
        printf("angle=%d bal=%d L=%d R=%d battery=%d\n",
             (int)(angle * 100), balance, left, right, int(battery * 100));
    }

}
