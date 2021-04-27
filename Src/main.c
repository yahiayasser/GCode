/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart1;

//File Variables
FATFS fs;  // file system
FIL file;
FILINFO fileInfo;
FRESULT res;
UINT byteswritten, bytesread;

/**** capacity related *****/
FATFS *pfs;
DWORD fre_clust;
uint32_t total, free_space;

/**** Joystick related *****/
uint16 Joystick_xy[2];
extern const JOYSTICK JoyStick_CfgParam;
JOYSTICK* Joystick_Handler = (JOYSTICK*)&JoyStick_CfgParam;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
Std_ReturnType getFileName(uint8 index, sint8* name);
uint8 filecount(void);
static uint8 string_length(sint8 str[]);
static void string_cat(sint8 str1[],sint8 str2[]);
static void StringCopy(sint8 str1[], sint8 str2[]);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
	/* USER CODE BEGIN 1 */
	uint8 A;

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_I2C1_Init();
	MX_SPI1_Init();
	MX_USART1_UART_Init();
	/* USER CODE BEGIN 2 */
	JoyStick_Init(Joystick_Handler);

	if(OLED_Init() != E_OK)
	{
		//Error
	}

	// Ask to connect (you might still use a computer and NOT connect this controller)
	OLED_setTextDisplay("","      Connect?    ","","");
	while (HAL_GPIO_ReadPin(Joystick_Button_GPIO_Port, Joystick_Button_Pin) == GPIO_PIN_RESET) {}  // wait for the button to be pressed
	//delay(50);
	while (HAL_GPIO_ReadPin(Joystick_Button_GPIO_Port, Joystick_Button_Pin) == GPIO_PIN_SET) {}  // be sure the button is released before we continue
	//delay(50);

	if(SD_init(SD_CS_GPIO_Port, SD_CS_Pin) != E_OK)
	{
		OLED_setTextDisplay("Error","SD Card Fail!","","");
		//Error
	}

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		/* USER CODE END WHILE */
		A = fileMenu();
		if (A==0) {
			moveMenu();
		} else {
			//sendFile(A);
		}
		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

uint8 fileMenu(void)
{
	/*
	  This is the file menu.
	  You can browse up and down to select a file.
	  Click the button to select a file

	  Move the stick right to exit the file menu and enter the Move menu
	 */
	uint8 fileindex = 1;
	sint8 fn[50];
	uint8 fc = filecount();
	sint8 string[100];

	getFileName(fileindex, fn);
	StringCopy(string, " -> ");
	string_cat(string, fn);
	OLED_setTextDisplay("Files ",string,0,"Click to select");

	while(1)
	{
		JoyStick_Read(Joystick_Handler, Joystick_xy);
		if(fileindex < fc && (JoystickIsFullDown(Joystick_xy)))
		{
			fileindex++;
			getFileName(fileindex, fn);
			StringCopy(string, " -> ");
			string_cat(string, fn);
			OLED_setTextRow(string, 1);
			waitForJoystickMid();
		}
		if(JoystickIsFullUp(Joystick_xy))
		{
			if(fileindex > 1)
			{
		        fileindex --;
		        fn[0] = '\0';
				getFileName(fileindex, fn);
				StringCopy(string, " -> ");
				string_cat(string, fn);
				OLED_setTextRow(string, 1);
				waitForJoystickMid();
			}
		}

		if (fileindex > 0 && (JoyStick_ReadButton() == JOYSTICK_PIN_RESET) && fn[0] != '\0')    // Pushed it!
		{
			StringCopy(string, " -> ");
			string_cat(string, fn);
			OLED_setTextDisplay("Send this file? ", string, 0, "Click to confirm");  // Ask for confirmation
			HAL_Delay(50);
			while (JoyStick_ReadButton() == JOYSTICK_PIN_RESET) {} // Wait for the button to be released
			HAL_Delay(50);

			volatile uint32 t = HAL_GetTick();

			while((HAL_GetTick() - t) < 1500)
			{
				if(JoyStick_ReadButton() == JOYSTICK_PIN_RESET)
				{
					HAL_Delay(50);
					while (JoyStick_ReadButton() == JOYSTICK_PIN_RESET) {} // Wait for the button to be released
					return fileindex;
					break;
				}
			}
			StringCopy(string, " -> ");
			string_cat(string, fn);
			OLED_setTextDisplay("Files ", string, 0, "Click to select");
		}

		if(JoystickIsFullRight(Joystick_xy))
		{
			waitForJoystickMid();
			return 0;
			StringCopy(string, " -> ");
			string_cat(string, fn);
			OLED_setTextDisplay("Files ", string, 0, "Click to select");
		}
	}
}

void moveMenu(void)
{
	OLED_Clear();
	sint8 MoveCommand[50] = "";
	boolean hardup,harddown,slowup, slowdown, updateDisplay;
	uint32 qtime;
	uint64 queue=0; // queue length in milliseconds
	uint64 startTime,lastUpdate;

	sint8 sln1[21];
	sint8 sln2[21];
	sint8 sln3[21];
	sint8 sln4[21];
	sint8 sla[30];
	sint8 slb[30];

	//clearRXBuffer();
}
/*
void sendFile(uint8 byte)
{

}
*/
uint8 filecount(void)
{
	/*
	    Count the number of files on the SD card.
	 */
	uint8 count =0;
	sint8 path[50] = "/";
	while(SD_NextFileDirectory(path) == E_OK)
	{
		count++;
	}

	return count;
}

Std_ReturnType getFileName(uint8 index, sint8* name)
{
	Std_ReturnType return_type = E_NOT_OK;
	uint8 count = 0;
	sint8 path[50] = "/";

	name[0] = 0;

	if(SD_NextFileDirectory(path) != E_OK){return return_type;}

	while(name[0] == 0)
	{
		if(SD_NextFileDirectory(path) != FR_OK){}
		else
		{
			if (fileInfo.fattrib & AM_DIR)
			{
				count++;
				if(count == index)
				{
#if _USE_LFN
					name = *fileInfo.lfname ? fileInfo.lfname : fileInfo.fname;
#else
					name = fileInfo.fname;
#endif
				}
			}
		}
	}

	return_type = E_OK;
	return return_type; //Success
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
			|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
	PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
void MX_I2C1_Init(void)
{

	/* USER CODE BEGIN I2C1_Init 0 */

	/* USER CODE END I2C1_Init 0 */

	/* USER CODE BEGIN I2C1_Init 1 */

	/* USER CODE END I2C1_Init 1 */
	hi2c1.Instance = I2C1;
	hi2c1.Init.ClockSpeed = 400000;
	hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN I2C1_Init 2 */

	/* USER CODE END I2C1_Init 2 */

}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
void MX_SPI1_Init(void)
{

	/* USER CODE BEGIN SPI1_Init 0 */

	/* USER CODE END SPI1_Init 0 */

	/* USER CODE BEGIN SPI1_Init 1 */

	/* USER CODE END SPI1_Init 1 */
	/* SPI1 parameter configuration*/
	hspi1.Instance = SPI1;
	hspi1.Init.Mode = SPI_MODE_MASTER;
	hspi1.Init.Direction = SPI_DIRECTION_2LINES;
	hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi1.Init.NSS = SPI_NSS_SOFT;
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi1.Init.CRCPolynomial = 10;
	if (HAL_SPI_Init(&hspi1) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN SPI1_Init 2 */

	/* USER CODE END SPI1_Init 2 */

}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
void MX_USART1_UART_Init(void)
{

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 115200;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/**
 * Enable DMA controller clock
 */
void MX_DMA_Init(void)
{
	/* DMA controller clock enable */
	__HAL_RCC_DMA1_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA1_Channel1_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : Joystick_Button_Pin */
	GPIO_InitStruct.Pin = Joystick_Button_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(Joystick_Button_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : SD_CS_Pin */
	GPIO_InitStruct.Pin = SD_CS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(SD_CS_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : MS12_Pin MS3_Pin MS6_Pin MS9_Pin */
	GPIO_InitStruct.Pin = MS12_Pin|MS3_Pin|MS6_Pin|MS9_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */

	/* USER CODE END Error_Handler_Debug */
}

static uint8 string_length(sint8 str[])
{
	uint8 count;
	/* count the string start from element 0 until the element before the NULL terminator */
	for(count = 0; str[count] != '\0'; ++count);
	return count;
}

static void string_cat(sint8 str1[],sint8 str2[])
{
	uint8 str1_length, count;
	/* This loop is to store the length of str1 in i
	 * It just counts the number of characters in str1
	 * You can also use strlen instead of this.
	 */
	str1_length = string_length(str1);

	/* This loop would concatenate the string str2 at
	 * the end of str1
	 */
	for(count=0; str2[count]!='\0'; ++count, ++str1_length)
	{
		str1[str1_length]=str2[count];
	}
	/* \0 represents end of string */
	str1[str1_length]='\0';
}

static void StringCopy(sint8 str1[], sint8 str2[]){
	uint8 count = 0;
	while(str2[count] != '\0'){
		str1[count] = str2[count];
		count++;
	}
	str1[count] = '\0';
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
