/*
 * Joystick.h
 *
 *  Created on: Mar 26, 2021
 *      Author: Yahia
 */

#ifndef INC_JOYSTICK_H_
#define INC_JOYSTICK_H_

#define HAL_ADC_MODULE_ENABLED

#include "stm32f1xx_hal.h"
#include "Platform_Types.h"

struct Joystick_Container
{
	GPIO_TypeDef* 	JoyStick_xGPIO;
	GPIO_TypeDef* 	JoyStick_yGPIO;
	uint16      	JoyStick_xPIN;
	uint16      	JoyStick_yPIN;
	ADC_TypeDef*   	ADC_Instance;
	uint32     	 	ADCx_CH;
	uint32     	 	ADCy_CH;
	uint16		   	Joystick_xyValues[2];
};

typedef struct Joystick_Container JOYSTICK;

#define Joystick_ADC_Resolution	4096

#if(Joystick_ADC_Resolution == 4096)
#define JoystickIsFullUp(xy_arr)		((xy_arr[0]) >= 3600)
#define JoystickIsFullDown(xy_arr)		((xy_arr[0]) <= 120)
#define JoystickIsFullRight(xy_arr)		((xy_arr[1]) >= 3600)
#define JoystickIsFullLeft(xy_arr)		((xy_arr[1]) <= 120)
#define JoystickIsSlowUp(xy_arr)		(((xy_arr[0]) < 3600) && ((xy_arr[0]) > 2400))
#define JoystickIsSlowDown(xy_arr)		(((xy_arr[0]) > 120) && ((xy_arr[0]) < 1200))
#define JoystickIsSlowRight(xy_arr)		(((xy_arr[1]) < 3600) && ((xy_arr[1]) > 2400))
#define JoystickIsSlowLeft(xy_arr)		(((xy_arr[1]) > 120) && ((xy_arr[1]) < 1200))
#define JoystickIsReleased(xy_arr)		(((xy_arr[0]) > 2000 && ((xy_arr[0]) < 2400)) && ((xy_arr[1]) > 2000 && ((xy_arr[1]) < 2400)))
#endif

/*-----[ Prototypes For All Functions ]-----*/
void JoyStick_Init(JOYSTICK* Joystick_PTR);
void JoyStick_Read(JOYSTICK* Joystick_PTR, uint16_t* JoyStick_XY);

#endif /* INC_JOYSTICK_H_ */
