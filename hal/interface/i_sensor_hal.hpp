#pragma once

class ISensorHal
{
public:
    virtual ~ISensorHal() = default;

    virtual float getAngle() = 0;
    virtual float getGyroBalance() = 0;
    virtual float getYawRateDps() = 0; // positive = left/CCW viewed from above
    virtual float getAccelZ() = 0;
    virtual float getBattery() = 0;
    virtual int getEncoderLeft() = 0;
    virtual int getEncoderRight() = 0;
    virtual bool poll() = 0;
};