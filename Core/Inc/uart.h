/*
 * uart.h
 *
 *  Created on: Mar 21, 2025
 *      Author: Maltsev Yuriy
 */

#ifndef INC_UART_H_
#define INC_UART_H_

#include "stm32f4xx_hal.h"

/*--------------------------- UART RX ----------------------------------------*/
/*
 * ver 1.1
 * Драйвер для приёма сообщений через UART в асинхронном режиме.
 * При получении данных ждём таймаут после которого понимаем, что пора обработать полученное.
 * Можно развить до следующего слоя абстрации, чтобы обрабатывать по счётчику принятых данных,
 * или по команде переданной в начале фрейма.
 * 11-12мсек на 1024 символа при 1млн бод = 0.68-0.74 эффективность канала
 * RX_BUF_LEN * 8/(0.7 * bod) * 1.2 = X секунд для заполнения буфера с запасом 20%
 * сократив выражение для 1млн бод Ttransmit (мсек) = RX_BUF_LEN * 0,014
 */

// #define UART_DEBUG // раскомментировать если нужна отладка приёма
#define TICK_WAIT_RX 50  //3.5 ms для 256 byte
#define UART &huart2
//#define dma_mem2mem_uart &hdma_memtomem_dma2_stream1

/*
 * Определил на практике следующие размеры буфера, которые может переварить данное ядро без пропуска данных.
 * f103 memcpy maxlen = 104 bytes справлятся, 108 - 112 зависает,
 * 	в DMA режиме - 512 справляется, чуть выше зависает
 * f407ve memcpy 520 работает, 524 байт виснет,
 * 	в DMA- 1856 работает, 1860 виснет
 *
 * Для DMA размер буффера выставлять кратным uint32_t = 4 байта
 */
#if defined(STM32F407xx)
#define RX_BUF_INIT_LEN 500
#define RX_BUF_LEN 1024
#else
#define RX_BUF_INIT_LEN 100
#define RX_BUF_LEN 128
#endif

// для отладочной информации, меньше задержек, точнее соблюдается порядок сообщений из прерываний
#define _PRNFAST(s) HAL_UART_Transmit_IT(UART, (uint8_t*)s, sizeof(s)-1); HAL_Delay(1);

extern uint8_t isDMA;
extern uint16_t maxlen;

/// Commands parse callback function.
typedef void (*parseFunc_t) (char *data);

/// Struct handles of parsing functions
typedef struct {
	parseFunc_t parseChr;
	parseFunc_t parseStr;
	parseFunc_t parseBin;
} stParseFunc_t;

// Реализация абстрактной функции для передачи данных через printf
//int fputc(int ch, FILE *f); // вроде её используем если C++, иначе __io_putchar
int __io_putchar(int ch);

void USART2_putch(char c);

void USART2_sendstr(const char *s);

void UART_Start();

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

uint8_t dataRxIsReady();

void dataParse(stParseFunc_t*);

#endif /* INC_UART_H_ */
