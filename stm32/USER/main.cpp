#include <cstdio>
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
#include "../../hal/stm32/stm32_monotonic_clock.hpp"
#include "../../src/app/app_control.hpp"
#include "../../src/app/motion_link.hpp"

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

    Stm32MonotonicClock clock;
    clock.init();

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

    AppControl app(sensor_hal,
                   motor_hal,
                   clock,
                   BalancePD(10200.0f, 78.0f, 0.0f),
                   VelocityPI(7000.0f, 35.0f, 400.0f),
                   YawRateP(1.0f, 30.0f, 100));

    app.requestDisarm();
    printf("DISARMED reason=boot\n");
    usart.enableOperatorCommands();

    // Stage 10 starting limits (plan §2): ±100 mm/s, ±30 deg/s, 200 mm/s², 60 deg/s², 250 ms timeout.
    MotionGovernor governor({ 100.0f, 30.0f, 200.0f, 60.0f, 250 });
    FrameParser    parser;
    bool           link_was_alive     = false;
    uint32_t       last_accepted_ms   = 0;
    uint32_t       frame_errors       = 0;
    uint32_t       printed_errors     = 0;
    uint32_t       last_err_print_ms  = 0;   

    while (true)
    {
        const uint32_t now = clock.nowMs();

        if (usart.takeOverflow())
        {
            if (app.armed()) { printf("LINK DISARM cause=overflow byte=0x00\n"); }
            app.requestDisarm();
        }

        uint16_t entry;
        while (usart.readByte(entry))
        {
            const LinkEvent ev = parser.feed(static_cast<uint8_t>(entry & 0xFF),
                                             (entry & RxRing::ERROR_BIT) != 0);
            switch (ev.kind)
            {
                case LinkEvent::Kind::Arm:
                    app.requestArm();
                    break;
                case LinkEvent::Kind::Disarm:
                    if (app.armed())
                    {
                        printf("LINK DISARM cause=%s byte=0x%02X\n", causeName(ev.cause), ev.byte);
                    }
                    app.requestDisarm();
                    break;
                case LinkEvent::Kind::Motion:
                    if (governor.onCommand(ev.seq, ev.target, now)) { last_accepted_ms = now; }
                    break;
                case LinkEvent::Kind::FrameError:
                    ++frame_errors;
                    break;
                case LinkEvent::Kind::None:
                    break;
            }
        }

        const MotionTarget target = governor.step(app.armed(), now);

        // Read after step(): a disarmed step resets the governor, so frames while disarmed never flap LINK UP.
        const bool link_alive = governor.linkAlive(now);
        if (link_alive && !link_was_alive) { printf("LINK UP\n"); }
        if (!link_alive && link_was_alive && app.armed())
        {
            printf("LINK LOST age=%lu\n", (unsigned long)(now - last_accepted_ms));
        }
        link_was_alive = link_alive;

        if (frame_errors != printed_errors && now - last_err_print_ms >= 1000)
        {
            printf("FRAME ERR n=%lu\n", (unsigned long)frame_errors);
            printed_errors    = frame_errors;
            last_err_print_ms = now;
        }

        app.update(toMoveX(target.forward_mm_s), target.yaw_rate_dps);
    }
}
