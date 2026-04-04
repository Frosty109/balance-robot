#pragma

class IMotorHal
{
public:
    virtual ~IMotorHal() = default;

    virtual void setMotorPWM(int pwm_left, int pwm_right) = 0;
};