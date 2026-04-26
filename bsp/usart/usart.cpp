#include <cstdio>

#include "usart.hpp"

extern "C" {
    #include "stm32f10x.h"
    #include "stm32f10x_gpio.h"
    #include "stm32f10x_usart.h"
    #include "stm32f10x_rcc.h"
    #include "misc.h"
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
    while ((USART1->SR & 0x40) == 0);
    USART1->DR = (uint8_t)ch;
    return ch;
}

extern "C" void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        uint8_t byte { (uint8_t)USART_ReceiveData(USART1) };
        USART_SendData(USART1, byte);
    }
}

