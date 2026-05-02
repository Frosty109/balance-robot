#include "sim_motor_hal.hpp"

SimMotorHal::SimMotorHal(Physics& physics)
    : physics_(physics)
{}

void SimMotorHal::setMotorPWM(int pwm_left, int pwm_right)
{
    physics_.setMotorPWM(pwm_left, pwm_right);
}