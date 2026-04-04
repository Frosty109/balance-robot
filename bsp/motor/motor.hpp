# pragma once

# include <cstdint>

class Motor
{
public: 
    Motor();

    void init(uint16_t arr, uint16_t psc);

    void setPWM(int pwm_left, int pwm_right);

    bool isFaulted(float angle, float battery, bool stop_flag);

    static int clamp(int value, int max, int min);
    static int deadzone(int value);

private: 
    static constexpr int DEADZONE = 1300;
    static constexpr float MAX_ANGLE = 40.0f;
    static constexpr float MIN_VOLTAGE = 9.6f;

    void initGPIO();
    void initPWM(uint16_t arr, uint16_t psc);

    static int abs(int value);
};

