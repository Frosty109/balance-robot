#include "sim_sensor_hal.hpp"

SimSensorHal::SimSensorHal(Physics& physics)
    : physics_(physics)
{}

float SimSensorHal::getAngle()       { return physics_.getPitch();       }
float SimSensorHal::getGyroBalance() { return physics_.getPitchRate();   }
float SimSensorHal::getGyroTurn()    { return 0.0f;                      }
float SimSensorHal::getAccelZ()      { return 0.0f;                      }
float SimSensorHal::getBattery()     { return 12.0f;                     }
int   SimSensorHal::getEncoderLeft() { return physics_.getEncoderLeft(); }
int   SimSensorHal::getEncoderRight(){ return physics_.getEncoderRight();}
