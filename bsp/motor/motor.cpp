#include "motor.hpp"

extern "C" {
    #include "stm32f10x.h"
    #include "stm32f10x_gpio.h"
    #include "stm32f10x_tim.h"
    #include "stm32f10x_rcc.h"
}

// PWN Register shortcuts 
static inline void motor_set_pwm(uint16_t l_a, uint16_t l_b, uint16_t r_a, uint16_t r_b)
{
    TIM8->CCR1 = l_a;
    TIM8->CCR2 = l_b;
    TIM8->CCR3 = r_a;
    TIM8->CCR4 = r_b;
}

Motor::Motor() {}

void Motor::init(uint16_t arr, uint16_t psc)
{
    initGPIO();
    initPWM(arr, psc);
}

void Motor::initGPIO()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7 |
                                    GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void Motor::initPWM(uint16_t arr, uint16_t psc)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef       TIM_OCInitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
    TIM_DeInit(TIM8);

    TIM_TimeBaseStructure.TIM_Period        = arr - 1;
    TIM_TimeBaseStructure.TIM_Prescaler     = psc;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM8, &TIM_TimeBaseStructure);

    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse       = 0;
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;

    TIM_OC1Init(TIM8, &TIM_OCInitStructure);
    TIM_OC2Init(TIM8, &TIM_OCInitStructure);
    TIM_OC3Init(TIM8, &TIM_OCInitStructure);
    TIM_OC4Init(TIM8, &TIM_OCInitStructure);

    TIM_CtrlPWMOutputs(TIM8, ENABLE);
    TIM_OC1PreloadConfig(TIM8, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM8, TIM_OCPreload_Enable);
    TIM_OC3PreloadConfig(TIM8, TIM_OCPreload_Enable);
    TIM_OC4PreloadConfig(TIM8, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM8, ENABLE);
    TIM_Cmd(TIM8, ENABLE);
}

void Motor::setPWM(int pwm_left, int pwm_right)
{
    if(pwm_left == 0 && pwm_right == 0)
    {
        motor_set_pwm(0, 0, 0, 0);
        return;
    }

    uint16_t l_a = pwm_left  < 0 ? abs(pwm_left)  : 0;
    uint16_t l_b = pwm_left  > 0 ? abs(pwm_left)  : 0;
    uint16_t r_a = pwm_right > 0 ? abs(pwm_right) : 0;
    uint16_t r_b = pwm_right < 0 ? abs(pwm_right) : 0;

    motor_set_pwm(l_a, l_b, r_a, r_b);
}

bool Motor::isFaulted(float angle, float battery, bool stop_flag)
{
    if(angle < -MAX_ANGLE || angle > MAX_ANGLE ||
       battery < MIN_VOLTAGE || stop_flag)
    {
        motor_set_pwm(0, 0, 0, 0);
        return true;
    }
    return false;
}

int Motor::clamp(int value, int max, int min)
{
    if(value > max) return max;
    if(value < min) return min;
    return value;
}

int Motor::deadzone(int value)
{
    if(value > 0) return value + DEADZONE;
    if(value < 0) return value - DEADZONE;
    return 0;
}

int Motor::abs(int value)
{
    return value < 0 ? -value : value
}
