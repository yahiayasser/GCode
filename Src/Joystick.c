/*
 * Joystick.c
 *
 *  Created on: Mar 26, 2021
 *      Author: Yahia
 */

#include "Joystick.h"
#include "Joystick_CFG.h"

static ADC_HandleTypeDef hadc = {0};
static ADC_ChannelConfTypeDef sConfig = {0};
static uint8_t calibrated = 0;

void JoyStick_Init(JOYSTICK* Joystick_PTR)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	if(Joystick_PTR -> JoyStick_xGPIO == GPIOA || Joystick_PTR -> JoyStick_yGPIO == GPIOA)
	{
		__HAL_RCC_GPIOA_CLK_ENABLE();
	}
	else if(Joystick_PTR -> JoyStick_xGPIO == GPIOB || Joystick_PTR -> JoyStick_yGPIO == GPIOB)
	{
		__HAL_RCC_GPIOB_CLK_ENABLE();
	}
	else if(Joystick_PTR -> JoyStick_xGPIO == GPIOC || Joystick_PTR -> JoyStick_yGPIO == GPIOC)
	{
		__HAL_RCC_GPIOC_CLK_ENABLE();
	}
	else if (Joystick_PTR -> JoyStick_xGPIO == GPIOD || Joystick_PTR -> JoyStick_yGPIO == GPIOD)
	{
		__HAL_RCC_GPIOD_CLK_ENABLE();
	}
	else if (Joystick_PTR -> JoyStick_xGPIO == GPIOE || Joystick_PTR -> JoyStick_yGPIO == GPIOE)
	{
		__HAL_RCC_GPIOD_CLK_ENABLE();
	}

	GPIO_InitStruct.Pin = Joystick_PTR -> JoyStick_xPIN;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(Joystick_PTR -> JoyStick_xGPIO, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = Joystick_PTR -> JoyStick_yPIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(Joystick_PTR -> JoyStick_yGPIO, &GPIO_InitStruct);

	hadc.Instance = Joystick_PTR -> ADC_Instance;
	hadc.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc.Init.ContinuousConvMode = DISABLE;
	hadc.Init.DiscontinuousConvMode = DISABLE;
	hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc.Init.NbrOfConversion = 1;
	HAL_ADC_Init(&hadc);
    sConfig.Channel = Joystick_PTR -> ADCx_CH;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;
	HAL_ADC_ConfigChannel(&hadc, &sConfig);

	if(calibrated == 0)
	{
		HAL_ADCEx_Calibration_Start(&hadc);
		calibrated = 1;
	}
}


void JoyStick_Read(JOYSTICK* Joystick_PTR, uint16_t* JoyStick_XY)
{
	// Select The JoyStick Instance ADC Channel For X
	sConfig.Channel = Joystick_PTR -> ADCx_CH;
    HAL_ADC_ConfigChannel(&hadc, &sConfig);
	// Start ADC Conversion
	HAL_ADC_Start(&hadc);
	// Poll ADC1 Peripheral & TimeOut = 1mSec
	HAL_ADC_PollForConversion(&hadc, 1);
	// Read The ADC Conversion Result Write It To JoyStick X
	Joystick_PTR -> Joystick_xyValues[0] = HAL_ADC_GetValue(&hadc);
	JoyStick_XY[0] = Joystick_PTR -> Joystick_xyValues[0];

	// Select The JoyStick Instance ADC Channel For Y
	sConfig.Channel = Joystick_PTR -> ADCy_CH;
	HAL_ADC_ConfigChannel(&hadc, &sConfig);
	// Start ADC Conversion
	HAL_ADC_Start(&hadc);
	// Poll ADC1 Peripheral & TimeOut = 1mSec
	HAL_ADC_PollForConversion(&hadc, 1);
	// Read The ADC Conversion Result Write It To JoyStick Y
	Joystick_PTR -> Joystick_xyValues[1] = HAL_ADC_GetValue(&hadc);
	JoyStick_XY[1] = Joystick_PTR -> Joystick_xyValues[1];
}

