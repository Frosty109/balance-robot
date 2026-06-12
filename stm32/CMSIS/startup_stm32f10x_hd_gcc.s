/**
 * startup_stm32f10x_hd_gcc.s - GCC startup file for STM32F10x High Density
 *
 * Equivalent to startup_stm32f10x_hd.s (Keil MDK version) but written in
 * GNU assembler syntax for use with arm-none-eabi-gcc.
 *
 * Same vector table order as the original Keil file.
 *
 * Based on ST Microelectronics standard peripheral library GCC startup template.
 * Copyright (C) 2011 STMicroelectronics. BSD-style license.
 */

  .syntax unified
  .cpu cortex-m3
  .fpu softvfp
  .thumb

  .global g_pfnVectors
  .global Default_Handler

/* Symbols defined by the linker script */
  .word _sidata   /* start of .data in flash  */
  .word _sdata    /* start of .data in RAM    */
  .word _edata    /* end   of .data in RAM    */
  .word _sbss     /* start of .bss  in RAM    */
  .word _ebss     /* end   of .bss  in RAM    */

/**
 * Reset_Handler - entry point after reset
 *
 * 1. Copy .data from flash to RAM
 * 2. Zero-fill .bss
 * 3. Call SystemInit (clock setup)
 * 4. Call main()
 */
  .section .text.Reset_Handler
  .weak Reset_Handler
  .type Reset_Handler, %function
Reset_Handler:
  ldr   sp, =_estack          /* set stack pointer to top of RAM */

  /* Copy initialised data from flash (_sidata) to RAM (_sdata.._edata) */
  movs  r1, #0
  b     LoopCopyDataInit

CopyDataInit:
  ldr   r3, =_sidata
  ldr   r3, [r3, r1]
  str   r3, [r0, r1]
  adds  r1, r1, #4

LoopCopyDataInit:
  ldr   r0, =_sdata
  ldr   r3, =_edata
  adds  r2, r0, r1
  cmp   r2, r3
  bcc   CopyDataInit

  /* Zero-fill .bss (_sbss.._ebss) */
  ldr   r2, =_sbss
  b     LoopFillZerobss

FillZerobss:
  movs  r3, #0
  str   r3, [r2], #4

LoopFillZerobss:
  ldr   r3, =_ebss
  cmp   r2, r3
  bcc   FillZerobss

  bl    SystemInit             /* configure clocks (system_stm32f10x.c) */
  bl    main                   /* call application entry point          */
  bx    lr
  .size Reset_Handler, .-Reset_Handler

/**
 * Default_Handler - catches any unimplemented IRQ (infinite loop)
 * All peripheral IRQ handlers are weak aliases pointing here.
 */
  .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b     Infinite_Loop
  .size Default_Handler, .-Default_Handler

/**
 * Vector table — must match the order in startup_stm32f10x_hd.s exactly.
 * Placed in .isr_vector section which the linker script puts first in flash.
 */
  .section .isr_vector,"a",%progbits
  .type g_pfnVectors, %object
  .size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
  /* Cortex-M3 core vectors */
  .word  _estack                     /* top of stack                          */
  .word  Reset_Handler               /* reset                                 */
  .word  NMI_Handler                 /* NMI                                   */
  .word  HardFault_Handler           /* hard fault                            */
  .word  MemManage_Handler           /* MPU fault                             */
  .word  BusFault_Handler            /* bus fault                             */
  .word  UsageFault_Handler          /* usage fault                           */
  .word  0                           /* reserved                              */
  .word  0                           /* reserved                              */
  .word  0                           /* reserved                              */
  .word  0                           /* reserved                              */
  .word  SVC_Handler                 /* SVCall                                */
  .word  DebugMon_Handler            /* debug monitor                         */
  .word  0                           /* reserved                              */
  .word  PendSV_Handler              /* PendSV                                */
  .word  SysTick_Handler             /* SysTick                               */
  /* STM32F10x HD external interrupts (same order as Keil file) */
  .word  WWDG_IRQHandler             /* window watchdog                       */
  .word  PVD_IRQHandler              /* PVD through EXTI line detect          */
  .word  TAMPER_IRQHandler           /* tamper                                */
  .word  RTC_IRQHandler              /* RTC                                   */
  .word  FLASH_IRQHandler            /* flash                                 */
  .word  RCC_IRQHandler              /* RCC                                   */
  .word  EXTI0_IRQHandler            /* EXTI line 0                           */
  .word  EXTI1_IRQHandler            /* EXTI line 1                           */
  .word  EXTI2_IRQHandler            /* EXTI line 2                           */
  .word  EXTI3_IRQHandler            /* EXTI line 3                           */
  .word  EXTI4_IRQHandler            /* EXTI line 4                           */
  .word  DMA1_Channel1_IRQHandler    /* DMA1 channel 1                        */
  .word  DMA1_Channel2_IRQHandler    /* DMA1 channel 2                        */
  .word  DMA1_Channel3_IRQHandler    /* DMA1 channel 3                        */
  .word  DMA1_Channel4_IRQHandler    /* DMA1 channel 4                        */
  .word  DMA1_Channel5_IRQHandler    /* DMA1 channel 5                        */
  .word  DMA1_Channel6_IRQHandler    /* DMA1 channel 6                        */
  .word  DMA1_Channel7_IRQHandler    /* DMA1 channel 7                        */
  .word  ADC1_2_IRQHandler           /* ADC1 & ADC2                           */
  .word  USB_HP_CAN1_TX_IRQHandler   /* USB high priority or CAN1 TX          */
  .word  USB_LP_CAN1_RX0_IRQHandler  /* USB low priority or CAN1 RX0          */
  .word  CAN1_RX1_IRQHandler         /* CAN1 RX1                              */
  .word  CAN1_SCE_IRQHandler         /* CAN1 SCE                              */
  .word  EXTI9_5_IRQHandler          /* EXTI lines 9..5                       */
  .word  TIM1_BRK_IRQHandler         /* TIM1 break                            */
  .word  TIM1_UP_IRQHandler          /* TIM1 update                           */
  .word  TIM1_TRG_COM_IRQHandler     /* TIM1 trigger and commutation          */
  .word  TIM1_CC_IRQHandler          /* TIM1 capture compare                  */
  .word  TIM2_IRQHandler             /* TIM2                                  */
  .word  TIM3_IRQHandler             /* TIM3                                  */
  .word  TIM4_IRQHandler             /* TIM4                                  */
  .word  I2C1_EV_IRQHandler          /* I2C1 event                            */
  .word  I2C1_ER_IRQHandler          /* I2C1 error                            */
  .word  I2C2_EV_IRQHandler          /* I2C2 event                            */
  .word  I2C2_ER_IRQHandler          /* I2C2 error                            */
  .word  SPI1_IRQHandler             /* SPI1                                  */
  .word  SPI2_IRQHandler             /* SPI2                                  */
  .word  USART1_IRQHandler           /* USART1                                */
  .word  USART2_IRQHandler           /* USART2                                */
  .word  USART3_IRQHandler           /* USART3                                */
  .word  EXTI15_10_IRQHandler        /* EXTI lines 15..10                     */
  .word  RTCAlarm_IRQHandler         /* RTC alarm through EXTI line           */
  .word  USBWakeUp_IRQHandler        /* USB wakeup from suspend               */
  .word  TIM8_BRK_IRQHandler         /* TIM8 break                            */
  .word  TIM8_UP_IRQHandler          /* TIM8 update                           */
  .word  TIM8_TRG_COM_IRQHandler     /* TIM8 trigger and commutation          */
  .word  TIM8_CC_IRQHandler          /* TIM8 capture compare                  */
  .word  ADC3_IRQHandler             /* ADC3                                  */
  .word  FSMC_IRQHandler             /* FSMC                                  */
  .word  SDIO_IRQHandler             /* SDIO                                  */
  .word  TIM5_IRQHandler             /* TIM5                                  */
  .word  SPI3_IRQHandler             /* SPI3                                  */
  .word  UART4_IRQHandler            /* UART4                                 */
  .word  UART5_IRQHandler            /* UART5                                 */
  .word  TIM6_IRQHandler             /* TIM6                                  */
  .word  TIM7_IRQHandler             /* TIM7                                  */
  .word  DMA2_Channel1_IRQHandler    /* DMA2 channel 1                        */
  .word  DMA2_Channel2_IRQHandler    /* DMA2 channel 2                        */
  .word  DMA2_Channel3_IRQHandler    /* DMA2 channel 3                        */
  .word  DMA2_Channel4_5_IRQHandler  /* DMA2 channel 4 & 5                   */

/* Weak aliases — unimplemented handlers fall through to Default_Handler */
  .weak NMI_Handler
  .thumb_set NMI_Handler,Default_Handler

  .weak HardFault_Handler
  .thumb_set HardFault_Handler,Default_Handler

  .weak MemManage_Handler
  .thumb_set MemManage_Handler,Default_Handler

  .weak BusFault_Handler
  .thumb_set BusFault_Handler,Default_Handler

  .weak UsageFault_Handler
  .thumb_set UsageFault_Handler,Default_Handler

  .weak SVC_Handler
  .thumb_set SVC_Handler,Default_Handler

  .weak DebugMon_Handler
  .thumb_set DebugMon_Handler,Default_Handler

  .weak PendSV_Handler
  .thumb_set PendSV_Handler,Default_Handler

  .weak SysTick_Handler
  .thumb_set SysTick_Handler,Default_Handler

  .weak WWDG_IRQHandler
  .thumb_set WWDG_IRQHandler,Default_Handler

  .weak PVD_IRQHandler
  .thumb_set PVD_IRQHandler,Default_Handler

  .weak TAMPER_IRQHandler
  .thumb_set TAMPER_IRQHandler,Default_Handler

  .weak RTC_IRQHandler
  .thumb_set RTC_IRQHandler,Default_Handler

  .weak FLASH_IRQHandler
  .thumb_set FLASH_IRQHandler,Default_Handler

  .weak RCC_IRQHandler
  .thumb_set RCC_IRQHandler,Default_Handler

  .weak EXTI0_IRQHandler
  .thumb_set EXTI0_IRQHandler,Default_Handler

  .weak EXTI1_IRQHandler
  .thumb_set EXTI1_IRQHandler,Default_Handler

  .weak EXTI2_IRQHandler
  .thumb_set EXTI2_IRQHandler,Default_Handler

  .weak EXTI3_IRQHandler
  .thumb_set EXTI3_IRQHandler,Default_Handler

  .weak EXTI4_IRQHandler
  .thumb_set EXTI4_IRQHandler,Default_Handler

  .weak DMA1_Channel1_IRQHandler
  .thumb_set DMA1_Channel1_IRQHandler,Default_Handler

  .weak DMA1_Channel2_IRQHandler
  .thumb_set DMA1_Channel2_IRQHandler,Default_Handler

  .weak DMA1_Channel3_IRQHandler
  .thumb_set DMA1_Channel3_IRQHandler,Default_Handler

  .weak DMA1_Channel4_IRQHandler
  .thumb_set DMA1_Channel4_IRQHandler,Default_Handler

  .weak DMA1_Channel5_IRQHandler
  .thumb_set DMA1_Channel5_IRQHandler,Default_Handler

  .weak DMA1_Channel6_IRQHandler
  .thumb_set DMA1_Channel6_IRQHandler,Default_Handler

  .weak DMA1_Channel7_IRQHandler
  .thumb_set DMA1_Channel7_IRQHandler,Default_Handler

  .weak ADC1_2_IRQHandler
  .thumb_set ADC1_2_IRQHandler,Default_Handler

  .weak USB_HP_CAN1_TX_IRQHandler
  .thumb_set USB_HP_CAN1_TX_IRQHandler,Default_Handler

  .weak USB_LP_CAN1_RX0_IRQHandler
  .thumb_set USB_LP_CAN1_RX0_IRQHandler,Default_Handler

  .weak CAN1_RX1_IRQHandler
  .thumb_set CAN1_RX1_IRQHandler,Default_Handler

  .weak CAN1_SCE_IRQHandler
  .thumb_set CAN1_SCE_IRQHandler,Default_Handler

  .weak EXTI9_5_IRQHandler
  .thumb_set EXTI9_5_IRQHandler,Default_Handler

  .weak TIM1_BRK_IRQHandler
  .thumb_set TIM1_BRK_IRQHandler,Default_Handler

  .weak TIM1_UP_IRQHandler
  .thumb_set TIM1_UP_IRQHandler,Default_Handler

  .weak TIM1_TRG_COM_IRQHandler
  .thumb_set TIM1_TRG_COM_IRQHandler,Default_Handler

  .weak TIM1_CC_IRQHandler
  .thumb_set TIM1_CC_IRQHandler,Default_Handler

  .weak TIM2_IRQHandler
  .thumb_set TIM2_IRQHandler,Default_Handler

  .weak TIM3_IRQHandler
  .thumb_set TIM3_IRQHandler,Default_Handler

  .weak TIM4_IRQHandler
  .thumb_set TIM4_IRQHandler,Default_Handler

  .weak I2C1_EV_IRQHandler
  .thumb_set I2C1_EV_IRQHandler,Default_Handler

  .weak I2C1_ER_IRQHandler
  .thumb_set I2C1_ER_IRQHandler,Default_Handler

  .weak I2C2_EV_IRQHandler
  .thumb_set I2C2_EV_IRQHandler,Default_Handler

  .weak I2C2_ER_IRQHandler
  .thumb_set I2C2_ER_IRQHandler,Default_Handler

  .weak SPI1_IRQHandler
  .thumb_set SPI1_IRQHandler,Default_Handler

  .weak SPI2_IRQHandler
  .thumb_set SPI2_IRQHandler,Default_Handler

  .weak USART1_IRQHandler
  .thumb_set USART1_IRQHandler,Default_Handler

  .weak USART2_IRQHandler
  .thumb_set USART2_IRQHandler,Default_Handler

  .weak USART3_IRQHandler
  .thumb_set USART3_IRQHandler,Default_Handler

  .weak EXTI15_10_IRQHandler
  .thumb_set EXTI15_10_IRQHandler,Default_Handler

  .weak RTCAlarm_IRQHandler
  .thumb_set RTCAlarm_IRQHandler,Default_Handler

  .weak USBWakeUp_IRQHandler
  .thumb_set USBWakeUp_IRQHandler,Default_Handler

  .weak TIM8_BRK_IRQHandler
  .thumb_set TIM8_BRK_IRQHandler,Default_Handler

  .weak TIM8_UP_IRQHandler
  .thumb_set TIM8_UP_IRQHandler,Default_Handler

  .weak TIM8_TRG_COM_IRQHandler
  .thumb_set TIM8_TRG_COM_IRQHandler,Default_Handler

  .weak TIM8_CC_IRQHandler
  .thumb_set TIM8_CC_IRQHandler,Default_Handler

  .weak ADC3_IRQHandler
  .thumb_set ADC3_IRQHandler,Default_Handler

  .weak FSMC_IRQHandler
  .thumb_set FSMC_IRQHandler,Default_Handler

  .weak SDIO_IRQHandler
  .thumb_set SDIO_IRQHandler,Default_Handler

  .weak TIM5_IRQHandler
  .thumb_set TIM5_IRQHandler,Default_Handler

  .weak SPI3_IRQHandler
  .thumb_set SPI3_IRQHandler,Default_Handler

  .weak UART4_IRQHandler
  .thumb_set UART4_IRQHandler,Default_Handler

  .weak UART5_IRQHandler
  .thumb_set UART5_IRQHandler,Default_Handler

  .weak TIM6_IRQHandler
  .thumb_set TIM6_IRQHandler,Default_Handler

  .weak TIM7_IRQHandler
  .thumb_set TIM7_IRQHandler,Default_Handler

  .weak DMA2_Channel1_IRQHandler
  .thumb_set DMA2_Channel1_IRQHandler,Default_Handler

  .weak DMA2_Channel2_IRQHandler
  .thumb_set DMA2_Channel2_IRQHandler,Default_Handler

  .weak DMA2_Channel3_IRQHandler
  .thumb_set DMA2_Channel3_IRQHandler,Default_Handler

  .weak DMA2_Channel4_5_IRQHandler
  .thumb_set DMA2_Channel4_5_IRQHandler,Default_Handler
