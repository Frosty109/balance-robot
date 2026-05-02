#pragma once

#include "../../hal/interface/i_motor_hal.hpp"
#include "../../simulation/physics.hpp"

class SimMotorHal : public IMotorHal
{
public:
    explicit SimMotorHal(Physics& physics);

    void setMotorPWM(int pwm_left, int pwm_right) override;

private:
    Physics& physics_;
};

