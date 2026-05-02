#include "physics.hpp"

Physics::Physics()
    : pitch_(0.0f), pitch_rate_(0.0f), velocity_(0.0f)
    , position_left_(0.0f), position_right_(0.0f)
    , pwm_left_(0), pwm_right_(0)
{}

void Physics::setMotorPWM(int pwm_left, int pwm_right)
{
    pwm_left_  = pwm_left;
    pwm_right_ = pwm_right;
}

void Physics::update(float dt)
{
    float force = (pwm_left_ + pwm_right_) * MOTOR_GAIN;

    float pitch_accel = (GRAVITY / PENDULUM_LENGTH) * pitch_ - force - DAMPING * pitch_rate_;

    pitch_rate_ += pitch_accel * dt;
    pitch_      += pitch_rate_ * dt;

    velocity_       = force * dt;
    position_left_  += velocity_ * ENCODER_SCALE;
    position_right_ += velocity_ * ENCODER_SCALE;
}

float Physics::getPitch()           const { return pitch_; }
float Physics::getPitchRate()       const { return pitch_rate_; }
float Physics::getVelocity()        const { return velocity_; }
int   Physics::getEncoderLeft()     const { return static_cast<int>(position_left_); }
int   Physics::getEncoderRight()    const { return static_cast<int>(position_right_); }


