#pragma once

#include <cstdint>

extern "C" {
    #include "stm32f10x.h"
}

struct UsartConfig {
    USART_TypeDef* usart;
    uint32_t       rcc_usart;
    uint32_t       rcc_gpio;
    GPIO_TypeDef*  gpio_port;
    uint16_t       tx_pin;
    uint16_t       rx_pin;
    uint8_t        nvic_channel;
};
 
class Usart
{
public:
    explicit Usart(UsartConfig config);

    void init(uint32_t baud);
    void sendByte(uint8_t ch);
    void sendBytes(uint8_t* data, uint16_t length);

private:
    UsartConfig cfg_;
};
