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
    : tim_(tim)
    , gpio_(gpio)
    , pin_a_(pin_a)
    , pin_b_(pin_b)
    , rcc_tim_(rcc_tim)
    , rcc_gpio_(rcc_gpio)
{}

void Encoder::init()
{   
    RCC_APB1PeriphClockCmd(rcc_tim_, ENABLE);
    RCC_APB2PeriphClockCmd(rcc_gpio_, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = pin_a_ | pin_b_;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(gpio_, &GPIO_InitStructure);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure{};
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_Period = PERIOD;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(tim_, &TIM_TimeBaseStructure);

    TIM_EncoderInterfaceConfig(tim_, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);

    TIM_ICInitTypeDef TIM_ICInitStructure{};
    TIM_ICInitStructure.TIM_ICFilter = 10;
    TIM_ICInit(tim_, &TIM_ICInitStructure);

    TIM_ClearFlag(tim_, TIM_FLAG_Update);
    TIM_ITConfig(tim_, TIM_IT_Update, ENABLE);
    TIM_SetCounter(tim_, 0);
    TIM_Cmd(tim_, ENABLE);

}

int Encoder::read() 
{
    int count { static_cast<short>(tim_->CNT)};
    tim_->CNT = 0;
    return count;
}