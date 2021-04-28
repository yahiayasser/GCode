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
DMA_HandleTypeDef hdma_usart1_rx;

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

boolean awaitingOK = FALSE;   // this is set true when we are waiting for the ok signal from the grbl board (see the sendCodeLine() void)
uint64 runningTime = 0;

sint8 machineStatus[10];   // last know state (Idle, Run, Hold, Door, Home, Alarm, Check)

// Globals
sint8 WposX[9];            // last known X pos on workpiece, space for 9 characters ( -999.999\0 )
sint8 WposY[9];            // last known Y pos on workpiece
sint8 WposZ[9];            // last known Z heighton workpiece, space for 8 characters is enough( -99.999\0 )
sint8 MposX[9];            // last known X pos absolute to the machine
sint8 MposY[9];            // last known Y pos absolute to the machine
sint8 MposZ[9];            // last known Z height absolute to the machine

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

/* USER CODE BEGIN PFP */
Std_ReturnType getFileName(uint8 index, sint8* name);
uint8 filecount(void);
void checkForOk(void);
void getStatus(void);
void updateDisplayStatus(uint64 runtime);
void sendCodeLine(sint8 *lineOfCode, boolean waitForOk);
void removeIfExists(sint8* lineOfCode,sint8* toBeRemoved);
void emergencyBreak(void);
void StopSpindle(void);
void ignoreUnsupportedCommands(sint8* lineOfCode);
void SpindleSlowStart(void);
static uint8 string_length(sint8 str[]);
static void string_cat(sint8 str1[],sint8 str2[]);
static void StringCopy(sint8 str1[], sint8 str2[]);
static void NumberToString(uint64 num, sint8* str);
uint8 Strings_Is_Equal(sint8 Str1[], sint8 Str2[]);
uint8 String_FindSubstringIndex(sint8* string, sint8* subString, uint8* index);
uint8 String_ReplaceSubstring(sint8* string, sint8* subString, sint8* replacement);
uint8 String_IsStartWith(sint8* string, sint8 Char);
void Sring_Trim(sint8* string);
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
	//MX_DMA_Init();
	MX_I2C1_Init();
	MX_SPI1_Init();
	MX_USART1_UART_Init();

	/*if (HAL_OK != HAL_UART_Receive_DMA(&huart1, aRXBufferUser, RX_BUFFER_SIZE))
	{
		Error_Handler();
	}*/

	/* USER CODE BEGIN 2 */
	JoyStick_Init(Joystick_Handler);

	if(OLED_Init() != E_OK)
	{
		Error_Handler();
	}

	// Ask to connect (you might still use a computer and NOT connect this controller)
	OLED_setTextDisplay("","      Connect?    ","","");
	while (HAL_GPIO_ReadPin(Joystick_Button_GPIO_Port, Joystick_Button_Pin) == GPIO_PIN_RESET) {}  // wait for the button to be pressed
	HAL_Delay(50);
	while (HAL_GPIO_ReadPin(Joystick_Button_GPIO_Port, Joystick_Button_Pin) == GPIO_PIN_SET) {}  // be sure the button is released before we continue
	HAL_Delay(50);

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
			sendFile(A);
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
	uint64 startTime,lastUpdate;

	sint8 sln1[21];
	sint8 sln2[21];
	sint8 sln3[21];
	sint8 sln4[21];
	sint8 sla[30];
	sint8 slb[30];

	sendCodeLine("G21", TRUE);
	sendCodeLine("G91",TRUE); // Switch to relative coordinates

	while (!Strings_Is_Equal(MoveCommand, "-1"))
	{
		MoveCommand [0] = '\0';
		// read the state of all inputs
		JoyStick_Read(Joystick_Handler, Joystick_xy);
		hardup   = !HAL_GPIO_ReadPin(MS12_GPIO_Port, MS12_Pin);
		harddown = !HAL_GPIO_ReadPin(MS9_GPIO_Port, MS9_Pin);
		slowup   = !HAL_GPIO_ReadPin(MS3_GPIO_Port, MS3_Pin);
		slowdown = !HAL_GPIO_ReadPin(MS6_GPIO_Port, MS6_Pin);

		if (Joystick_xy[1] < 30)
		{
			StringCopy(MoveCommand, "G1 X-5 F2750"); // Slow Left
		}
		else
		{
			if (Joystick_xy[1] < 300)
			{
				StringCopy(MoveCommand, "G1 X-1 F500"); // Slow Left
			}
			else
			{
				if (Joystick_xy[1] > 900)
				{
					StringCopy(MoveCommand, "G1 X5 F2750");   // Full right
				}
				else
				{
					if (Joystick_xy[1] > 600)
					{
						StringCopy(MoveCommand, "G1 X1 F500");  // Slow Right
					}
				}
			}
		}

		if (Joystick_xy[0] < 30)
		{
			StringCopy(MoveCommand, "G1 Y-5 F2750");  // Full Reverse
		}
		else
		{
			if (Joystick_xy[0] < 300)
			{
				StringCopy(MoveCommand, "G1 Y-1 F500");   // slow in reverse
			}
			else
			{
				if (Joystick_xy[0] > 900)
				{
					StringCopy(MoveCommand, "G1 Y5 F2750");  // Full forward
				}
				else
				{
					if (Joystick_xy[0] > 600)
					{
						StringCopy(MoveCommand, "G1 Y1 F500");  // slow forward
					}
				}
			}
		}

		if (slowup)  {StringCopy(MoveCommand, "G1 Z0.2 F110");}    // Up Z
		if (hardup)  {StringCopy(MoveCommand, "G1 Z1 F2000");}     // Full up Z
		if (slowdown){StringCopy(MoveCommand, "G1 Z-0.2 F110");}   // Down Z
		if (harddown){StringCopy(MoveCommand, "G1 Z-1 F2000");}    // Full down Z

		if (MoveCommand[0] != '\0')
		{
			// send the commands
			sendCodeLine(MoveCommand,TRUE);
			MoveCommand[0] = '\0';
		}

		if (MoveCommand[0] == '\0')
			startTime = HAL_GetTick();
		// get the status of the machine and monitor the receive buffer for OK signals

		if ((HAL_GetTick() - lastUpdate) >= 500) {
			getStatus();
			lastUpdate=HAL_GetTick();
			updateDisplay = TRUE;
		}

		if (updateDisplay) {
			updateDisplay = FALSE;
			StringCopy(sln1, "X: ");
			string_cat(sln1, WposX);

			StringCopy(sln2, "Y: ");
			string_cat(sln2, WposY);

			StringCopy(sln3, "Z: ");
			string_cat(sln3, WposZ);

			StringCopy(sln4, "Click stick to exit");

			StringCopy(slb, WposX);
			string_cat(slb, " ");
			StringCopy(slb, WposY);
			string_cat(slb, " ");
			StringCopy(slb, WposZ);

			if (sla != slb) {
				OLED_setTextDisplay(sln1,sln2,sln3,sln4);
				StringCopy(sla,slb);
			}
		}

		if (JoyStick_ReadButton() == JOYSTICK_PIN_RESET) { // button is pushed, exit the move loop
			// set x,y and z to 0
			sendCodeLine("G92 X0 Y0 Z0",TRUE); //For GRBL v8
			getStatus();
			OLED_Clear();
			StringCopy(MoveCommand,"-1");
			while (JoyStick_ReadButton() == JOYSTICK_PIN_RESET) {}; // wait until the user releases the button
			HAL_Delay(10);
		}
	}
}

void sendFile(sint8 fileIndex)
{
	sint8 strLine[100] = "";
	sint8 filename[20];
	uint64 lastUpdate = 0;;

	if(getFileName((uint8)fileIndex, filename) != E_OK)
	{
		OLED_setTextDisplay("File","", "Error, file not found","");
		HAL_Delay(1000); // show the error
		return;
	}

	// Set the Work Position to zero
	sendCodeLine("G90",TRUE); // absolute coordinates
	sendCodeLine("G21",TRUE);
	sendCodeLine("G92 X0 Y0 Z0",TRUE);  // set zero

	// Start the spindle
	SpindleSlowStart();

	// reset the timer
	runningTime = HAL_GetTick();

	if (!awaitingOK)
	{
		SD_ReadUntil((uint8*)strLine, (uint8*)filename, '\n');
		ignoreUnsupportedCommands(strLine);
		if (strLine[0] != '\0') sendCodeLine(strLine,TRUE);    // sending it!
	}

	// get the status of the machine and monitor the receive buffer for OK signals
	if (HAL_GetTick() - lastUpdate >= 250) {
		lastUpdate=HAL_GetTick();
		updateDisplayStatus(runningTime);
	}
	if (!HAL_GPIO_ReadPin(MS12_GPIO_Port, MS12_Pin)) {emergencyBreak();}


	/*
	   End of File!
	   All Gcode lines have been send but the machine may still be processing them
	   So we query the status until it goes Idle
	 */

	while (Strings_Is_Equal (machineStatus,"Idle") != 0) {
		if (!HAL_GPIO_ReadPin(MS12_GPIO_Port, MS12_Pin)) {emergencyBreak();}
		HAL_Delay(250);
		getStatus();
		updateDisplayStatus(runningTime);
	}
	// Now it is done.

	// Stop the spindle
	StopSpindle();

	OLED_setTextRow("                ", 1);
	OLED_setTextRow("                ", 2);
	OLED_setTextRow("                ", 3);
	while (JoyStick_ReadButton()==JOYSTICK_PIN_SET) {} // Wait for the button to be pressed
	HAL_Delay(50);
	while (JoyStick_ReadButton()==JOYSTICK_PIN_RESET) {} // Wait for the button to be released
	HAL_Delay(50);
	return;
}

void StopSpindle(void)
{
	HAL_GPIO_WritePin(SpindleStartRelay_GPIO_Port, SpindleStartRelay_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SpindleRelay_GPIO_Port, SpindleRelay_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(yellowLED_GPIO_Port, yellowLED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(greenLED_GPIO_Port, greenLED_Pin, GPIO_PIN_RESET);
}

void emergencyBreak(void){
	uint8 data = 24;
	HAL_UART_Transmit(&huart1, (uint8*)"!", 1, 100); // feed hold
	OLED_setTextDisplay("Pause","Green = Continue","Red = Reset","");
	while (TRUE) {
		if (!HAL_GPIO_ReadPin(MS3_GPIO_Port, MS3_Pin)) {HAL_UART_Transmit(&huart1, (uint8*)"~", 1, 100);return;} // send continue and return
		if (!HAL_GPIO_ReadPin(MS6_GPIO_Port, MS6_Pin)) {
			HAL_UART_Transmit(&huart1, &data, 1, 100);
			HAL_Delay(500);
			//HAL_GPIO_WritePin(resetpin_GPIO_Port, resetpin_Pin, GPIO_PIN_RESET);
		}
	}
}

void ignoreUnsupportedCommands(sint8* lineOfCode)
{
	/*
	  Remove unsupported codes, either because they are unsupported by GRBL or because I choose to.
	 */
	removeIfExists(lineOfCode,"G64");   // Unsupported: G64 Constant velocity mode
	removeIfExists(lineOfCode,"G40");   // unsupported: G40 Tool radius comp off
	removeIfExists(lineOfCode,"G41");   // unsupported: G41 Tool radius compensation left
	removeIfExists(lineOfCode,"G81");   // unsupported: G81 Canned drilling cycle
	removeIfExists(lineOfCode,"G83");   // unsupported: G83 Deep hole drilling canned cycle
	removeIfExists(lineOfCode,"M6");    // ignore Tool change
	removeIfExists(lineOfCode,"M7");    // ignore coolant control
	removeIfExists(lineOfCode,"M8");    // ignore coolant control
	removeIfExists(lineOfCode,"M9");    // ignore coolant control
	removeIfExists(lineOfCode,"M10");   // ignore vacuum, pallet clamp
	removeIfExists(lineOfCode,"M11");   // ignore vacuum, pallet clamp
	removeIfExists(lineOfCode,"M5");    // ignore spindle off
	String_ReplaceSubstring(lineOfCode, "M2 ", "M5 M2 "); // Shut down spindle on program end.

	// Ignore comment lines
	// Ignore tool commands, I do not support tool changers
	if (String_IsStartWith(lineOfCode, '(') || String_IsStartWith(lineOfCode, 'T') ) {lineOfCode[0] = '\0';}
	Sring_Trim(lineOfCode);
}

void removeIfExists(sint8* lineOfCode,sint8* toBeRemoved )
{
	String_ReplaceSubstring(lineOfCode, toBeRemoved, " ");
}

void SpindleSlowStart(void)
{
	// The first relay gives power to the spindle through a 1 ohm power resistor.
	// This limits the current just enough to prevent the current protection.
	//HAL_GPIO_WritePin(yellowLED_Button_GPIO_Port, yellowLED_Button_Pin, GPIO_PIN_SET);
	//HAL_GPIO_WritePin(SpindleStartRelay_Button_GPIO_Port, SpindleStartRelay_Button_Pin, GPIO_PIN_RESET);
	HAL_Delay(500);
	//HAL_GPIO_WritePin(yellowLED_Button_GPIO_Port, yellowLED_Button_Pin, GPIO_PIN_RESET);

	// After the spindle reaches full speed, the second relay takes over, this relay powers the
	// spindle without any resitors
	//HAL_GPIO_WritePin(SpindleRelay_Button_GPIO_Port, SpindleRelay_Button_Pin, GPIO_PIN_RESET);
	//HAL_GPIO_WritePin(greenLED_Button_GPIO_Port, greenLED_Button_Pin, GPIO_PIN_SET);
	HAL_Delay(1000); // wait for the spindle to rev up completely
}

void sendCodeLine(sint8 *lineOfCode, boolean waitForOk)
{
	uint32 updateScreen = 0;
	HAL_UART_Transmit(&huart1, (uint8*)lineOfCode, string_length(lineOfCode), 5000);
	awaitingOK = TRUE;
	//HAL_Delay(10);
	checkForOk();
	while (waitForOk && awaitingOK)
	{
		HAL_Delay(50);
		if (updateScreen++ > 4)
		{
			updateScreen=0;
			updateDisplayStatus(runningTime);
		}
		checkForOk();
	}
}

void updateDisplayStatus(uint64 runtime)
{
	/*
	   I had some issues with updating the display while carving a file
	   I created this extra void, just to update the display while carving.
	 */

	uint64 t = HAL_GetTick() - runtime;
	uint32 H,M,S;
	sint8 StringNum[4];
	sint8 StringTime[10];
	sint8 StringMachine[20];

	t=t/1000;
	// Now t is the a number of seconds.. we must convert that to "hh:mm:ss"
	H = t/3600;
	t = t - (H * 3600);
	M = t/60;
	S = t - (M * 60);
	NumberToString(H, StringNum);
	StringCopy(StringTime, StringNum);
	string_cat(StringTime, ";");
	NumberToString(M, StringNum);
	StringCopy(StringTime, StringNum);
	string_cat(StringTime, ";");
	NumberToString(S, StringNum);
	StringCopy(StringTime, StringNum);

	getStatus();
	OLED_Clear();
	StringCopy(StringMachine, machineStatus);
	string_cat(StringMachine, " ");
	string_cat(StringMachine, StringTime);
	OLED_setTextRow(StringMachine, 0);
	StringCopy(StringMachine, "X: ");
	string_cat(StringMachine, WposX);
	string_cat(StringMachine, " ");
	OLED_setTextRow(StringMachine, 1);
	StringCopy(StringMachine, "Y: ");
	string_cat(StringMachine, WposY);
	string_cat(StringMachine, " ");
	OLED_setTextRow(StringMachine, 1);
	StringCopy(StringMachine, "Z: ");
	string_cat(StringMachine, WposZ);
	string_cat(StringMachine, " ");
	OLED_setTextRow(StringMachine, 2);
}

void getStatus(void)
{
	/*
	    This gets the status of the machine
	    The status message of the machine might look something like this (this is a worst scenario message)
	    The max length of the message is 72 characters long (including carriage return).

	    <Check,MPos:-995.529,-210.560,-727.000,WPos:-101.529,-115.440,-110.000>
	 */

	sint8 content[80];
	sint8 character;
	uint8 index=0;
	boolean completeMessage=false;
	uint32 i=0;
	uint32 c=0;

	checkForOk();
	HAL_UART_Transmit(&huart1, (uint8*)"?", 2, 5000);

	while(!(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE))){}
	while(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE))
	{
		HAL_UART_Receive(&huart1, (uint8*)&character, 1, 100);
		if (content[index] =='>') completeMessage=TRUE; // a simple check to see if the message is complete
		if (index>0) {
			if (content[index]=='k' && content[index-1]=='o') {awaitingOK=FALSE;}
		}
		index++;
		HAL_Delay(1);
	}

	if (!completeMessage) { return; }
	i++;
	while (c<9 && content[i] !=',') {machineStatus[c++]=content[i++]; machineStatus[c]=0; } // get the machine status
	while (content[i++] != ':') ; // skip until the first ':'
	c=0;
	while (c<8 && content[i] !=',') { MposX[c++]=content[i++]; MposX[c] = 0;} // get MposX
	c=0; i++;
	while (c<8 && content[i] !=',') { MposY[c++]=content[i++]; MposY[c] = 0;} // get MposY
	c=0; i++;
	while (c<8 && content[i] !=',') { MposZ[c++]=content[i++]; MposZ[c] = 0;} // get MposZ
	while (content[i++] != ':') ; // skip until the next ':'
	c=0;
	while (c<8 && content[i] !=',') { WposX[c++]=content[i++]; WposX[c] = 0;} // get WposX
	c=0; i++;
	while (c<8 && content[i] !=',') { WposY[c++]=content[i++]; WposY[c] = 0;} // get WposY
	c=0; i++;
	while (c<8 && content[i] !='>') { WposZ[c++]=content[i++]; WposZ[c] = 0;} // get WposZ

	if (WposZ[0]=='-')
	{ WposZ[5]='0';WposZ[6]=0;}
	else
	{ WposZ[4]='0';WposZ[5]=0;}
}

void checkForOk(void)
{
	// read the receive buffer (if anything to read)
	uint8 c,lastc;
	c=64;
	lastc=64;

	while(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE))
	{
		HAL_UART_Receive(&huart1, &c, 1, 100);
		if (lastc =='o' && c == 'k')
		{
			awaitingOK = FALSE;
		}
		lastc = c;
		HAL_Delay(3);
	}
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
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
void MX_ADC1_Init(void)
{

	/* USER CODE BEGIN ADC1_Init 0 */

	/* USER CODE END ADC1_Init 0 */

	ADC_ChannelConfTypeDef sConfig = {0};

	/* USER CODE BEGIN ADC1_Init 1 */

	/* USER CODE END ADC1_Init 1 */
	/** Common config
	 */
	hadc1.Instance = ADC1;
	hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
	hadc1.Init.ContinuousConvMode = ENABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion = 2;
	if (HAL_ADC_Init(&hadc1) != HAL_OK)
	{
		Error_Handler();
	}
	/** Configure Regular Channel
	 */
	sConfig.Channel = ADC_CHANNEL_1;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/** Configure Regular Channel
	 */
	sConfig.Channel = ADC_CHANNEL_2;
	sConfig.Rank = ADC_REGULAR_RANK_2;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN ADC1_Init 2 */

	/* USER CODE END ADC1_Init 2 */

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
	/* DMA1_Channel5_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

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

static void NumberToString(uint64 num, sint8* str)
{
	uint8 count, rem, length = 0;
	uint64 value = num;

	while (value != 0)
	{
		length++;
		value /= 10;
	}
	for (count = 0; count < length; count++)
	{
		rem = num % 10;
		num = num / 10;
		str[length - (count + 1)] = rem + '0';
	}
	str[length] = '\0';
}

uint8 Strings_Is_Equal(sint8 Str1[], sint8 Str2[]){
	uint8 count = 0;
	uint8 Str1_length = string_length(Str1);
	uint8 Str2_length = string_length(Str2);
	if(Str1_length == Str2_length){
		while(count < Str1_length){
			if(Str1[count] != Str2[count]){
				return FALSE;
			}
			count++;
		}
		return TRUE;
	}
	else{
		return FALSE;
	}
}

uint8 String_FindSubstringIndex(sint8* string, sint8* subString, uint8* index)
{
	uint8 StringCount = 0;
	uint8 SubStringCount = 0;

	uint8 StringSize = string_length(string);

	uint8 matchFlag = 0;

	for(; StringCount < StringSize; StringCount++)
	{
		if(string[StringCount] == subString[SubStringCount])
		{
			if(SubStringCount == 0)
			{
				*index = StringCount;
				matchFlag = 1;
			}
			else if(subString[SubStringCount + 1] == '\0')
			{
				return 1;
			}
			SubStringCount++;
		}
		else if(matchFlag == 1)
		{
			matchFlag = 0;
			SubStringCount = 0;
		}
	}

	return 0;
}

uint8 String_ReplaceSubstring(sint8* string, sint8* subString, sint8* replacement)
{
	uint8 returnValue = 0;
	uint8 index;
	uint8 SubstringSize = string_length(subString);
	uint8 StringCount = 0;

	sint8 output[255] = "";
	uint8 OutputCount = 0;

	if(1 == String_FindSubstringIndex(string, subString, &index))
	{
		for(; StringCount < index; StringCount++)
		{
			output[StringCount] = string[StringCount];
		}

		OutputCount = index;

		for(; replacement[OutputCount - index] != '\0'; OutputCount++)
		{
			output[OutputCount] = replacement[OutputCount - index];
		}

		StringCount = StringCount + SubstringSize;

		while(string[StringCount] != '\0')
		{
			output[OutputCount] = string[StringCount];
			StringCount++;
			OutputCount++;
		}

		output[OutputCount] = '\0';
		OutputCount = 0;

		while(output[OutputCount] != '\0')
		{
			string[OutputCount] = output[OutputCount];
			OutputCount++;
		}

		string[OutputCount] = '\0';
		returnValue = 1;
	}

	return returnValue;
}

uint8 String_IsStartWith(sint8* string, sint8 Char)
{
	if(string[0] == Char)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void Sring_Trim(sint8* string)
{
	uint8 count1 = 0, count2 = 0;
	sint8 output[255] = "";
	while(string[count1] != '\0')
	{
		if(string[count1] != ' ')
		{
			output[count2] = string[count1];
			count2 ++;
		}
		count1 ++;
	}
	output[count2] = '\0';
	count2 = 0;

	while(output[count2] != '\0')
	{
		string[count2] = output[count2];
		count2++;
	}
	string[count2] = '\0';
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
