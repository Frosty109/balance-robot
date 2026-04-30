#include "stm32_sensor_hal.hpp"

Stm32SensorHal::Stm32SensorHal(Encoder encoder_left,
                                Encoder encoder_right,
                                Battery battery)
    : encoder_left_(encoder_left)
    , encoder_right_(encoder_right)
    , battery_(battery)
{}

void Stm32SensorHal::init()
{
    imu_.init();
    encoder_left_.init();
    encoder_right_.init();
    battery_.init();
}

float Stm32SensorHal::getAngle()       { return imu_.getPitch();    }
float Stm32SensorHal::getGyroBalance() { return imu_.getGyroX();    }
float Stm32SensorHal::getGyroTurn()    { return imu_.getGyroZ();    }
float Stm32SensorHal::getAccelZ()      { return imu_.getAccelZ();   }
float Stm32SensorHal::getBattery()     { return battery_.read();    }
int   Stm32SensorHal::getEncoderLeft() { return encoder_left_.read();  }
int   Stm32SensorHal::getEncoderRight(){ return encoder_right_.read(); }
