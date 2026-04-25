#pragma once

#include <cstdint>

class Battery
{
public:
    Battery();

    void init();
    float read();

private:
    static uint16_t getRaw(uint8_t ch);                     // Gets single ADC reading
    static uint16_t getAverage(uint8_t ch, uint8_t times);  // averaged reading 


    static constexpr float VREF { 3.3f };
    static constexpr float DIVIDER_RATIO { 4.03f };
    static constexpr int ADC_RESOLUTION { 4096 };
};