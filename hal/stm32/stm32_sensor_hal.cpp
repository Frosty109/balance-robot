#include "stm32_sensor_hal.hpp"

Stm32SensorHal::Stm32SensorHal(Encoder encoder_left,
                                Encoder encoder_right,
                                Battery battery)
    : encoder_left_(encoder_left)
    , encoder_right_(encoder_right)
    , battery_(battery)
{}

bool Stm32SensorHal::init()
{
    bool imu_ok = imu_.init();
    encoder_left_.init();
    encoder_right_.init();
    battery_.init();
    return imu_ok;
}

float Stm32SensorHal::getAngle()        { return imu_.getPitch();    }
float Stm32SensorHal::getGyroBalance()  { return imu_.getGyroX();    }
float Stm32SensorHal::getGyroTurn()     { return imu_.getGyroZ();    }
float Stm32SensorHal::getAccelZ()       { return imu_.getAccelZ();   }
float Stm32SensorHal::getBattery()      { return battery_.read();    }
int   Stm32SensorHal::getEncoderLeft()  { return encoder_left_.read();  }
int   Stm32SensorHal::getEncoderRight() { return encoder_right_.read(); }
void  Stm32SensorHal::poll()            { imu_.read(); }
