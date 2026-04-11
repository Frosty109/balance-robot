#pragma once

extern "C" {
    #include "stm32f10x.h"
}

class Encoder 
{
public:
    Encoder(TIM_TypeDef*    tim,
            GPIO_TypeDef*   gpio,
            uint16_t        pin_a,
            uint16_t        pin_b,
            uint32_t        rcc_tim,
            uint32_t        rcc_gpio);

    void init();
    int read();

private:
    static constexpr uint16_t PERIOD = 65535;

    TIM_TypeDef*  tim_;
    GPIO_TypeDef* gpio_;
    uint16_t      pin_a_;
    uint16_t      pin_b_;
    uint32_t      rcc_tim_;
    uint32_t      rcc_gpio_;
};