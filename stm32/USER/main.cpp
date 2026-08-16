#include <cstdio>

extern "C" {
    #include "stm32f10x.h"
    #include "sys.h"
    #include "delay.h"
}

#include "../../bsp/encoder/encoder.hpp"
#include "../../bsp/battery/battery.hpp"
#include "../../bsp/usart/usart.hpp"
#include "../../hal/stm32/stm32_sensor_hal.hpp"
#include "../../hal/stm32/stm32_motor_hal.hpp"
#include "../../src/app/app_control.hpp"

int main()
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    delay_init();
    JTAG_Set(JTAG_SWD_DISABLE);
    JTAG_Set(SWD_ENABLE);

    Encoder enc_left (TIM3, GPIOA, GPIO_Pin_6, GPIO_Pin_7,
                      RCC_APB1Periph_TIM3, RCC_APB2Periph_GPIOA);
    Encoder enc_right(TIM4, GPIOB, GPIO_Pin_6, GPIO_Pin_7,
                      RCC_APB1Periph_TIM4, RCC_APB2Periph_GPIOB);

    BatteryConfig bat_cfg { RCC_APB2Periph_GPIOC, GPIOC, GPIO_Pin_4,
                            RCC_APB2Periph_ADC1, ADC1, ADC_Channel_14 };
    Battery battery(bat_cfg);

    UsartConfig uart_cfg { USART1,
                           RCC_APB2Periph_USART1, RCC_APB2Periph_GPIOA,
                           GPIOA, GPIO_Pin_9, GPIO_Pin_10, USART1_IRQn };
    Usart usart(uart_cfg);
    usart.init(115200);
    setvbuf(stdout, NULL, _IONBF, 0); 

    Stm32SensorHal sensor_hal(enc_left, enc_right, battery);
    constexpr uint16_t PWM_ARR = 2880;  // 72MHz / 2880 = 25kHz
    constexpr uint16_t PWM_PSC = 0;
    Stm32MotorHal  motor_hal(PWM_ARR, PWM_PSC);

    printf("BOOT\n");

    if (!sensor_hal.init())
    {
        printf("IMU INIT FAILED\n");

        while (true)
        {
            delay_ms(1000);
        }
    }

    printf("IMU OK\n");

    motor_hal.init();
    printf("MOTOR OK\n");

    AppControl app(sensor_hal, motor_hal,
                BalancePD(2000.0f, 78.0f, 0.0f),
                VelocityPI(0.0f, 0.0f, 200.0f),
                TurnPD(0.0f, 0.0f));

    while (true)
    {
        app.update();
        // delay_ms(5);
    }
}

    
