#pragma once

#include "../../hal/interface/i_sensor_hal.hpp"
#include "../../hal/interface/i_motor_hal.hpp"
#include "../pid/pid_control.hpp"

class AppControl
{
public:
    AppControl(ISensorHal& sensor, IMotorHal& motor, BalancePD balance,
                VelocityPI velocity, TurnPD turn);

    void update();
    void reset();

private:
    ISensorHal& sensor_;
    IMotorHal&  motor_;
    BalancePD   balance_;
    VelocityPI  velocity_;
    TurnPD      turn_;
};