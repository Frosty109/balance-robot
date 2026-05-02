#pragma once

#include "../../hal/interface/i_sensor_hal.hpp"
#include "../../simulation/physics.hpp"

class SimSensorHal : public ISensorHal
{
public:
    explicit SimSensorHal(Physics& physics);

    float getAngle()        override;
    float getGyroBalance()  override;
    float getGyroTurn()     override;
    float getAccelZ()       override;
    float getBattery()      override;
    int   getEncoderLeft()  override;
    int   getEncoderRight() override;

private:
    Physics& physics_;
};