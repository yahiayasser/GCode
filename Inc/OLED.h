
#ifndef OLED_H
#define OLED_H

#include "stm32f1xx_hal.h"
#include "Platform_Types.h"
#include "fonts.h"
#include "ssd1306.h"

Std_ReturnType OLED_Init(void);
void OLED_setTextDisplay(sint8* line1, sint8* line2, sint8* line3, sint8* line4);
void OLED_setTextRow(sint8* line1, uint8 row);
void OLED_Clear(void);

#endif
