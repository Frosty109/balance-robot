#pragma once

#include <cstdint>

extern "C" {
    #include "stm32f10x.h"
}

struct BatteryConfig {
    uint32_t      gpio_clk;
    GPIO_TypeDef* gpio_port;
    uint16_t      gpio_pin;
    uint32_t      adc_clk;
    ADC_TypeDef*  adc;
    uint8_t       adc_channel;
};

class Battery {
public:
    explicit Battery(BatteryConfig config);

    void  init();
    float read();

private:
    uint16_t getRaw(uint8_t ch);
    uint16_t getAverage(uint8_t ch, uint8_t times);

    BatteryConfig cfg_;

    static constexpr float VREF          { 3.3f   };
    static constexpr float DIVIDER_RATIO { 4.03f  };
    static constexpr int   ADC_RESOLUTION{ 4096   };
};
