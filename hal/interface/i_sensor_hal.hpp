#pragma once

class ISensorHal
{
public:
    virtual ~ISensorHal() = default;

    virtual float getAngle() = 0;
    virtual float getGyroBalance() = 0;
    virtual float getGyroTurn() = 0;
    virtual float getAccelZ() = 0;
    virtual float getBattery() = 0;
    virtual int getEncoderLeft() = 0;
    virtual int getEncoderRight() = 0;
};