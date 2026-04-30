#include "battery.hpp"

extern "C" {
    #include "stm32f10x.h"
    #include "stm32f10x_gpio.h"
    #include "stm32f10x_adc.h"
    #include "stm32f10x_rcc.h"
}

Battery::Battery(BatteryConfig config)
    : cfg_(config)
{}

void Battery::init()
{
    // ADC clock must not exceed 14Mhz - 72Mhz/6 = 12Mhz
    RCC_APB2PeriphClockCmd(cfg_.gpio_clk | cfg_.adc_clk, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    GPIO_InitTypeDef GPIO_InitStructure{};
    GPIO_InitStructure.GPIO_Pin  = cfg_.gpio_pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(cfg_.gpio_port, &GPIO_InitStructure);

    ADC_InitTypeDef ADC_InitStructure{};
    ADC_DeInit(cfg_.adc);
    ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode       = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel       = 1;
    ADC_Init(cfg_.adc, &ADC_InitStructure);

    ADC_Cmd(cfg_.adc, ENABLE);
    ADC_ResetCalibration(cfg_.adc);
    while(ADC_GetResetCalibrationStatus(cfg_.adc));
    ADC_StartCalibration(cfg_.adc);
    while(ADC_GetCalibrationStatus(cfg_.adc));
}

float Battery::read()
{
    uint16_t raw { getAverage(cfg_.adc_channel, 4) };
    float measured { static_cast<float>(raw) * (VREF / ADC_RESOLUTION) };
    return measured * DIVIDER_RATIO;
}

uint16_t Battery::getRaw(uint8_t ch)
{
    uint16_t timeout { 1000 };
    ADC_RegularChannelConfig(cfg_.adc, ch, 1, ADC_SampleTime_239Cycles5);
    ADC_SoftwareStartConvCmd(cfg_.adc, ENABLE);
    while(!ADC_GetFlagStatus(cfg_.adc, ADC_FLAG_EOC) && timeout--);
    return ADC_GetConversionValue(cfg_.adc);
}

uint16_t Battery::getAverage(uint8_t ch, uint8_t times)
{
    uint16_t total {};
    for (uint8_t i{0}; i < times; i++)
    {
        total += getRaw(ch);
    }
    return (times == 4) ? total >> 2 : total / times;
}

