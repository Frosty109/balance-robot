#include "pid_control.hpp"
#include <algorithm>

// BalancePD
BalancePD::BalancePD(float kp, float kd, float mid_angle)
    : kp_(kp), kd_(kd), mid_angle_(mid_angle)
{
}

int BalancePD::compute(float angle, float gyro)
{
    float angle_bias = mid_angle_ - angle;
    float gyro_bias = 0 - gyro;
    return static_cast<int>(-kp_ / 100.0f * angle_bias - gyro_bias * kd_ / 100.0f);
}

// Velocity PI 
VelocityPI::VelocityPI(float kp, float ki, float integral_limit)
    : kp_(kp), ki_(ki), integral_limit_(integral_limit),
      encoder_bias_(0.0f), encoder_integral_(0.0f)
{
}

int VelocityPI::compute(int encoder_left, int encoder_right, float move_x)
{
    float encoder_least = 0 - (encoder_left + encoder_right);
    encoder_bias_ *= 0.84f;
    encoder_bias_ += encoder_least * 0.16f;
    encoder_integral_ += encoder_bias_;
    encoder_integral_ += move_x;

    if (encoder_integral_ > integral_limit_)
    {
        encoder_integral_ = integral_limit_;
    }
    if (encoder_integral_ < -integral_limit_)
    {
        encoder_integral_ = -integral_limit_;
    }

    return -encoder_bias_ * kp_ / 100.0f - encoder_integral_ * ki_ / 100.0f;
}

void VelocityPI::reset()
{
    encoder_integral_ = 0.0f;
    encoder_bias_ = 0.0f;
}

YawRateP::YawRateP(float kp_pwm_per_dps, float max_target_dps, int max_effort)
    : kp_pwm_per_dps_(kp_pwm_per_dps), max_target_dps_(max_target_dps), max_effort_(max_effort)
{   
}

int YawRateP::compute(float target_yaw_rate_dps, float measured_yaw_rate_dps) const
{
    const float target = std::clamp(target_yaw_rate_dps, -max_target_dps_, max_target_dps_);
    const float error = target - measured_yaw_rate_dps;

    const int requested = static_cast<int>(kp_pwm_per_dps_ * error);
    return std::clamp(requested, -max_effort_, max_effort_);
}

