/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
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
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
//LCD更新：
//功能：电脑串口消息显示在右边，蓝牙消息在左边。支持滚动，使用+++向上翻一行，---向下翻一行
//没有对其它代码有任何影响，没有改动除lcd部分的任何已有代码，变量只增不删
//缺陷：  1. 电脑串口发送的太快会出奇怪的消息错误，不如发123会收到1123，+++会收到++++，我不知道怎么解决，但是好像没啥影响
//	    2. 消息框不太好画。。。凑乎用吧，如果有一半的信息显示在屏幕，那么它的框子也不会显示
//	    3. 目前找不到比较好的刷新已有字符的办法，我现在使用全局刷新而不是局部刷新，效果好像还行，字符是逐渐显示的
//		2.3.可以改但是会很麻烦
//For LCD
#define MaxSize 200 //max block size
#define number_of_show_rows 9 //length of show rows
#define length_of_one_row 14
#define ScreenX1 5
#define ScreenY1 50
#define ScreenX2 230
#define ScreenY2 400
#define FontSize 24 //12 16 24
#define RowSpace 30
#define Bounda 40
#define Boundb 40
uint8_t rxBuffer[200];
static unsigned char uRx_Data_lcd[200] = { 0 };
static unsigned char Data_lcd[MaxSize][50]; //all data
static int DrawRectangle[MaxSize][2]; //if it is the start of one msg, the rows of this msg
static int rowLength[MaxSize];
static int side[MaxSize];
static int base = 0; //base of Data
static int start = 0; //index of show start
//static int line[showLength];
static unsigned char debugmsg[200];
static unsigned char up_msg[] = { '+', '+', '+' };
static unsigned char down_msg[] = { '-', '-', '-' };
int is_self;
int new_node_flag = 0;
//For Bluetooth
uint8_t rxBuffer1[1 << 8];
uint8_t rxBuffer2[1 << 8];
uint8_t msg[1 << 8];
int AT_flag;
uint8_t AT_msg[1 << 8];
int AT_TBC_flag; // AT ToBeContinued Flag

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//For LCD
//API
void ScreenShow() {
	int winstart = start;
	int line = 0;
	POINT_COLOR = WHITE;
	LCD_Fill(0, ScreenY1, ScreenX2 + 1, ScreenY2, BLACK);
	for (int n = winstart; n < winstart + number_of_show_rows; n++) {
		if (Data_lcd[n][0] != '\0') {
			if (side[n] == 0) { //right
				//#########################content###############
				POINT_COLOR = GREEN;
				BACK_COLOR = BLACK;
//				LCD_Fill(ScreenX1, ScreenY1 + RowSpace * line,
//				ScreenX2 - FontSize / 2 * rowLength[n],
//				ScreenY1 + RowSpace * (line + 1),
//				BLACK);
				LCD_ShowString(ScreenX2 - FontSize / 2 * rowLength[n],
				ScreenY1 + RowSpace * line, 200, FontSize, FontSize,
						(uint8_t*) Data_lcd[n]); //每个字符占的宽度是高度的一半
				//###########################frame################
				if (DrawRectangle[n][0] == 1) {
					POINT_COLOR = YELLOW;
					int x1 = ScreenX2 - FontSize / 2 * rowLength[n] - 2;
					int y1 = ScreenY1 + RowSpace * line;
					int x2 = ScreenX2 + 1;
					int y2 = ScreenY1 + RowSpace * (line + DrawRectangle[n][1])
							- 5;
					LCD_DrawRectangle(x1, y1, x2, y2);
				}

			} else { //left
				POINT_COLOR = WHITE;
				BACK_COLOR = BLACK;

//				LCD_Fill(ScreenX1 + FontSize / 2 * rowLength[n],
//				ScreenY1 + RowSpace * line,
//				ScreenX2,
//				ScreenY1 + RowSpace * (line + 1),
//				BLACK);
				LCD_ShowString(ScreenX1, ScreenY1 + RowSpace * line, 200,
				FontSize,
				FontSize, (uint8_t*) Data_lcd[n]);
				if (DrawRectangle[n][0] == 1) {
					POINT_COLOR = YELLOW;
					int x1 = ScreenX1 - 1;
					int y1 = ScreenY1 + RowSpace * line;
					int x2 = ScreenX1 + FontSize / 2 * rowLength[n] + 1;
					int y2 = ScreenY1 + RowSpace * (line + DrawRectangle[n][1])
							-5;
					LCD_DrawRectangle(x1, y1, x2, y2);
				}
			}
		}
//		} else {
//			LCD_Fill(ScreenX1, ScreenY1 + RowSpace * line, ScreenX2,
//			ScreenY1 + RowSpace * (line + 1), BLACK);
//		}
		line++;
	}
}

int checkhistory(unsigned char bluetooth_msg[]) {
	if (strlen(bluetooth_msg) == 3) {
		sprintf(debugmsg, "check: ");
		HAL_UART_Transmit(&huart1, debugmsg, 20, 0xffff);
		int x = 0;
		int y = 0;
		while (bluetooth_msg[x] == up_msg[x])
			x++;
		while (bluetooth_msg[y] == down_msg[y])
			y++;
		if (x == 4) { //不知道为啥4就完了
			if (start > 0)
				start--;
			sprintf(debugmsg, "up\n");
			HAL_UART_Transmit(&huart1, debugmsg, 20, 0xffff);
			return 1;
		}
		if (y == 4) {
			if (start < base)
				start++;
			sprintf(debugmsg, "down\n");
			HAL_UART_Transmit(&huart1, debugmsg, 20, 0xffff);
			return 1;
		}
	}
	return 0;
}

void add_message(unsigned char bluetooth_msg[], int is_self) {
	int bias = 0;
	int len = strlen(bluetooth_msg);
	if (!checkhistory(bluetooth_msg)) {
		int tmp = base;
		while (len > 0) {
			if (len >= length_of_one_row) {
				for (int n = 0; n < length_of_one_row; n++)
					Data_lcd[base][n] = bluetooth_msg[n
							+ bias * length_of_one_row];
				rowLength[base] = length_of_one_row;
			} else {
				for (int n = 0; n < len; n++)
					Data_lcd[base][n] = bluetooth_msg[n
							+ bias * length_of_one_row];
				rowLength[base] = len;
			}
			side[base] = is_self;
			base++;
			bias++;
			len -= length_of_one_row;
		}

		DrawRectangle[tmp][0] = 1;
		DrawRectangle[tmp][1] = base - tmp;
		start = base - number_of_show_rows;
		if (start < 0)
			start = 0;
	}
	new_node_flag = 1;
}

void clear_buffer(char *buffer, int length) {
	for (int i = 0; i < length; i++) {
		buffer[i] = '\0';
	}
}

void set_cmd(uint8_t *cmd) {
	AT_flag = 1;
	uint8_t success = 0;
	HAL_GPIO_WritePin(BLEN_GPIO_Port, BLEN_Pin, GPIO_PIN_SET);
	HAL_Delay(50);
	uint8_t cmd_msg[1 << 8];
	sprintf(cmd_msg, "%s\r\n", cmd);
	HAL_UART_Transmit(&huart2, cmd_msg, strlen(cmd_msg), 0xffff);
//TO DO, make the fast send safe
	uint8_t i = 5;
	while (AT_flag && i--) {
		HAL_Delay(20);
	}
	HAL_GPIO_WritePin(BLEN_GPIO_Port, BLEN_Pin, GPIO_PIN_RESET);

}

int starts_with(char *text, char *begining) {
	if (strlen(text) < strlen(begining)) {
		return 0;
	}
	for (int i = 0; i < strlen(begining); i++) {
		if (text[i] != begining[i]) {
			return 0;
		}
	}
	return 1;
}

char* get_state() {
	set_cmd("AT+STATE?");
	while (!starts_with(AT_msg, "+STATE:")) {
		HAL_Delay(500);
		set_cmd("AT+STATE?");
	}
	char *state_name = AT_msg;
	state_name += 7;
	int i = 0;
	while (1) {
//		HAL_UART_Transmit(&huart1, "..", strlen(".."), 0xffff);
		if (state_name[i] == '\n') {
			state_name[i - 1] = '\0';
			break;
		}
		i++;
	}
	return state_name;
}

int get_role() {
	set_cmd("AT+ROLE?");
	while (!starts_with(AT_msg, "+ROLE:")) {
		HAL_Delay(50);
		set_cmd("AT+ROLE?");
	}
	return AT_msg[6] == '1';
}

void default_connect() {
	HAL_UART_Transmit(&huart1, "START CONNECTION\n",
			strlen("START CONNECTION\n"), 0xffff);
	if (strcmp(get_state(), "CONNECTED")) { // not connected
		HAL_UART_Transmit(&huart1, "ENTER CONNECTION\n",
				strlen("ENTER CONNECTION\n"), 0xffff);
		if (get_role() == 1) {
			set_cmd("AT+ROLE=0");
		} else {
			set_cmd("AT+ROLE=1");
		}
		HAL_Delay(50);
		set_cmd("AT+RESET");
		HAL_UART_Transmit(&huart1, "END CONNECTION\n",
				strlen("END CONNECTION\n"), 0xffff);
	}
}

void disconnect() {
	char *state = get_state();
//	HAL_UART_Transmit(&huart1, state, strlen(state), 0xffff);
	if (!strcmp(state, "CONNECTED")) { // connected
		if (get_role() == 1) {
			set_cmd("AT+ROLE=0");
		} else {
			set_cmd("AT+ROLE=1");
		}
	}
	set_cmd("AT+DISC");
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART1) { // when received from PC
		static unsigned char uRx_Data1[1024] = { 0 };
		static unsigned char uLength1 = 0;
		if (rxBuffer1[0] == '\n') {
			uRx_Data1[uLength1] = '\0';
			if (uRx_Data1[0] == ':') {
				uint8_t temp_msg[1 << 8];
				unsigned char *uRx_Data_ptr = uRx_Data1;
				set_cmd(uRx_Data_ptr + 1);
				sprintf(temp_msg, "AT_answer: %s\n", AT_msg);
				HAL_UART_Transmit(&huart1, temp_msg, strlen(temp_msg), 0xffff);

			} else {
				// add zero char to the end and send to bluetooth
				sprintf(msg, "%s\0", uRx_Data1);
				HAL_UART_Transmit(&huart2, msg, strlen(msg) + 1, 0xffff);

				//display message
				add_message(msg, is_self);

				// send back to the PC

				sprintf(msg, "sent: %s\r\n", uRx_Data1);
				HAL_UART_Transmit(&huart1, msg, strlen(msg), 0xffff);

			}
			// reset uLength
			uLength1 = 0;
			for (int i = 0; i < 1024; i++) {
				uRx_Data1[i] = '\0';
			}
		} else {
			uRx_Data1[uLength1] = rxBuffer1[0];
			uLength1++;
		}
	} else { // when recieved from bluetooth
		static unsigned char uRx_Data[1024] = { 0 };
		static unsigned char uLength = 0;
		if (rxBuffer2[0] == '\0') { // msg of normal mode
			uRx_Data[uLength] = '\0';
			sprintf(msg, "recv: %s\n", uRx_Data);
			add_message(uRx_Data, ~is_self);
			HAL_UART_Transmit(&huart1, msg, strlen(msg), 0xffff);
			uLength = 0;
		} else if (rxBuffer2[0] == '\n') { // answer of AT mode
			uRx_Data[uLength] = '\0';
			if (uRx_Data[0] == '+') {
				clear_buffer(AT_msg, 1 << 8);
				strcpy(AT_msg, uRx_Data);
				AT_TBC_flag = 1;
			} else if (AT_TBC_flag) {
				sprintf(AT_msg, "%s\n%s\n", AT_msg, uRx_Data);
				AT_TBC_flag = 0;
				AT_flag = 0;
			} else {
				clear_buffer(AT_msg, 1 << 8);
				sprintf(AT_msg, "%s\n", uRx_Data);
				AT_flag = 0;
			}
			//HAL_UART_Transmit(&huart1, uRx_Data, strlen(uRx_Data), 0xffff);
			uLength = 0;
		} else {
			uRx_Data[uLength] = rxBuffer2[0];
			uLength++;
		}
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	switch (GPIO_Pin) {
	case KEYR_Pin:
		disconnect();
		break;
	case STATE_Pin:
//				HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, HAL_GPIO_ReadPin(STATE_GPIO_Port, STATE_Pin));
		break;
	case KEY0_Pin:
		HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
		default_connect();
		break;
	case KEY1_Pin:
		HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
		disconnect();
		break;
//		case KEY0_DOWN_Pin:
//			start--;
//			HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
//			HAL_Delay(250);
//			HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
//			break;
	default:
		break;
	}

}

char last_state[20];

void display_state() {
	char *state = get_state();
//if (!strcmp(state, last_state)) return;
	strcpy(last_state, state);
	char *display_msg[64];
	sprintf(display_msg, "State: %s", state);
	POINT_COLOR = WHITE;
	LCD_Color_Fill(0, 0, 240, 40, BLACK);
	if (!strcmp(state, "CONNECTED")) {
		POINT_COLOR = GREEN;
		LCD_ShowString(25, 10, 200, 24, 24, (uint8_t*) display_msg);
	} else {
		POINT_COLOR = RED;
		LCD_ShowString(25, 10, 200, 24, 24, (uint8_t*) display_msg);
	}
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */
	LCD_Init();
	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USART1_UART_Init();
	MX_USART2_UART_Init();
	/* USER CODE BEGIN 2 */
	HAL_UART_Receive_IT(&huart1, (uint8_t*) rxBuffer1, 1);
	HAL_UART_Receive_IT(&huart2, (uint8_t*) rxBuffer2, 1);
	AT_flag = 0;
	AT_TBC_flag = 0;
	new_node_flag = 0;

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	LCD_Clear(BLACK);
	BACK_COLOR = BLACK;
	while (1) {
		display_state();
		ScreenShow();
		int count = 0;
		while (!new_node_flag) {
			if (count++ > 25) {
				display_state();
				count = 0;
			}
			HAL_Delay(200);
		}
		new_node_flag = 0;
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 9600;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {

	/* USER CODE BEGIN USART2_Init 0 */

	/* USER CODE END USART2_Init 0 */

	/* USER CODE BEGIN USART2_Init 1 */

	/* USER CODE END USART2_Init 1 */
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 9600;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(BLEN_GPIO_Port, BLEN_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : KEYR_Pin */
	GPIO_InitStruct.Pin = KEYR_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(KEYR_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : KEY0_DOWN_Pin KEY1_Pin */
	GPIO_InitStruct.Pin = KEY0_DOWN_Pin | KEY1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pin : STATE_Pin */
	GPIO_InitStruct.Pin = STATE_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(STATE_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : BLEN_Pin */
	GPIO_InitStruct.Pin = BLEN_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(BLEN_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : KEY0_Pin */
	GPIO_InitStruct.Pin = KEY0_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(KEY0_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : LED0_Pin */
	GPIO_InitStruct.Pin = LED0_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LED0_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : LED1_Pin */
	GPIO_InitStruct.Pin = LED1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LED1_GPIO_Port, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI0_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(EXTI0_IRQn);

	HAL_NVIC_SetPriority(EXTI1_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(EXTI1_IRQn);

	HAL_NVIC_SetPriority(EXTI4_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(EXTI4_IRQn);

	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */

	/* USER CODE END Error_Handler_Debug */
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
