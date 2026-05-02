#pragma once

#include "../../hal/interface/i_motor_hal.hpp"
#include "../../bsp/motor/motor.hpp"

class Stm32MotorHal : public IMotorHal
{
public:
    Stm32MotorHal(uint16_t arr, uint16_t psc);

    void init();

    void setMotorPWM(int pwm_left, int pwm_right) override;

private:
    Motor    motor_;
    uint16_t arr_;
    uint16_t psc_;
};