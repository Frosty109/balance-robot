#pragma once

class BalancePD
{
    public:
        BalancePD(float kp, float kd, float mid_angle);
        int compute(float angle, float gyro);

    private:
        float kp_;
        float kd_;
        float mid_angle_;
};

class VelocityPI
{
    public:
        VelocityPI(float kp, float ki, float integral_limit);
        int compute(int encoder_left, int encoder_right, float move_x = 0.0f);
        void reset();
        float integral() const { return encoder_integral_; }

    private:
        float kp_;
        float ki_;
        float integral_limit_;
        float encoder_bias_;
        float encoder_integral_;
};

class YawRateP
{
    public: 
        YawRateP(float kp_pwm_per_dps, float max_target_dps, 
                 int max_effort, float ff_pwm_per_dps = 0.0f);

        int compute(float target_yaw_rate_dps, 
                    float measured_yaw_rate_dps) const;

    private:
        float kp_pwm_per_dps_;
        float max_target_dps_;
        int max_effort_;
        float ff_pwm_per_dps_;
};