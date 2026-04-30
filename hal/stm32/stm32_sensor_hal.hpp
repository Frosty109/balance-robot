#pragma once

#include "../../hal/interface/i_sensor_hal.hpp"
#include "../../bsp/mpu6050/imu.hpp"
#include "../../bsp/encoder/encoder.hpp"
#include "../../bsp/battery/battery.hpp"

class Stm32SensorHal : public ISensorHal
{
public:
    Stm32SensorHal(Encoder encoder_left,
                   Encoder encoder_right,
                   Battery battery);

    void init();

    float getAngle()        override;
    float getGyroBalance()  override;
    float getGyroTurn()     override;
    float getAccelZ()       override;
    float getBattery()      override;
    int   getEncoderLeft()  override;
    int   getEncoderRight() override;

private:
    Imu     imu_;
    Encoder encoder_left_;
    Encoder encoder_right_;
    Battery battery_;
};