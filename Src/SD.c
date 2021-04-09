/*
 * SD.c
 *
 *  Created on: Apr 7, 2021
 *      Author: Yahia
 */

#include "main.h"
#include "fatfs.h"
#include "SD.h"

//File Variables
FATFS fs;  // file system
FIL file;
FRESULT res;
UINT byteswritten, bytesread;

// SD card init function
Std_ReturnType SD_init(GPIO_TypeDef * SD_CS_PORT, uint16_t SD_CS_PIN)
{
	Std_ReturnType return_type = E_NOT_OK;
	uint8 ErrorCount = 0;

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET);
	MX_FATFS_Init();
	do
	{
		return_type = f_mount(&fs , "" , 0);
		ErrorCount ++;
	}while(return_type != FR_OK && ErrorCount < 5);

	if(return_type == FR_OK)
		return_type = E_OK;
	else
		return_type = E_NOT_OK;

	return return_type;
}

// USB flash drive test Write function
Std_ReturnType SD_WriteOrCreate(uint8* wdataPTR, uint8* path, FlashDrive_WriteCreate state)
{
	Std_ReturnType return_type = E_NOT_OK;

	//Open or Create file for writing
	if(state == FLASHDRIVE_WRITE)
	{
		if(f_open(&file, (const void*)path, FA_WRITE) != FR_OK)
		{
			return return_type;
		}
	}
	else if(state == FLASHDRIVE_CREATE)
	{
		if(f_open(&file, (const void*)path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
		{
			return return_type;
		}
	}

	//Write to text file
	res = f_write(&file, (const void *)wdataPTR, strlen((const sint8*)wdataPTR), &byteswritten);
	if((res != FR_OK) || (byteswritten == 0))
	{
		return return_type;
	}

	f_close(&file);
	return_type = E_OK;
	return return_type; //Success

}

// USB flash drive test Write function
Std_ReturnType SD_AppendText(uint8* wdataPTR, uint8* path)
{
	Std_ReturnType return_type = E_NOT_OK;

	if(f_open(&file, (const void*)path, FA_WRITE) != FR_OK){return return_type;}

	if(f_lseek(&file, f_size(&file)) != FR_OK){return return_type;}

	//Write to text file
	res = f_write(&file, (const void *)wdataPTR, strlen((const sint8*)wdataPTR), &byteswritten);
	if((res != FR_OK) || (byteswritten == 0))
	{
		return return_type;
	}

	f_close(&file);
	return_type = E_OK;
	return return_type; //Success
}

// USB flash drive test Read function
Std_ReturnType SD_Read(uint8* rdataPTR, uint8* path)
{
	Std_ReturnType return_type = E_NOT_OK;
	uint32 size;
	uint32 count;

	//Open file for reading
	if(f_open(&file, (const void*)path, FA_READ) != FR_OK)
	{
		return return_type;
	}

	size = f_size(&file);

	//Read text from files until NULL
	for(count = 0; count < size; count++)
	{
		res = f_read(&file, (uint8*)&rdataPTR[count], 1, &bytesread);
		if(rdataPTR[count] == 0x00) // NULL string
		{
			bytesread = count;
			break;
		}
	}

	rdataPTR[count] = 0x0;
	bytesread = count;

	//Reading error handling
	if(bytesread == 0)
	{
		return return_type;
	}

	//Close file
	f_close(&file);
	return_type = E_OK;
	return return_type;  // success

}
