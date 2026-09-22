#include <cstdio>

#include "usart.hpp"

#include "../../src/app/operator_command.hpp"

extern "C" {
    #include "stm32f10x.h"
    #include "stm32f10x_gpio.h"
    #include "stm32f10x_usart.h"
    #include "stm32f10x_rcc.h"
    #include "misc.h"
}

namespace {
    volatile bool rx_arm_;
    volatile bool rx_disarm_;

    constexpr uint16_t RECEIVE_ERRORS = USART_FLAG_ORE | USART_FLAG_FE |
                                        USART_FLAG_NE | USART_FLAG_PE;

    // __get/__set_PRIMASK live in core_cm3.c, which is excluded from the build.
    inline uint32_t primaskSave()
    {
        uint32_t primask;
        __ASM volatile ("MRS %0, primask" : "=r" (primask));
        __disable_irq();
        return primask;
    }

    inline void primaskRestore(uint32_t primask)
    {
        __ASM volatile ("MSR primask, %0" : : "r" (primask));
    }
}

Usart::Usart(UsartConfig config)
    : cfg_(config)
{}

void Usart::init(uint32_t baud) {
    RCC_APB2PeriphClockCmd(cfg_.rcc_usart | cfg_.rcc_gpio, ENABLE);


    GPIO_InitTypeDef GPIO_InitStructure {};
	USART_InitTypeDef USART_InitStructure {};
	NVIC_InitTypeDef NVIC_InitStructure {}; 

    // TX
    GPIO_InitStructure.GPIO_Pin = cfg_.tx_pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(cfg_.gpio_port, &GPIO_InitStructure);

    // RX
    GPIO_InitStructure.GPIO_Pin = cfg_.rx_pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(cfg_.gpio_port, &GPIO_InitStructure);

    // USART Config
    USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(cfg_.usart, &USART_InitStructure);
    
    USART_ITConfig(cfg_.usart, USART_IT_RXNE, DISABLE);
    USART_Cmd(cfg_.usart, ENABLE);    

    NVIC_InitStructure.NVIC_IRQChannel = cfg_.nvic_channel;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void Usart::sendByte(uint8_t ch)
{
    while(USART_GetFlagStatus(cfg_.usart, USART_FLAG_TXE) == RESET);
    USART_SendData(cfg_.usart, ch);
}

void Usart::sendBytes(uint8_t* data, uint16_t length)
{
    while (length--)
        sendByte(*data++);
}

extern "C" int fputc(int ch, FILE* f)
{
    while ((USART1->SR & 0x80) == 0);
    USART1->DR = (uint8_t)ch;
    return ch;
}

// newlib (GCC) calls _write for all stdout; Keil calls fputc above
// __GNUC__ too, exclude explicitly, GCC-only path.
#if defined(__GNUC__) && !defined(__ARMCC_VERSION)
extern "C" int _write(int fd, char* buf, int len)
{
    (void)fd;
    for (int i = 0; i < len; i++) 
    {
        while ((USART1->SR & 0x80) == 0);
        USART1->DR = (uint8_t)buf[i];
    }
    return len;
}
#endif

void Usart::enableOperatorCommands()
{
    // Purge bytes received during initialization so none can arm on the first loop.
    const uint32_t primask = primaskSave();

    rx_arm_    = false;
    rx_disarm_ = false;

    if ((cfg_.usart->SR & USART_FLAG_RXNE) != 0)
    {
        (void)cfg_.usart->DR;
    }
    NVIC_ClearPendingIRQ(static_cast<IRQn_Type>(cfg_.nvic_channel));
    USART_ITConfig(cfg_.usart, USART_IT_RXNE, ENABLE);

    primaskRestore(primask);
}

OperatorCommands Usart::takeCommands()
{
    const uint32_t primask = primaskSave();

    const OperatorCommands cmd { rx_arm_, rx_disarm_ };

    rx_arm_    = false;
    rx_disarm_ = false;

    primaskRestore(primask);
    return cmd;
}

extern "C" void USART1_IRQHandler(void)
{
    // F103 clears ORE/FE/NE/PE as part of the status-then-data read, so SR must be captured before draining DR.
    const uint16_t sr   { USART1->SR };
    const uint8_t  byte { (uint8_t)USART_ReceiveData(USART1) };

    const OperatorCommand cmd = decodeOperatorByte(byte,
                                                   (sr & RECEIVE_ERRORS) != 0);
    if (cmd == OperatorCommand::Arm)    { rx_arm_    = true; }
    if (cmd == OperatorCommand::Disarm) { rx_disarm_ = true; }
}

