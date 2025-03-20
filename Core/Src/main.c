/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  * Author: Maltsev Yuriy
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <task.h>
#include <stdio.h>    // printf
#include <stdbool.h>
#include <stdlib.h>   // atoi
#include <stdint.h>   // uint32_t uint8_t
#include <string.h>   // strlen, strcmp
#include <ctype.h>    // isxdigit isspace toupper
//#include <signal.h>
//#include <time.h>
//#include <sys/time.h>
//#include <sys/times.h>

//#include "os.h"
#include "CRC.h"
#include "buttons.h"
#include "../BH1750/bh1750.h" // либо включить папку в список путей. Правой - Add/remove include path
#include "eeprom.h"
#include "i2c.h"
#include "bmp280.h"
#include "ATH25.h"
#include "testspeedcopy.h"
//#include "mt6701.h"

// то что хотим видеть в глобальной области видимости, подключаем непосредственно в "main.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticSemaphore_t osStaticMutexDef_t;
typedef StaticSemaphore_t osStaticSemaphoreDef_t;
typedef StaticEventGroup_t osStaticEventGroupDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define INCLUDE_vTaskSuspend 1
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

CRC_HandleTypeDef hcrc;

DAC_HandleTypeDef hdac;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

IWDG_HandleTypeDef hiwdg;

RNG_HandleTypeDef hrng;

TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart2;

DMA_HandleTypeDef hdma_memtomem_dma2_stream1;
/* Definitions for keyReadTask */
osThreadId_t keyReadTaskHandle;
uint32_t keyReadTaskBuffer[ 128 ];
osStaticThreadDef_t keyReadTaskControlBlock;
const osThreadAttr_t keyReadTask_attributes = {
  .name = "keyReadTask",
  .cb_mem = &keyReadTaskControlBlock,
  .cb_size = sizeof(keyReadTaskControlBlock),
  .stack_mem = &keyReadTaskBuffer[0],
  .stack_size = sizeof(keyReadTaskBuffer),
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for uartReadTask */
osThreadId_t uartReadTaskHandle;
uint32_t uartReadTaskBuffer[ 256 ];
osStaticThreadDef_t uartReadTaskControlBlock;
const osThreadAttr_t uartReadTask_attributes = {
  .name = "uartReadTask",
  .cb_mem = &uartReadTaskControlBlock,
  .cb_size = sizeof(uartReadTaskControlBlock),
  .stack_mem = &uartReadTaskBuffer[0],
  .stack_size = sizeof(uartReadTaskBuffer),
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for ADC1ReadTask */
osThreadId_t ADC1ReadTaskHandle;
uint32_t myTaskADC1Buffer[ 128 ];
osStaticThreadDef_t myTaskADC1ControlBlock;
const osThreadAttr_t ADC1ReadTask_attributes = {
  .name = "ADC1ReadTask",
  .cb_mem = &myTaskADC1ControlBlock,
  .cb_size = sizeof(myTaskADC1ControlBlock),
  .stack_mem = &myTaskADC1Buffer[0],
  .stack_size = sizeof(myTaskADC1Buffer),
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for BMP280Task */
osThreadId_t BMP280TaskHandle;
uint32_t BMP280TaskBuffer[ 512 ];
osStaticThreadDef_t BMP280TaskControlBlock;
const osThreadAttr_t BMP280Task_attributes = {
  .name = "BMP280Task",
  .cb_mem = &BMP280TaskControlBlock,
  .cb_size = sizeof(BMP280TaskControlBlock),
  .stack_mem = &BMP280TaskBuffer[0],
  .stack_size = sizeof(BMP280TaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for myMutex01 */
osMutexId_t myMutex01Handle;
osStaticMutexDef_t myMutex01ControlBlock;
const osMutexAttr_t myMutex01_attributes = {
  .name = "myMutex01",
  .cb_mem = &myMutex01ControlBlock,
  .cb_size = sizeof(myMutex01ControlBlock),
};
/* Definitions for myBinarySem01 */
osSemaphoreId_t myBinarySem01Handle;
osStaticSemaphoreDef_t myBinarySem01ControlBlock;
const osSemaphoreAttr_t myBinarySem01_attributes = {
  .name = "myBinarySem01",
  .cb_mem = &myBinarySem01ControlBlock,
  .cb_size = sizeof(myBinarySem01ControlBlock),
};
/* Definitions for myEvent01 */
osEventFlagsId_t myEvent01Handle;
osStaticEventGroupDef_t myEvent01ControlBlock;
const osEventFlagsAttr_t myEvent01_attributes = {
  .name = "myEvent01",
  .cb_mem = &myEvent01ControlBlock,
  .cb_size = sizeof(myEvent01ControlBlock),
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_CRC_Init(void);
static void MX_DAC_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_RNG_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_IWDG_Init(void);
void StartTaskKeyRead(void *argument);
void StartTaskUartRead(void *argument);
void StartTaskADC(void *argument);
void StartTaskReadBMP280(void *argument);

/* USER CODE BEGIN PFP */
// область для объявления приватных функций
void command_ch(char);
void command_str(char*);
void command_bin(char*);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Реализация абстрактной функции для передачи данных через printf
//int fputc(int ch, FILE *f){ // вроде её используем если C++, иначе __io_putchar
int __io_putchar(int ch){
	ITM_SendChar((uint32_t) ch); // блокирующий режим встроен
	while (HAL_UART_Transmit(&huart2, (unsigned char*) &ch, 1, 10) == HAL_BUSY); // может вернуть HAL_ERROR, HAL_BUSY
	// если хотим слать в USB CDC
	//while( CDC_Transmit_FS((uint8_t *)&ch, 1) == USBD_BUSY ); // USBD_FAIL , USBD_BUSY
	return ch;
}

// ещё способ передачи
void USART2_putch(char c) {
	//ожидание готовности
	while (!(USART2->SR & USART_SR_TXE));
	USART2->DR = c;
}
void USART2_sendstr(const char *s) {
	while (*s)
		USART2_putch(*s++);
}

uint16_t adc[4] = {0};

char bufLCDstr[100] = { 0 };

/* callback на завершение преобразования ADC */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
	if (hadc->Instance == ADC1) {
		//HAL_ADC_Stop_DMA(hadc);
		float vdd = (1 * 1.2f * 4096) / adc[3]; // если внутреннее опорное ровно 1.2В
		//printf("callback ADC: %4d, Tout= %4d (%.1f C), vdd= %4d (%.3f V), Tint= %4d\r\n", adc[0], adc[1], TMP36convertToTemperature(adc[1]), adc[2], vdd, adc[3]);
		printf("ADC: A0:%4d, A1:%4d, Tint=%4d, vdd=%4d (%.3f V)\r\n", adc[0], adc[1], adc[2], adc[3], vdd);
		//HAL_Delay(1);
		//HAL_ADC_Start_DMA(hadc, (uint32_t*)adcDMA, 4); // нужно перезапускать если не континуос режим и запуск не от таймера

		// LCD print
		ColorInk=BLACK;
		ColorBack=WHITE;
		//memset(bufLCDstr, 0, 100);
		sprintf(bufLCDstr, " Vdd:  %.3f V\n ADC1: %4d\n ADC2: %4d", vdd, adc[0], adc[1]);
		printstr_lcd(bufLCDstr, 0, 0, 1, 1);

#ifdef INC_MT6701_H_
		//task5_MT6701();
		uint16_t rez_raw = mt6701_read();
		float deg = rez_raw * (360.0f / 16384.0f);
		printf(" MT6701 = raw: %4d, deg: %.3f\r\n", rez_raw, deg);
#endif
		//memset(bufLCDstr, 0, 100);
		//sprintf(bufLCDstr, " MT6701: %4d\n deg: %.3f", rez_raw, deg);
		//printstr_lcd(bufLCDstr, 0, 4, 1, 1);

		//printchr_lcd('*', 0, 0, 1, 1);
		//lcd_prn(bufLCDstr);
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	printf("GPIO_EXTI_callback %d\r\n", GPIO_Pin);
}

//TaskInitTypeDef* t_BMP280;

/*--------------------------- UART RX ----------------------------------------*/
// ver 1.1
// Драйвер для приёма посылок через UART в асинхронном режиме.
// При получении данных ждём таймаут после которого понимаем что пора обработать полученное.
// Можно развить до следующего слоя абстрации, чтобы обрабатывать по счётчику принятых данных,
// или по команде переданной в начале фрейма.
// 11-12мсек на 1024 символа при 1млн бод = 0.68-0.74% эффективность канала
// RX_BUF_LEN * 8/(0.7 * bod) * 1.2 = X секунд для заполнения буфера с запасом 20%
// сократив выражение для 1млн бод Ttransmit (мсек) = RX_BUF_LEN * 0,014

//#define UART_DEBUG // закомментировать если не нужна отладка приёма UART
#define UART &huart2
#define HDMA_MEM2MEM &hdma_memtomem_dma2_stream1
#define RX_BUF_LEN 1024  //для DMA делать кратным uint32 - 4 байта
#define TICK_WAIT_RX 50  //3.5 ms для 256 byte
uint8_t chRx = 0;
char bufRx1[RX_BUF_LEN] = { 0 };
char bufRx2[RX_BUF_LEN] = { 0 };
volatile uint16_t idx = 0;
volatile uint16_t lenRxData = 0;
volatile uint8_t isFull = 0; // флаг переполнения
uint8_t isReciveCmplete = 0; // флаг приёма перевода строки
uint32_t UARTtimeout = 0;

// для отладочной информации, меньше задержек, точнее соблюдается порядок сообщений из прерываний
#define _PRNFAST(s) HAL_UART_Transmit_IT(UART, (uint8_t*)s, sizeof(s)-1); HAL_Delay(1);

uint8_t isDMA = 0;
#if defined(STM32F407xx)
uint16_t maxlen = 500;
#else
uint16_t maxlen = 100;
#endif

/* Определил на практике следующие размеры буфера, которые может переварить данное ядро без пропуска данных.
 * f103 memcpy maxlen bytes 108 - 112 зависает, 104 справлятся, в DMA режиме - 512 справляется, чуть выше зависает
 * f407ve memcpy 524 байт виснет, 520 работает, DMA- 1856 работает, 1860 виснет
 * */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	//while (HAL_UART_Transmit(&huart1, (unsigned char*) bufRx, strlen(bufRx), 30) == HAL_BUSY); // для примера
	UARTtimeout = HAL_GetTick(); // сбрасываем таймаут приёма по которому начнём парсинг
	// доп проверка нужна чтобы избежать выхода за верхнюю границу на случай заполнения обоих буферв
	if (maxlen > RX_BUF_LEN)
		maxlen = RX_BUF_LEN;
	if (idx < maxlen - 1)
		bufRx1[idx++] = chRx;

	// TO DO: здесь можно добавить проверку:
	// если первый символ например указывает что будут бинарные данные, то принять байт длинны посылки и выполнять внешний код при заполнении
	// или если строковая то ждём /r /n.
	if (chRx == '\n' && bufRx1[0] != '*') {
		isReciveCmplete = 1; // начнётся обработка строки при следующем запуске task4_uartRx() не дожидаясь таймаута
	}

	/* Если второй буфер и так полный и первый тоже заполнился, то прерывания продолжаем,
	 * но новые данные не сохраняем (будут потери).
	 * �?наче перебросим данные из первого во второй и выставим флаг переполнения.
	 * memcpy не успевает скопировать от 100 до 120 байт без пропуска следующего символа на 1млн бод
	 * Получаем скорость копирования около 11МГц. volatile не спасает, как и оптимизация.
	 * Можно попробовать ускорить с помощью DMA, либо использовать блинный буфер,
	 * который точно не переполнится при самой длинной команде или данных. */
	if ((isFull == 0) && (idx >= maxlen - 1)) {	//if ((chRx[0] == '\n') || (idx >= RX_BUF_LEN - 1 - 0)) {
		bufRx1[maxlen - 1] = 0; // '\0' символ, если вдруг буду использовать как string
		if (isDMA == 0) {
			memcpy(bufRx2, bufRx1, maxlen); // strcpy не подходит тк данные могут быть бинарными
		} else {
			HAL_DMA_Start(HDMA_MEM2MEM, (uint32_t) bufRx1, (uint32_t) bufRx2, maxlen / 4);
			HAL_DMA_PollForTransfer(HDMA_MEM2MEM, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
		}
		idx = 0;
		lenRxData = maxlen;
		isFull = 1;
		//_PRNFAST("_ENDF_\r\n");
	}
	// запрашиваем следующий байт данных из порта через прерывание
	HAL_UART_Receive_IT(UART, &chRx, 1);
}

/*
 * Полингом проверяем есть ли флаг переполнения входного буфера,
 * а так же был ли таймаут приёма.
 * TO DO: желательно начинать парсинг данных не по таймауту, а по сигнатуре конца посылки,
 * например для строк и символов это окончание строки \r или \n.
 * Для бинарных иной вариант, например фиксированная длинна, или длинна указанная в первом байте.
 */
void task4_uartRx() {
	// был переполнен входной буфер, данные уже лежат в bufRx2
	// нежелательное событие, так как данные либо не были вовремя обработаны и
	// скопилась очередь, либо пришла команда длинее чем входной буфер.
	if (isFull != 0) {
		_PRNFAST("_FULL_BUF_\r\n");
#ifdef UART_DEBUG
		HAL_StatusTypeDef stat = HAL_UART_Transmit_IT(UART, (uint8_t*) bufRx2, maxlen-1);
		HAL_Delay(TICK_WAIT_RX);
		if (stat != HAL_OK) {
			_PRNFAST("*");
		}
		_PRNFAST("_ENDF_\r\n");
#endif
		// тут должна быть обработка данных в буфере
		isFull = 0; // не сбрасываем пока до конца не обработали данные буфера
	}
	// данные в bufRx1, таймаут TICK_WAIT_RX мс
	if (idx > 0) {
		if (isReciveCmplete || (HAL_GetTick() > UARTtimeout + TICK_WAIT_RX)) {
			/*uint16_t n = idx;
			idx = 0; // как можно быстрее готовим к приёму данных
			memcpy(bufRx2, bufRx1, n);*/

			memcpy(bufRx2, bufRx1, idx);
			bufRx2[idx] = 0; // конец строки

#ifdef UART_DEBUG
			_PRNFAST("_T_");
			HAL_StatusTypeDef stat = HAL_UART_Transmit_IT(UART, (uint8_t*) bufRx2, idx);
			HAL_Delay(TICK_WAIT_RX);
			if (stat != HAL_OK) {
				_PRNFAST("!");
			}
			_PRNFAST("_END_T_\r\n");
#endif
			lenRxData = idx;
			idx = 0;
			isFull = 0;
			isReciveCmplete = 0;
			//UARTtimeout = HAL_GetTick();
			//char tmpbuf[20];
			//itoa((UARTtimeout - HAL_GetTick()), tmpbuf);
			printf("timeout: %d\r\n", (int) (HAL_GetTick() - UARTtimeout));

			/* TO DO: добавить семафор, чтобы не копировались данные из bufRx1 в bufRx2
			 * пока не завершилась обработка данных из буфера		 */

			// разделяем строковые (начинаются с '_'), бинарные (начинаются с '*') и односимвольные команды
			if (bufRx2[0] == '*') { //первый символ '*' значит бинарный поток данных
				command_bin(bufRx2);
			} else if (bufRx2[0] != 0 && bufRx2[0] != '\r' && bufRx2[0] != '\n') { // первый символ не конец сообщения
				if (bufRx2[1] == 0 || bufRx2[1] == '\r' || bufRx2[1] == '\n') {    // второй символ - конец сообщения
					command_ch(*bufRx2);  // значит приняли односимвольную команду
				} else
					command_str(bufRx2);  // иначе приняли строковую команду
			}

			/* if (*bufRx2 == '_') {
				command_str(bufRx2 + 1); // пропустим первый символ '_'
			} else if (*bufRx2 == '*') {
				command_bin(bufRx2 + 1); // пропустим первый символ '*'
			} else {
				command_ch(*bufRx2);
			} */
		}
	}
}

/* парсим бинарные команды
 * принять бинарный поток данных без символа \n для управления адресной светодиодной лентой
 * message format: ['*'][idx_led (1 byte)][size data (2 bytes)][data r,g,b (3*n bytes)]
 */
void command_bin(char *str) {
	USART2_sendstr("Recive bin\r\n");

	/* uint16_t len, count, i;
	uint16_t n;
	len = 0;
	count = 0;
	n = 0;
	i = 1; // пропустим первый символ '*'
	if (lenRxData >= 3) {
		n = (uint8_t) str[i++];
		len = ((uint8_t) str[i++] << 8) | (uint8_t) str[i++];
	}
	while (i < lenRxData && count < len) {
		count += 3;
		// count += Serial.readBytes(cmdStr, 3);
		if (n <= strip.numPixels()) //NUM_LEDS  strip.numPixels() ->uint16_t
			strip.setPixelColor(n++, str[i++], str[i++], str[i++]); //(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
	}
	*/
}

// парсим строковые команды
// _XXX

 /* TODO: может вместо inline и strlen использовать макрос заменяющий следующее:
 sizeof вычисляется компилятором, strlen же занимает процессорное время.
 strncmp(cmdStr, "del", sizeof("del")-1) == 0
 static inline bool  //байт 61844  -> 61828  ->  61812 -> 61796 (замена всех на compCmd2)
#define CMPCMD(c) (strncmp(cmdStr, (c), sizeof((c))-1) == 0)
static inline bool compCmd2(const char* cmd){
  return strncmp(bufRx2, cmd, strlen(cmd)) == 0;
}
if (strncmp(token, "_i2c", strlen("_i2c")) == 0) {;}
Если сравнивать подстроку то пользуемся
int strncmp(const char *str1, const char *str2, size_t n).
Если сравниваем строки учитывая их длины то
int strcmp(const char *str1, const char *str2);

if (compCmd(token, "test")) {}
*/
static inline bool compCmd(char* str, const char* cmd){
  return strncmp(str, cmd, strlen(cmd)) == 0;
}

bool nextArg(char **token, uint8_t len, uint8_t isHex) {
	*token = strtok(NULL, " ");
	//printf("nextArg: %s\r\n", *token);
	if (*token == NULL) {
		printf("arg is NULL\r\n");
		return false;
	} else if (strlen(*token) != len) {
		printf("arg len != %d, token: \'%s\'\r\n", len, *token);
		return false;
	} else if (isHex == 1) {
		for (uint8_t i = 0; i < len; ++i) {
			char ch = (*token)[i];
			if (!isxdigit(ch)) { // проверяет, является ли c шестнадцатеричной цифрой
				printf("arg not hex: \'%s\'\r\n", *token);
				return false;
			}
		}
	}
	return true;
}

/** Команды для управления переферией через UART
_i2c bus_addr7_hex  'w/r' len_addr[0..2] reg_addr16_hex len_data16_hex *data_hex
_i2c 52 w 1 0010 0003 0A1B2C
_i2c 52 r 1 0000 0020
_i2c 52 r 0 0000 0010

_i2c 57 w 2 0010 0003 0A1B2C
_i2c 57 r 2 0000 00ff

DS3231
_i2c 68 r 1 0000 0013                       DS3231 read all registers
_i2c 68 w 1 0000 0007 00000000000000        DS3231 clear datetime
_i2c 68 w 1 0000 0007 00521800030423        DS3231 write datetime dt[0:6] c:m:h:00:d:m:y
_i2c 68 w 1 0000 0003 005218                DS3231 write time dt[0:2] c:m:h

INA226
_i2c 40 r 1 0001 0002   INA226 SHUNT_VOLTAGE  sign16 (1 LSB = 2.5 uV) R=0.1 Om
_i2c 40 r 1 0002 0002   INA226 BUS_VOLTAGE  unsign16 (1 LSB = 1.25 mV)

#define I2C_ADDR_24C02  0x52  // AT24C02, (A0 & A2 = GND, A1 = Vcc) I2C address: 0x52 = b1010010 = dec: 82
#define I2C_ADDR_24C32  0x57  // AT24C32,I2C address 0x57, bin: 01010111, dec: 87  (4096 x 8) page 32 bytes
#define I2C_ADDR_INA226 0x40  // INA226, I2C address 0x40, bin: 01000000
#define I2C_ADDR_OLED   0x3C  // OLED,   I2C address 0x3C, bin: 00111100, dec: 60
#define I2C_ADDR_DS3231 0x68  // RTC_DS3231, address 0x68, bin: 01101000, dec: 104
*/
void command_str(char *str) {
	//str++; // пропустим первый символ '_'
	USART2_sendstr("Recive: \'");
	USART2_sendstr(str); // echo
	USART2_sendstr("\'\r\n");
	char *token; // указатель на подстроку в str. Строка str изменяется командой strtok!!!
	token = strtok(str, " "); // get command
	// TO DO: не понимает команды если в конце не добавить пробел

	if (token == NULL) {
		printf("cmd token is NULL. str=%s\r\n", str);
		return;
	} else {
		printf("cmd token: \'%s\'\r\n", token);
	}

	if (strcmp(token, "test") == 0) {
		uint32_t startTime = HAL_GetTick();
		printf("cmd: test\r\ntime(ms): %d\r\n", (int) (HAL_GetTick() - startTime)); //uint32_t endTime = HAL_GetTick();
	} else if (strcmp(token, "task") == 0) {
		token = strtok(NULL, " ");
		if (token == NULL) {
			printf("task arg is NULL\r\n");
			return;
		}
		if (!strcmp(token, "bmp280")) {
			if (!nextArg(&token, 1, 0))
				return;
			char cmd = *token;
			printf("cmd arg: \'%c\'\r\n", cmd);
			//printf("cmd not exist. token: \'%s\'\r\n", token);

			switch (cmd) {
			case 'p':
				printf("cmd pause task change\r\n");
				//pause();
				printf("Task BMP280 Run/Stop\r\n");
//				if (t_BMP280 != NULL && t_BMP280->task != NULL) {
//					t_BMP280->pause = (t_BMP280->pause) ? 0 : 1;
//				}
				break;
			case 't':
				token = strtok(NULL, " ");
//				uint32_t period = (uint32_t) atoi(token);
//				if (t_BMP280 != NULL && t_BMP280->task != NULL && period > 0) {
//					t_BMP280->period = period;
//				}
				break;
			default:
				break;
			}
		} else {
			printf("task name not exist\r\n");
			return;
		}
	} else if (strcmp(token, "i2c") == 0) {
		uint32_t startTime = HAL_GetTick(); // micros(); //millis();

		if(!nextArg(&token, 2, 1)) return;
		uint8_t busaddr = (hex2byte(token[0]) << 4) | hex2byte(token[1]); // адрес микросхемы
		printf("arg1 bus_addr: %d, 0x%02x\r\n", busaddr, busaddr);

		if(!nextArg(&token, 1, 0)) return;
		char wr = *token;
		printf("arg2 wr: \'%c\'\r\n", wr);

		if(!nextArg(&token, 1, 0)) return;
		uint8_t memaddrsize = (uint8_t) atoi(token); //memaddrsize=0,1,2
		printf("arg3 mem addr size: %d\r\n", memaddrsize);

		if(!nextArg(&token, 4, 1)) return; // адрес регистра
		//uint16_t memaddr = hex2ul(token, 4);
		uint16_t memaddr = (hex2byte(token[0]) << 12) | (hex2byte(token[1]) << 8)
				| (hex2byte(token[2]) << 4) | hex2byte(token[3]);
		printf("arg4 addr reg: %d, 0x%04x\r\n", memaddr, memaddr);

		//if(!nextArg(&token, 4, 1)) return; // пока так, нужно сделать чтобы не искал \0
		token = strtok(NULL, " ");
		uint16_t len_data = hex2ul(token, 4);
		printf("arg5 len_data: %d, 0x%04x\r\n", len_data, len_data);

		/* write */
		if ((wr == 'w') || (wr == 'W')) {
			token += 5; // с учётом длины предыдущего параметра len_data
			uint8_t buffW[len_data];
			for (int i = 0; i < len_data; ++i) {
				char chH = *token++;
				/* isspace(c): проверяет, является ли c пробелом (' '),
				 * символом перевода строки ('\n'), возвратом каретки ('\r'),
				 * перевод страницы ('\f'), горизонтальная ('\t') или вертикальная ('\v') табуляция
				 *
				 * isxdigit(c) - проверяет, является ли c шестнадцатеричной цифрой
				 * if ((chH == '\0') || (chH == '\r') || (chH == '\n') || (chH == ' '))
				 * */
				if (!isxdigit(chH)) {
					printf("Error: данных меньше чем указывалось. len_data: %d, i: %d\r\n",
							len_data, i);
					print_mem_8(buffW, len_data);
					return;
				}
				char chL = *token++;
				if (!isxdigit(chL)) {
					printf("Error: данных меньше чем указывалось. len_data: %d, i: %d\r\n",
							len_data, i);
					print_mem_8(buffW, len_data);
					return;
				}
				buffW[i] = (hex2byte(chH) << 4) | hex2byte(chL);
			}

//			if (len_data != 1) {
//				printf("Error: arg5 len_data != 1\r\n");
//				return;
//			}
//			if (!nextArg(&token, 2)) return;
//			uint8_t data = (hex2int(token[0]) << 4) | hex2int(token[1]);
//			printf("arg data: %d, 0x%02x\r\n", data, data);

			// типы данных для STM32 (unsigned int) aka (uint32_t) aka (unsigned long)
			uint32_t crc = ~0L; // crc Init  : 0xFFFFFFFF
			printf("data1: [");
			for (int i = 0; i < len_data; i++) {
				printf("%02X ", buffW[i]);
				crc32stream(buffW[i], &crc);
			}
			printf("]\r\nCRC START\r\n%x\r\nCRC END\r\n", (unsigned int)crc);

			printf("\r\ndata2: [");
			print_mem_8(buffW, len_data);
			uint32_t crc32 = crc32buf(buffW, len_data);
			printf("]\r\nCRC START\r\n%x\r\nCRC END\r\n", (unsigned int)crc32);

			if (len_data == 1)
				printf("\r\nWrite to I2C: [dev] 0x%02x -> [mem] 0x%02x = [data] 0x%02x\r\n",
						busaddr, memaddr, buffW[0]);
			else
				printf("\r\nWrite to I2C: [dev] 0x%02x -> [mem] 0x%02x..0x%02x\r\n",
						busaddr, memaddr, memaddr + len_data);

			if (memaddrsize == 1) {
				HAL_I2C_Mem_Write(H_I2C, busaddr << 1, memaddr, I2C_MEMADD_SIZE_8BIT,  buffW, len_data, 100);
			} else if (memaddrsize == 2) {
				HAL_I2C_Mem_Write(H_I2C, busaddr << 1, memaddr, I2C_MEMADD_SIZE_16BIT, buffW, len_data, 100);
			} else {
				printf("Error: memaddrsize not 1,2: %d", memaddrsize);
			}

			// TO DO: убрать после отладки BMP280
			if (busaddr == 0x76) {
				//bmp280_write(memaddr, data);
				HAL_Delay(100);
				BMP280_Read_All();
			}
		}
		/* read */	// i2c 76 r 1 0000 0100    i2c 76 r 1 00f5 0001
		else if ((wr == 'r') || (wr == 'R')) {
			if (len_data == 0) {
				printf("Error: len_data = 0\r\n");
				return;
			}
			uint8_t readbuf[len_data];

			if (memaddrsize == 0) { // принять по i2c не указывая адрес ячейки, полезно при чтении станицами и для некоторых микросхем
				HAL_StatusTypeDef st = HAL_I2C_Master_Receive(H_I2C, busaddr << 1, readbuf, len_data, 200); // HAL_MAX_DELAY
				if (st != HAL_OK) {
					printf("Error: HAL status HAL_I2C_Master_Receive = %d", st);
					return;
				}
			} else if (memaddrsize == 1) {
				//HAL_I2C_Mem_Write(H_I2C, busaddr << 1, memaddr, I2C_MEMADD_SIZE_8BIT,  buffW, len_data, 100);
				HAL_StatusTypeDef st = HAL_I2C_Mem_Read(H_I2C, busaddr << 1, memaddr, I2C_MEMADD_SIZE_8BIT, readbuf, len_data, 200);
				if (st != HAL_OK) {
					printf("Error: HAL status I2C_Mem_Read = %d", st);
					return;
				}
			} else if (memaddrsize == 2) {
				HAL_StatusTypeDef st = HAL_I2C_Mem_Read(H_I2C, busaddr << 1, memaddr, I2C_MEMADD_SIZE_16BIT, readbuf, len_data, 200);
				if (st != HAL_OK) {
					printf("Error: HAL status I2C_Mem_Read = %d", st);
					return;
				}
			} else {
				printf("Error: memaddrsize not 0,1,2: = %d", memaddrsize);
				return;
			}

			unsigned long crc = crc32buf(readbuf, len_data);
			printf("DATA START");
			print_mem_8(readbuf, len_data);
	        printf("DATA END\r\nCRC START\r\n%x\r\nCRC END\r\n", (unsigned int)crc);
		}

		uint32_t endTime = HAL_GetTick();
		//printf("\r\nWrite time(ms): %d\r\n", (int)(endTime - startTime));
		printf("Time(ms): %d\r\n\r\n", (int)(endTime - startTime));
	} else if (strcmp(token, "i2c2") == 0) {
		printf("cmd: i2c2\r\n");
	} else {
		printf("cmd not exist. token: \'%s\'\r\n", token);
		return;
	}
/* 	uint8_t writebuff[len_data];
	uint8_t id_wr_buff = 0;
	while (id_wr_buff < len_data) {
		if (ix + 1 >= n) {
			printf("cmd: i2c error read data string"); //длинна переданных данных меньше указанных
			break;
		}
		writebuff[id_wr_buff++] = (hex2int(cmdStr[ix++]) << 4)
				| hex2int(cmdStr[ix++]);
	}

	uint16 i = 0;
	byte k; // количество байт до конца страницы с указанного адреса
	while (i < len_data) {
		// каждую страницу записывать инициализируя обмен с начала
		_TW.beginTransmission(bus_addr);
		if (len_addr == 2)
			_TW.write((uint8_t) ((addr + i) >> 8));
		_TW.write((uint8_t) (addr + i));

		k = 8 - (addr + i) % 8; // количество байт до конца страницы с указанногго адреса
		k = (len_data - i < k) ? len_data - i : k; // запишем правильное количество оставшихся байт даже если не доходим до границы страницы
		int n = _TW.write(writebuff + i, k); //Wire : WireBase #define BUFFER_LENGTH 32
		_TW.endTransmission(); // отправляем на устройство по шшине i2c
		delay(15);
		i += k;
	}

	pin = (uint8) atoi(token); // uint8_t pin_num = atoi(token);
	Serial << "pin '" << pin << "'" << endl;
	token = strtok(NULL, " "); // arg2 WiringPinMode
	if (token != NULL && pin >= 0) {
		if (!strcmp(token, "PM") || !strcmp(token, "pinMode")) {
			token = strtok(NULL, " "); // arg3 WiringPinMode
			if (token != NULL) {
				pinmode = (WiringPinMode) atoi(token);
				Serial << "pinMode '" << pinmode << "'" << endl;
				if (pinmode >= 0) {
					pinMode(pin, pinmode);
				}
			}
		} else if (!strcmp(token, "DW") || !strcmp(token, "digitalWrite")) {
			token = strtok(NULL, " "); // arg2 WiringPinMode
			if (token != NULL) {
				value = atoi(token); // (uint8)atoi(token);
				Serial << "digitalWrite '" << value << "'" << endl;
				digitalWrite(pin, (uint8) value);
			}
		}
	} */
}

// парсим односимвольные команды
void command_ch(char c) {
	/*if (c == 't') testspeedcopy(); */
	switch (c) {
	case 'c':
		printf("Test LCD LPH8731\r\n");
		Lcd_test();
		break;
	case 'r':
#ifdef INC_MT6701_H_
		printf("mt6701 read\r\n");
		mt6701_read_test();
#endif
#ifdef INC_BMP280_H_
		printf("BMP280 read\r\n");
		BMP280_Read_All();
#endif
		break;
	case 'R':
#ifdef INC_BMP280_H_
		printf("BMP280 read press and temp\r\n");
		BMP280_Read_PT();
#endif
		break;
	case 't':
		printf("Test speed data copy\r\n");
#ifdef INC_TESTSPEEDCOPY_H_
		testspeedcopy();
#endif
		break;
	case 'h':
		printf("ATH25 Test\r\n");
#ifdef INC_ATH25_H_
		ATH25_test();
#endif
		break;
	case 's':
		printf("scan I2C\r\n");
		scanI2C();
		break;
	case 'q':
		printf("TIM_OC_Stop\r\n");
		//HAL_TIM_OC_Start(&htim4, TIM_CHANNEL_4); // запускаем таймер
		HAL_TIM_OC_Stop(&htim4, TIM_CHANNEL_4);
		break;
	case 'Q':
		printf("Task BMP280 Run/Stop\r\n");
//		для приостановки задач нужно подключить заголовочный файл task.h
//		и определить макро INCLUDE_vTaskSuspend в значение 1
//		vTaskSuspend( xHandle );
//		vTaskResume( xHandle );

//		if (t_BMP280 != NULL && t_BMP280->task != NULL) {
//			t_BMP280->pause = (t_BMP280->pause) ? 0 : 1;
//		}
		break;
	case 'd':
		isDMA = !isDMA;
#if defined(STM32F407xx)
		maxlen = (isDMA == 0) ? 520 : 1856;
#else
		maxlen = (isDMA == 0) ? 100 : 512;
#endif
		printf("Set isDMA %d, maxlen %d\r\n", isDMA, maxlen);
		break;
	case 'u':
		maxlen += 4;
		if (maxlen > RX_BUF_LEN)
			maxlen = RX_BUF_LEN;
		printf("maxlen up %d\r\n", maxlen);
		break;
	case 'y':
		if (maxlen >= 4)
			maxlen -= 4;
		printf("maxlen dw %d\r\n", maxlen);
		break;
	case 'U':
		maxlen += 4*10;
		if (maxlen > RX_BUF_LEN)
			maxlen = RX_BUF_LEN;
		printf("maxlen up %d\r\n", maxlen);
		break;
	case 'Y':
		if (maxlen >= 4*10)
			maxlen -= 4*10;
		printf("maxlen dw %d\r\n", maxlen);
		break;
	case 'e':
		printf("Read EEPROM 24C02\r\n");
		//eeprom_readall();
		break;
	case '1':
		printf("Command: Keyboard_test\r\n");
		uint16_t kb = Keyboard_read();
		printf(" %x\r\n", kb);
		Keyboard_test(kb);
		break;
	case '2':
		printf("Command 2\r\n");
		break;
	case '3':
		_PRNFAST("Command 3\r\n");
//		unsigned portBASE_TYPE uxTaskGetStackHighWaterMark( xTaskHandle xTask );
		printf("MinFreeStack BMP280TaskHandle %d\r\n", (uint16_t)uxTaskGetStackHighWaterMark(BMP280TaskHandle));
		printf("MinFreeStack ADC1ReadTaskHandle %d\r\n", (uint16_t)uxTaskGetStackHighWaterMark(ADC1ReadTaskHandle));
		printf("MinFreeStack uartReadTaskHandle %d\r\n", (uint16_t)uxTaskGetStackHighWaterMark(uartReadTaskHandle));
		printf("MinFreeStack keyReadTaskHandle %d\r\n", (uint16_t)uxTaskGetStackHighWaterMark(keyReadTaskHandle));
		break;
	default:
		break;
	}
}

/*----------------------------------------------------------------------------*/
/* stepper motor driver
STM32CubeIDE Settings:
Clock Configuration tab -> HCLK (MHz) to 72
Timer -> TIM1 -> Clock Source set to Internal Clock -> Prescaler set to 72-1
void microDelay (uint16_t delay)
{
  __HAL_TIM_SET_COUNTER(&htim1, 0);
  while (__HAL_TIM_GET_COUNTER(&htim1) < delay);
}

void step (int steps, uint8_t direction, uint16_t delay)
{
  int x;
  if (direction == 0)
    HAL_GPIO_WritePin(DIR_PORT, DIR_PIN, GPIO_PIN_SET);
  else
    HAL_GPIO_WritePin(DIR_PORT, DIR_PIN, GPIO_PIN_RESET);
  for(x=0; x<steps; x=x+1)
  {
    HAL_GPIO_WritePin(STEP_PORT, STEP_PIN, GPIO_PIN_SET);
    microDelay(delay);
    HAL_GPIO_WritePin(STEP_PORT, STEP_PIN, GPIO_PIN_RESET);
    microDelay(delay);
  }
}
step(800, 1, 5000); */

/*----------------------------------------------------------------------------*/
// задача по опросу клавиатуры и выполнению команд
void task3_KB() {
	uint16_t kb = Keyboard_read();
	Keyboard_test(kb);
	if (kb & KB_LEFT)  {
		printf("Scanning I2C buses\r\n");
		scanI2C();

		printf("Lcd test\r\n");
		Lcd_test();
	}
	if (kb & KB_RIGHT)  {
		/*HAL_ADC_Stop_DMA(&hadc1);
		printf("Stop_DMA\r\n");*/

		/*TIM4->ARR = 300;
		TIM4->EGR = TIM_EGR_UG;
		printf("Set TIM4 period 500 ms\r\n");*/
	}
	if (kb & KB_SET)  {
		isDMA = !isDMA;
#if defined(STM32F407xx)
		maxlen = (isDMA == 0) ? 520 : 1856;
#else
		maxlen = (isDMA == 0) ? 100 : 512;
#endif
		printf("Set isDMA %d, maxlen %d\r\n", isDMA, maxlen);
	}
	if (kb & KB_UP) {
		maxlen += 4;
		if (maxlen > RX_BUF_LEN)
			maxlen = RX_BUF_LEN;
		printf("maxlen up %d\r\n", maxlen);
	}
	if (kb & KB_DOWN) {
		if (maxlen >= 4)
			maxlen -= 4;
		printf("maxlen dw %d\r\n", maxlen);
	}
	if (kb & KB_MID) {
#ifdef INC_BMP280_H_
		printf("BMP280 read\r\n");
		BMP280_Read_All();
#endif
	}
}

void task5_MT6701() {
#ifdef INC_MT6701_H_
	uint16_t rez_raw = mt6701_read();
	float deg = rez_raw * (360.0f / 16384.0f);
	//printf("MT6701 = raw: %4d, deg: %.3f", rez_raw, deg);

	//memset(bufLCDstr, 0, 100);
	sprintf(bufLCDstr, " MT6701: %4d\n deg: %.3f   \n               ", rez_raw, deg);
	printstr_lcd(bufLCDstr, 0, 4, 1, 1);
#endif
}

// Задача по опросу BMP280 и ATH25 и выводу результатов в терминал и на LCD
void read_BMP280_ATH25() {
#ifdef INC_BMP280_H_
	float temperature, pressure;
	//BMP280_Read_PT();
//	taskENTER_CRITICAL();
	BMP280_Read_Data(&temperature, &pressure);
//	taskEXIT_CRITICAL();
//	taskYIELD();
#ifdef INC_ATH25_H_
	//ATH25_init();
	float ATH25_temperature, ATH25_humidity;
	HAL_StatusTypeDef status;

//	taskENTER_CRITICAL();
	status = ATH25_Read_Data(&ATH25_temperature, &ATH25_humidity);
//	taskEXIT_CRITICAL();
	if (status == HAL_OK) {
		printf("T: %.2f; P1: %.2f; P2: %.4f; H: %.2f; T2: %.2f\r\n",
				temperature, pressure, pressure/mmHg, ATH25_humidity, ATH25_temperature);
	} else {
		printf("T: %.2f; P1: %.2f; P2: %.4f;\r\n",
				temperature, pressure, pressure / mmHg);
	}
#else
	printf("T: %.2f; P1: %.2f; P2: %.4f;\r\n",
			temperature, pressure, pressure/mmHg);
	//printf("%.2f; %.2f; %.4f;\r\n", temperature, pressure, pressure/mmHg);
#endif

	//uint16_t rez_raw = mt6701_read();
	//float deg = rez_raw * (360.0f / 16384.0f);
	//printf("MT6701 = raw: %4d, deg: %.3f", rez_raw, deg);

	// LCD print
	//uint16_t lastInc = ColorInk;
	uint16_t lastBack = ColorBack;
	ColorInk=RED;
	ColorBack=WHITE;
	//memset(bufLCDstr, 0, 100);
#ifdef INC_ATH25_H_
	sprintf(bufLCDstr, "T: %.2f %d \nP: %.3f mmhg\nH: %.2f T2:%.2f", temperature, (int)pressure, pressure/mmHg, ATH25_humidity, ATH25_temperature);
#else
	sprintf(bufLCDstr, "T: %.2f %d \nP: %.3f mmhg", temperature, (int)pressure, pressure/mmHg);
#endif
	printstr_lcd(bufLCDstr, 0, 4, 1, 1);
	//ColorInk=lastInc;
	ColorBack=lastBack;
	ColorInk=BLACK;
#endif
}

void User_Init() {
#if defined(STM32F103xB)
	printf("Run on controller 'STM32F103xB'\r\n");
#elif defined(STM32F407xx)
	printf("Run on controller 'STM32F407xx'\r\n");
#else
	printf("Run on controller 'uncnown'\r\n");
#endif

	InitLCD();

	//HAL_ADCEx_Calibration_Start(&hadc1);

	HAL_ADC_Start_DMA(&hadc1, (uint32_t*) adc, _SIZE_ARRAY(adc));
	/* меняем настройки прескалера у TIM4 */
	htim4.Init.Prescaler = 42000 - 1; // 36 MHz -> 1 KHz
	htim4.Init.Period = 5000; // msec
	TIM_Base_SetConfig(htim4.Instance, &htim4.Init); /* Set the Time Base configuration */
	HAL_TIM_OC_Start(&htim4, TIM_CHANNEL_4); // запускаем таймер
	//HAL_TIM_OC_Stop(&htim4, TIM_CHANNEL_4);
	/* настройка прескалера напрямую в регистры */
	//__HAL_TIM_SET_PRESCALER(&htim4, 42000 - 1);
	//TIM4->PSC = (&htim4)->Instance->PSC = htim4.Instance->PSChtim4.Instance->PSC
	//TIM4->PSC = 36000u;  /* Set the Prescaler value */
	//TIM4->ARR = 2000u;     /* Set the Autoreload value */
	//TIM4->EGR = TIM_EGR_UG; /* Generate an update event to reload the Prescaler and the repetition counter value immediately (only for advanced timer) */
	//__HAL_TIM_ENABLE_IT(&htim4, ADC_read_onechannelmode);
	HAL_UART_Receive_IT(UART, &chRx, 1);

#ifdef INC_BMP280_H_
	printf("BMP280 init\r\n");
	bmp280_init();
#endif
#ifdef INC_ATH25_H_
	ATH25_init();
#endif

	//taskAdd(&task1, 30000);    // messeg: _Worked
//	taskAdd(&task2_led, 500);    // LED_Toggle
//	taskAdd(&task3_KB, 20);      // Keyboard_read
//	taskAdd(&task4_uartRx, 5);   // проверка на таймаут приёма UART
//#ifdef INC_MT6701_H_
//	taskAdd(&task5_MT6701, 200); // опрос магнитного энкодера MT6701
//#endif
//#ifdef INC_BMP280_H_
//	t_BMP280 = taskAdd(&task6_BMP280, 3000); // опрос BMP280
//#endif
//#ifdef INC_ATH25_H_
//	taskAdd(&task7_ATH25, 3000);
//#endif

	//Lcd_test();
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

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
  MX_ADC1_Init();
  MX_CRC_Init();
  MX_DAC_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_RNG_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
  User_Init();
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Create the mutex(es) */
  /* creation of myMutex01 */
  myMutex01Handle = osMutexNew(&myMutex01_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of myBinarySem01 */
  myBinarySem01Handle = osSemaphoreNew(1, 0, &myBinarySem01_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of keyReadTask */
  keyReadTaskHandle = osThreadNew(StartTaskKeyRead, NULL, &keyReadTask_attributes);

  /* creation of uartReadTask */
  uartReadTaskHandle = osThreadNew(StartTaskUartRead, NULL, &uartReadTask_attributes);

  /* creation of ADC1ReadTask */
  ADC1ReadTaskHandle = osThreadNew(StartTaskADC, NULL, &ADC1ReadTask_attributes);

  /* creation of BMP280Task */
  BMP280TaskHandle = osThreadNew(StartTaskReadBMP280, NULL, &BMP280Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* creation of myEvent01 */
  myEvent01Handle = osEventFlagsNew(&myEvent01_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T4_CC4;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 4;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_144CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = 3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_VREFINT;
  sConfig.Rank = 4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief DAC Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC_Init(void)
{

  /* USER CODE BEGIN DAC_Init 0 */

  /* USER CODE END DAC_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC_Init 1 */

  /* USER CODE END DAC_Init 1 */

  /** DAC Initialization
  */
  hdac.Instance = DAC;
  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT1 config
  */
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC_Init 2 */

  /* USER CODE END DAC_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
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
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 400000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
  hiwdg.Init.Reload = 4095;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

}

/**
  * @brief RNG Initialization Function
  * @param None
  * @retval None
  */
static void MX_RNG_Init(void)
{

  /* USER CODE BEGIN RNG_Init 0 */

  /* USER CODE END RNG_Init 0 */

  /* USER CODE BEGIN RNG_Init 1 */

  /* USER CODE END RNG_Init 1 */
  hrng.Instance = RNG;
  if (HAL_RNG_Init(&hrng) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RNG_Init 2 */

  /* USER CODE END RNG_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 41999;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 500;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV2;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TOGGLE;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 1000000;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  * Configure DMA for memory to memory transfers
  *   hdma_memtomem_dma2_stream1
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* Configure DMA request hdma_memtomem_dma2_stream1 on DMA2_Stream1 */
  hdma_memtomem_dma2_stream1.Instance = DMA2_Stream1;
  hdma_memtomem_dma2_stream1.Init.Channel = DMA_CHANNEL_0;
  hdma_memtomem_dma2_stream1.Init.Direction = DMA_MEMORY_TO_MEMORY;
  hdma_memtomem_dma2_stream1.Init.PeriphInc = DMA_PINC_ENABLE;
  hdma_memtomem_dma2_stream1.Init.MemInc = DMA_MINC_ENABLE;
  hdma_memtomem_dma2_stream1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma_memtomem_dma2_stream1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdma_memtomem_dma2_stream1.Init.Mode = DMA_NORMAL;
  hdma_memtomem_dma2_stream1.Init.Priority = DMA_PRIORITY_LOW;
  hdma_memtomem_dma2_stream1.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
  hdma_memtomem_dma2_stream1.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
  hdma_memtomem_dma2_stream1.Init.MemBurst = DMA_MBURST_SINGLE;
  hdma_memtomem_dma2_stream1.Init.PeriphBurst = DMA_PBURST_SINGLE;
  if (HAL_DMA_Init(&hdma_memtomem_dma2_stream1) != HAL_OK)
  {
    Error_Handler( );
  }

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, LCD_CS_Pin|LCD_RES_Pin|LCD_RS_Pin|LCD_CLK_Pin
                          |LCD_DAT_Pin|LED1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PE2 PE3 PE4 PE5
                           PE6 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PE8 PE10 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_CS_Pin LCD_RS_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin|LCD_RS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_RES_Pin LED1_Pin */
  GPIO_InitStruct.Pin = LCD_RES_Pin|LED1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_CLK_Pin LCD_DAT_Pin */
  GPIO_InitStruct.Pin = LCD_CLK_Pin|LCD_DAT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : BT_U_Pin BT_SET_Pin BT_D_Pin BT_R_Pin
                           BT_M_Pin BT_L_Pin */
  GPIO_InitStruct.Pin = BT_U_Pin|BT_SET_Pin|BT_D_Pin|BT_R_Pin
                          |BT_M_Pin|BT_L_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartTaskKeyRead */
/**
  * @brief  Function implementing the keyReadTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartTaskKeyRead */
void StartTaskKeyRead(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
//  const char *mess = "Task: KeyRead\r\n";
  for(;;)
  {
    task3_KB();
    osDelay(20 / portTICK_RATE_MS); // Delay for 500 ms
//    printf(mess);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTaskUartRead */
/**
* @brief Function implementing the uartReadTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskUartRead */
void StartTaskUartRead(void *argument)
{
  /* USER CODE BEGIN StartTaskUartRead */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartTaskUartRead */
}

/* USER CODE BEGIN Header_StartTaskADC */
/**
* @brief Function implementing the myTaskADC1 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskADC */
void StartTaskADC(void *argument)
{
  /* USER CODE BEGIN StartTaskADC */
  /* Infinite loop */
  portTickType xLastWakeTime;
  for(;;)
  {
//	if (osSemaphoreRelease(myBinarySem01Handle) == osOK) {
//	}
//	osSemaphoreAcquire(myBinarySem01Handle, 0);
//	taskENTER_CRITICAL();
//	vTaskSuspendAll();
//	ADC_read_DMA_mode();
//	xTaskResumeAll();
//	taskEXIT_CRITICAL();
	vTaskDelayUntil( &xLastWakeTime, ( 1000 / portTICK_RATE_MS ) );
  }
  /* USER CODE END StartTaskADC */
}

/* USER CODE BEGIN Header_StartTaskReadBMP280 */
/**
* @brief Function implementing the BMP280Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskReadBMP280 */
void StartTaskReadBMP280(void *argument)
{
  /* USER CODE BEGIN StartTaskReadBMP280 */
  /* Infinite loop */
	//	const char *mess = "Task: KeyADC\r\n";
	portTickType xLastWakeTime;
  for(;;)
  {
//  простой пинг в терминал, что работаем и не зависли
//	printf("_Worked: %.3f sec\r\n", HAL_GetTick()/1000.0f);
	LED_Toggle;
	read_BMP280_ATH25();
	vTaskDelayUntil( &xLastWakeTime, ( 3000 / portTICK_RATE_MS ) );
  }
  /* USER CODE END StartTaskReadBMP280 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
