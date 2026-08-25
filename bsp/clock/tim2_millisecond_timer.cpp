#include "tim2_millisecond_timer.hpp"

extern "C" {
    #include "misc.h"
    #include "stm32f10x.h"
    #include "stm32f10x_rcc.h"
    #include "stm32f10x_tim.h"
}

namespace
{
    volatile std::uint32_t elapsed_ms {0};

    // APB1 runs at 36 MHz, but its timer clock doubles to 72 MHz.
    // 72 MHz / 72 / 1000 = 1 kHz, producing one interrupt per millisecond.
    constexpr std::uint16_t TIM2_PRESCALER {71};
    constexpr std::uint16_t TIM2_PERIOD {999};
}

void Tim2MillisecondTimer::init()
{
    elapsed_ms = 0;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_DeInit(TIM2);

    TIM_TimeBaseInitTypeDef timer {};
    timer.TIM_Prescaler     = TIM2_PRESCALER;
    timer.TIM_Period        = TIM2_PERIOD;
    timer.TIM_ClockDivision = TIM_CKD_DIV1;
    timer.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &timer);

    TIM_SetCounter(TIM2, 0);
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

    NVIC_InitTypeDef interrupt {};
    interrupt.NVIC_IRQChannel = TIM2_IRQn;
    interrupt.NVIC_IRQChannelPreemptionPriority = 0;
    interrupt.NVIC_IRQChannelSubPriority = 0;
    interrupt.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&interrupt);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}

std::uint32_t Tim2MillisecondTimer::nowMs() const
{
    return elapsed_ms;
}

extern "C" void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        ++elapsed_ms;
    }
}
