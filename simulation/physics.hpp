#pragma once

class Physics
{
public:
    Physics();

    void update(float dt);
    void setMotorPWM(int pwm_left, int pwm_right);

    float getPitch()        const;
    float getPitchRate()    const;
    float getVelocity()     const;
    int   getEncoderLeft()  const;
    int   getEncoderRight() const;

private:
    float pitch_;
    float pitch_rate_;
    float velocity_;
    float position_left_;
    float position_right_;
    int   pwm_left_;
    int   pwm_right_;

    static constexpr float GRAVITY         { 9.81f };
    static constexpr float PENDULUM_LENGTH { 0.15f };
    static constexpr float MOTOR_GAIN      { 0.001f };
    static constexpr float DAMPING         { 0.1f };
    static constexpr float ENCODER_SCALE   { 10.0f };

};

