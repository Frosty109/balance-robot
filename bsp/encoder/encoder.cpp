#include "encoder.hpp"

extern "C" {
    #include "stm32f10x.h"
    #include "stm32f10x_gpio.h"
    #include "stm32f10x_tim.h"
    #include "stm32f10x_rcc.h"
}

Encoder::Encoder(TIM_TypeDef*  tim,
                 GPIO_TypeDef* gpio,
                 uint16_t      pin_a,
                 uint16_t      pin_b,
                 uint32_t      rcc_tim,
                 uint32_t      rcc_gpio)
    : time_(tim)
    , gpio_(gpio)
    , pin_a_(pin_a)
    , pin_b_(pin_b)
    , rcc_tim_(rcc_tim)
    , rcc_gpio_(rcc_gpio)
{}

Encoder::init()
{
    
}