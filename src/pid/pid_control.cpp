#include "pid_control.hpp"

// BalancePD
BalancePD::BalancePD(float kp, float kd, float mid_angle)
    : kp_(kp), kd_(kd), mid_angle_(mid_angle)
{
}

int BalancePD::compute(float angle, float gyro)
{
    float angle_bias = mid_angle_ - angle;
    float gyro_bias = 0 - gyro;
    return -kp_ / 100.0f * angle_bias - gyro_bias * kd_ / 100.0f;
}