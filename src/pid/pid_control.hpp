#pragma once

class BalancePD
{
    public:
        BalancePD(float kp, float kd, float mid_angle);
        int compute(float angle, float gyro);

    private:
    float kp_, kd_, mid_angle_;
};

class VelocityPI
{
    public:
        VelocityPI(float kp, float ki, float integral_limit);
        int compute(int encoder_left, int encoder_right, float move_x = 0.0f);
        void reset();

    private:
        float kp_, ki_, integral_limit_;
        float encoder_bias_, encoder_integral_;
};

class TurnPD
{
    public: 
        TurnPD(float kp, float kd);
        int compute(float gyro_z, float move_z = 0.0f);

    private:
        float kp_, kd_;
}