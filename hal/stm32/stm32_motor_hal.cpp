#include "stm32_motor_hal.hpp"

Stm32MotorHal::Stm32MotorHal(uint16_t arr, uint16_t psc)
    : arr_(arr)
    , psc_(psc)
{}

void Stm32MotorHal::init()
{
    motor_.init(arr_, psc_);
}

void Stm32MotorHal::setMotorPWM(int pwm_left, int pwm_right)
{
    motor_.setPWM(pwm_left, pwm_right);
}

