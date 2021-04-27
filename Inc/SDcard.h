
#ifndef SDCARD_H_
#define SDCARD_H_

#include <string.h>
#include <stdbool.h>

#include "stm32f1xx_hal.h"
#include "Platform_Types.h"
#include "ff.h"

typedef uint8 FlashDrive_WriteCreate;
#define FLASHDRIVE_WRITE	((FlashDrive_WriteCreate)0x0)
#define FLASHDRIVE_CREATE	((FlashDrive_WriteCreate)0x1)

// SD card init function
Std_ReturnType SD_init(GPIO_TypeDef * SD_CS_PORT, uint16_t SD_CS_PIN);
// SD card test Write function
Std_ReturnType SD_WriteOrCreate(uint8* wdataPTR, uint8* path, FlashDrive_WriteCreate state);
// SD card test Write function
Std_ReturnType SD_AppendText(uint8* wdataPTR, uint8* path);
// SD card test Read function
Std_ReturnType SD_Read(uint8* rdataPTR, uint8* path);
// SD card test Read function
Std_ReturnType SD_NextFileDirectory(sint8* path);

#endif
