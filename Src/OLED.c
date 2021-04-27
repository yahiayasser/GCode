/*
 * OLED.c
 *
 *  Created on: Apr 7, 2021
 *      Author: Yahia
 */


#include "OLED.h"

static uint8 string_length(sint8 str[]);


static uint8 string_length(sint8 str[])
{
	uint8 count;
	/* count the string start from element 0 until the element before the NULL terminator */
	for(count = 0; str[count] != '\0'; ++count);
	return count;
}

Std_ReturnType OLED_Init(void)
{
	Std_ReturnType return_type = E_NOT_OK;
	if(SSD1306_Init() == 1)
	{
		return_type = E_OK;
	}

	return return_type;
}

void OLED_setTextDisplay(sint8* line1, sint8* line2, sint8* line3, sint8* line4)
{
	if(*line1)
	{
		SSD1306_GotoXY(0, 0);
		SSD1306_Puts((sint8*)line1, &Font_11x18, 1);
		for (uint8 count = string_length(line1); count < 20; count++)
		{
			SSD1306_Putc(' ', &Font_11x18, 1);
		}
	}
	if(*line2)
	{
		SSD1306_GotoXY(0, 1);
		SSD1306_Puts((sint8*)line2, &Font_11x18, 1);
		for (uint8 count = string_length(line2); count < 20; count++)
		{
			SSD1306_Putc(' ', &Font_11x18, 1);
		}
	}
	if(*line3)
	{
		SSD1306_GotoXY(0, 2);
		SSD1306_Puts((sint8*)line3, &Font_11x18, 1);
		for (uint8 count = string_length(line3); count < 20; count++)
		{
			SSD1306_Putc(' ', &Font_11x18, 1);
		}
	}
	if(*line4)
	{
		SSD1306_GotoXY(0, 3);
		SSD1306_Puts((sint8*)line4, &Font_11x18, 1);
		for (uint8 count = string_length(line4); count < 20; count++)
		{
			SSD1306_Putc(' ', &Font_11x18, 1);
		}
	}
}

void OLED_setTextRow(sint8* line, uint8 row)
{
	SSD1306_GotoXY(0, row);
	SSD1306_Puts((sint8*)line, &Font_11x18, 1);
	for (uint8 count = string_length(line); count < 20; count++)
	{
		SSD1306_Putc(' ', &Font_11x18, 1);
	}
}

void OLED_Clear(void)
{
	SSD1306_Clear();
}
