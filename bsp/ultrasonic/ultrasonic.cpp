#include "ultrasonic.hpp"

extern "C" {
    #include "delay.h"
    #include "misc.h"
    #include "stm32f10x.h"
    #include "stm32f10x_gpio.h"
    #include "stm32f10x_rcc.h"
    #include "stm32f10x_tim.h"
}

namespace
{
    Ultrasonic* active_sensor {nullptr};

    // APB1 timer clock is 72 MHz: divide by 72 for 1 us ticks.
    constexpr std::uint16_t TIMER_PRESCALER {71};
    constexpr std::uint16_t TIMER_PERIOD {0xFFFF};
}

void Ultrasonic::init()
{
    if (initialised_ || (active_sensor != nullptr && active_sensor != this))
    {
        return;
    }

    NVIC_DisableIRQ(TIM5_IRQn);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

    // Set the output latch low before making PA0 an output.
    GPIO_ResetBits(GPIOA, GPIO_Pin_0);

    GPIO_InitTypeDef gpio {};
    gpio.GPIO_Pin = GPIO_Pin_0;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_1;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    TIM_DeInit(TIM5);

    TIM_TimeBaseInitTypeDef timer {};
    timer.TIM_Prescaler = TIMER_PRESCALER;
    timer.TIM_Period = TIMER_PERIOD;
    timer.TIM_CounterMode = TIM_CounterMode_Up;
    timer.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM5, &timer);

    TIM_ICInitTypeDef capture {};
    capture.TIM_Channel = TIM_Channel_2;
    capture.TIM_ICPolarity = TIM_ICPolarity_Rising;
    capture.TIM_ICSelection = TIM_ICSelection_DirectTI;
    capture.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    capture.TIM_ICFilter = 0;
    TIM_ICInit(TIM5, &capture);

    // Channel 3 provides an internal deadline without driving PA2.
    TIM_OCInitTypeDef deadline {};
    TIM_OCStructInit(&deadline);
    deadline.TIM_OCMode = TIM_OCMode_Timing;
    deadline.TIM_OutputState = TIM_OutputState_Disable;
    deadline.TIM_Pulse = CAPTURE_TIMEOUT_US;
    TIM_OC3Init(TIM5, &deadline);

    TIM_CCxCmd(TIM5, TIM_Channel_2, TIM_CCx_Disable);
    TIM_ITConfig(TIM5, TIM_IT_CC2 | TIM_IT_CC3, DISABLE);
    TIM_SetCounter(TIM5, 0);
    TIM_ClearFlag(TIM5, TIM_FLAG_Update | TIM_FLAG_CC2 |
                        TIM_FLAG_CC3 | TIM_FLAG_CC2OF);
    NVIC_ClearPendingIRQ(TIM5_IRQn);

    active_sensor = this;

    NVIC_InitTypeDef interrupt {};
    interrupt.NVIC_IRQChannel = TIM5_IRQn;
    interrupt.NVIC_IRQChannelPreemptionPriority = 1;
    interrupt.NVIC_IRQChannelSubPriority = 0;
    interrupt.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&interrupt);

    initialised_ = true;
}

void Ultrasonic::finishCapture(EchoStatus status, std::uint32_t pulse_us)
{
    TIM_ITConfig(TIM5, TIM_IT_CC2 | TIM_IT_CC3, DISABLE);
    TIM_Cmd(TIM5, DISABLE);
    TIM_CCxCmd(TIM5, TIM_Channel_2, TIM_CCx_Disable);

    TIM_ClearFlag(TIM5, TIM_FLAG_Update | TIM_FLAG_CC2 |
                        TIM_FLAG_CC3 | TIM_FLAG_CC2OF);
    NVIC_ClearPendingIRQ(TIM5_IRQn);

    if (status == EchoStatus::captured)
    {
        completed_after_us_ = static_cast<std::uint32_t>(rise_ticks_) + pulse_us;
    }
    else if (status == EchoStatus::no_echo || status == EchoStatus::timeout)
    {
        completed_after_us_ = CAPTURE_TIMEOUT_US;
    }
    else
    {
        completed_after_us_ = 0;
    }

    pulse_us_ = pulse_us;
    status_ = status;

    // Publish completion only after the result fields are ready.
    state_ = State::complete;
}

bool Ultrasonic::start(std::uint32_t now_ms)
{
    if (!initialised_ || active_sensor != this || state_ != State::idle)
    {
        return false;
    }

    if (has_started_ && static_cast<std::uint32_t>(now_ms - last_started_ms_) < MIN_TRIGGER_INTERVAL_MS)
    {
        return false;
    }

    NVIC_DisableIRQ(TIM5_IRQn);
    TIM_ITConfig(TIM5, TIM_IT_CC2 | TIM_IT_CC3, DISABLE);
    TIM_Cmd(TIM5, DISABLE);
    TIM_CCxCmd(TIM5, TIM_Channel_2, TIM_CCx_Disable);
    GPIO_ResetBits(GPIOA, GPIO_Pin_0);

    // Restore rising-edge capture and restart the prescaler phase.
    TIM5->CCER = TIM5->CCER & ~TIM_CCER_CC2P;
    TIM_GenerateEvent(TIM5, TIM_EventSource_Update);
    TIM_SetCounter(TIM5, 0);
    TIM_ClearFlag(TIM5, TIM_FLAG_Update | TIM_FLAG_CC2 |
                        TIM_FLAG_CC3 | TIM_FLAG_CC2OF);
    NVIC_ClearPendingIRQ(TIM5_IRQn);

    rise_ticks_ = 0;
    pulse_us_ = 0;
    completed_after_us_ = 0;
    status_ = EchoStatus::capture_error;
    started_ms_ = now_ms;
    last_started_ms_ = now_ms;
    has_started_ = true;

    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) != Bit_RESET)
    {
        finishCapture(EchoStatus::capture_error, 0);
        NVIC_EnableIRQ(TIM5_IRQn);
        return true;
    }

    state_ = State::waiting_rise;
    TIM_CCxCmd(TIM5, TIM_Channel_2, TIM_CCx_Enable);
    TIM_ITConfig(TIM5, TIM_IT_CC2 | TIM_IT_CC3, ENABLE);
    TIM_Cmd(TIM5, ENABLE);
    NVIC_EnableIRQ(TIM5_IRQn);

    GPIO_SetBits(GPIOA, GPIO_Pin_0);
    delay_us(15);
    GPIO_ResetBits(GPIOA, GPIO_Pin_0);
    return true;
}

void Ultrasonic::handleInterrupt()
{
    const State state = state_;
    const auto flags = TIM5->SR;

    if (state != State::waiting_rise && state != State::waiting_fall)
    {
        TIM_ClearFlag(TIM5, TIM_FLAG_Update | TIM_FLAG_CC2 |
                            TIM_FLAG_CC3 | TIM_FLAG_CC2OF);
        return;
    }

    if ((flags & (TIM_FLAG_CC2OF | TIM_FLAG_Update)) != 0)
    {
        finishCapture(EchoStatus::capture_error, 0);
        return;
    }

    if ((flags & TIM_FLAG_CC2) != 0)
    {
        // Freeze CCR2 before reading it; reading also clears CC2IF.
        TIM_CCxCmd(TIM5, TIM_Channel_2, TIM_CCx_Disable);
        const std::uint16_t ticks = TIM_GetCapture2(TIM5);
        const auto capture_flags = TIM5->SR;

        if ((capture_flags & (TIM_FLAG_CC2OF | TIM_FLAG_Update)) != 0)
        {
            finishCapture(EchoStatus::capture_error, 0);
            return;
        }

        if (ticks >= CAPTURE_TIMEOUT_US)
        {
            finishCapture(state == State::waiting_rise
                              ? EchoStatus::no_echo : EchoStatus::timeout, 0);
            return;
        }

        if (state == State::waiting_fall)
        {
            const std::uint16_t rise = rise_ticks_;
            if (ticks <= rise)
            {
                finishCapture(EchoStatus::capture_error, 0);
                return;
            }

            finishCapture(EchoStatus::captured,
                          static_cast<std::uint32_t>(ticks - rise));
            return;
        }

        rise_ticks_ = ticks;
        state_ = State::waiting_fall;

        if ((capture_flags & TIM_FLAG_CC3) != 0)
        {
            finishCapture(EchoStatus::timeout, 0);
            return;
        }

        // CC2P selects falling edges while channel 2 is an input.
        TIM5->CCER = TIM5->CCER | TIM_CCER_CC2P;
        TIM_CCxCmd(TIM5, TIM_Channel_2, TIM_CCx_Enable);

        // A low input without a captured fall means the edge was missed.
        if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == Bit_RESET &&
            TIM_GetFlagStatus(TIM5, TIM_FLAG_CC2) == RESET)
        {
            finishCapture(EchoStatus::capture_error, 0);
            return;
        }
    }

    if (TIM_GetFlagStatus(TIM5, TIM_FLAG_CC3) != RESET)
    {
        // Let the next IRQ inspect any edge that arrived during this handler.
        if (TIM_GetFlagStatus(TIM5, TIM_FLAG_CC2) != RESET)
        {
            return;
        }

        finishCapture(state_ == State::waiting_rise
                          ? EchoStatus::no_echo : EchoStatus::timeout, 0);
    }
}

bool Ultrasonic::poll(std::uint32_t now_ms, EchoCapture& result)
{
    if (!initialised_ || active_sensor != this)
    {
        return false;
    }

    NVIC_DisableIRQ(TIM5_IRQn);

    const bool waiting = state_ == State::waiting_rise ||
                         state_ == State::waiting_fall;

    // The extra millisecond avoids expiring early through clock quantisation.
    if (waiting &&
        static_cast<std::uint32_t>(now_ms - started_ms_) >
            CAPTURE_TIMEOUT_US / 1000)
    {
        // Preserve a valid captured edge even if its IRQ was delayed.
        handleInterrupt();

        if (state_ == State::waiting_rise || state_ == State::waiting_fall)
        {
            finishCapture(EchoStatus::capture_error, 0);
        }
    }

    const bool ready = state_ == State::complete;
    if (ready)
    {
        result.pulse_us = pulse_us_;
        result.completed_after_us = completed_after_us_;
        result.status = status_;
        state_ = State::idle;
    }

    NVIC_EnableIRQ(TIM5_IRQn);
    return ready;
}

extern "C" void TIM5_IRQHandler(void)
{
    if (active_sensor != nullptr)
    {
        active_sensor->handleInterrupt();
    }
}