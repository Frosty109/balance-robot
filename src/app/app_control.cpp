#include "app_control.hpp"

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
    float angle     = sensor_.getAngle();
    float battery   = sensor_.getBattery();

    if (angle < -40.0f || angle > 40.0f || battery < 9.6f)
    {
        motor_.setMotorPWM(0, 0);
        velocity_.reset();
        return;
    }

    float gyro      = sensor_.getGyroBalance();
    float gyro_z    = sensor_.getGyroTurn();
    int   enc_l     = sensor_.getEncoderLeft();
    int   enc_r     = sensor_.getEncoderRight();

    int balance  = balance_.compute(angle, gyro);
    int velocity = velocity_.compute(enc_l, enc_r, move_x);
    int turn     = turn_.compute(gyro_z, move_z);

    motor_.setMotorPWM(balance + velocity - turn, balance + velocity + turn);

}