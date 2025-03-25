/*
 * uart.c
 *
 *  Created on: Mar 21, 2025
 *      Author: Maltsev Yuriy
 */
#include "uart.h"

#include "main.h"
#include <string.h>   // strlen, strcmp memcpy
#include <stdio.h>

uint8_t chRx = 0;
char bufRx1[RX_BUF_LEN] = { 0 };
char bufRx2[RX_BUF_LEN] = { 0 };
volatile uint16_t idx = 0;
volatile uint16_t lenRxData = 0;
volatile uint8_t isFull = 0; // флаг переполнения
uint8_t isReciveCmplete = 0; // флаг приёма перевода строки
uint32_t UARTtimeout = 0;

// Динамическое изменения размера буфера приёма
// для тестирования под конкретный камень приёма данных максимальной длины
uint16_t maxlen = RX_BUF_INIT_LEN;
uint8_t isDMA = 0; // on / off DMA copy mode
DMA_HandleTypeDef* dma_mem2mem_uart = &hdma_memtomem_dma2_stream1;
//UART_HandleTypeDef* UART = &huart2;

/*--------------------------- UART TX ----------------------------------------*/
// Реализация абстрактной функции для передачи данных через printf
// в C++ используем fputc, в C __io_putchar
// int fputc(int ch, FILE *f){
int __io_putchar(int ch){
	ITM_SendChar((uint32_t) ch); // блокирующий режим
	while (HAL_UART_Transmit(&huart2, (unsigned char*) &ch, 1, 10) == HAL_BUSY); // может вернуть HAL_ERROR, HAL_BUSY
	// если хотим слать в USB CDC
	//while( CDC_Transmit_FS((uint8_t *)&ch, 1) == USBD_BUSY ); // USBD_FAIL , USBD_BUSY
	return ch;
}

// ещё вариант передачи на регистрах
void USART2_putch(char c) {
	while (!(USART2->SR & USART_SR_TXE)); // ожидание готовности
	USART2->DR = c;
}

void USART2_sendstr(const char *s) {
	while (*s)
		USART2_putch(*s++);
}

/*--------------------------- UART RX ----------------------------------------*/
/* В инициализации нужно запустить приём первого символа данных из UART
 * командой HAL_UART_Receive_IT(UART, &chRx, 1). Далее он будет перевызываться в обработчике.
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
	// если первый символ например указывает что будут бинарные данные,
	// то принять байт длинны посылки и выполнять внешний код при заполнении
	// или если строковая то ждём /r /n.
	if (chRx == '\n' && bufRx1[0] != '*') {
		isReciveCmplete = 1; // начнётся обработка строки не дожидаясь таймаута при следующем запуске dataRxIsReady()
	}

	/* Если второй буфер и так полный и первый тоже заполнился, то прерывания продолжаем,
	 * но новые данные не сохраняем (будут потери).
	 * Иначе перебросим данные из первого во второй и выставим флаг переполнения.
	 * memcpy не успевает скопировать от 100 до 120 байт без пропуска следующего символа на 1млн бод
	 * Получаем скорость копирования около 11МГц. volatile не спасает, как и оптимизация.
	 * Можно попробовать ускорить с помощью DMA, либо использовать блинный буфер,
	 * который точно не переполнится при самой длинной команде или данных. */
	if ((isFull == 0) && (idx >= maxlen - 1)) {	//if ((chRx[0] == '\n') || (idx >= RX_BUF_LEN - 1 - 0)) {
		bufRx1[maxlen - 1] = 0; // '\0' символ, если вдруг буду использовать как string
		if (isDMA == 0) {
			// strcpy не подходит тк данные могут быть бинарными
			memcpy(bufRx2, bufRx1, maxlen);
		} else {
			HAL_DMA_Start(dma_mem2mem_uart, (uint32_t) bufRx1, (uint32_t) bufRx2, maxlen / 4);
			HAL_DMA_PollForTransfer(dma_mem2mem_uart, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
		}
		idx = 0;
		lenRxData = maxlen;
		isFull = 1;
	}
	// запрашиваем следующий байт данных из порта через прерывание
	HAL_UART_Receive_IT(UART, &chRx, 1);
}

/* Запустим приём первого символа данных по прерыванию
 * Обработчик перывания складывает принятые байты в буфер и заного запустит приём
 * Такая сложность изза незнания сколько всего надо принять,
 * попытка сделать универсальный драйвер связи. */
void UART_Start() {
	// Стартуем приём команд из UART
	HAL_UART_Receive_IT(UART, &chRx, 1);
}

/* Полингом примерно раз в 5 мсек проверяем выставлен ли флаг переполнения входного буфера,
 * а так же был ли таймаут приёма.
 * TO DO: желательно начинать парсинг данных не по таймауту, а по сигнатуре конца посылки,
 * например для строк и символов это окончание строки \r или \n.
 * Для бинарных иной вариант, например фиксированная длинна, или длинна указанная в первом байте.
 */
uint8_t dataRxIsReady()
{
	// isFull = 1 был переполнен входной буфер, данные уже лежат в bufRx2
	// нежелательное событие, так как данные либо не были вовремя обработаны и
	// скопилась очередь, либо пришла команда длинее чем входной буфер.
	if (isFull != 0)
	{
#ifdef UART_DEBUG
		_PRNFAST("_FULL_BUFF_\r\n");
		HAL_StatusTypeDef stat = HAL_UART_Transmit_IT(UART, (uint8_t *)bufRx2, maxlen - 1);
		HAL_Delay(TICK_WAIT_RX);
		if (stat != HAL_OK)
		{
			_PRNFAST("*");
		}
		_PRNFAST("_ENDF_\r\n");
#endif
		// тут должна быть обработка данных при переполнении в буфере
		// isFull = 0; // желательно не сбрасывать, пока не обработали данные буфера
	}
	// данные в bufRx1 после таймаута ожидания TICK_WAIT_RX
	if (idx == 0)
	{
		return 0;
	}
	if (isReciveCmplete || (HAL_GetTick() > UARTtimeout + TICK_WAIT_RX))
	{
		// uint16_t n = idx;
		// idx = 0; // как можно быстрее готовим к приёму данных
		// memcpy(bufRx2, bufRx1, n);

		memcpy(bufRx2, bufRx1, idx);
		bufRx2[idx] = 0; // впишем конец строки, чтобы не было казусов

#ifdef UART_DEBUG
		_PRNFAST("_T_");
		HAL_StatusTypeDef stat = HAL_UART_Transmit_IT(UART, (uint8_t *)bufRx2, idx);
		HAL_Delay(TICK_WAIT_RX);
		if (stat != HAL_OK)
		{
			_PRNFAST("!");
		}
		_PRNFAST("_END_T_\r\n");
#endif
		lenRxData = idx;
		idx = 0;
		isFull = 0;
		isReciveCmplete = 0;
//#ifdef UART_DEBUG
		printf("timeout: %d\r\n", (int)(HAL_GetTick() - UARTtimeout));
		// или, если на малых кристаллах не используем printf:
		// char tmpbuf[20];
		// itoa((UARTtimeout - HAL_GetTick()), tmpbuf);
//#endif

		/* TO DO: добавить семафор, чтобы не копировались данные из bufRx1 в bufRx2
		 * пока не завершилась обработка данных из буфера */
		return 1;
	}
	return 0;
}


// Идентифицируем какого типа команда в буфере и
// распределяем обработку по соответствующим callback'ам:
// строковые (начинаются с '_'),
// бинарные (начинаются с '*')
// и односимвольные команды.
void dataParse(stParseFunc_t *funcs) {
	if (funcs == NULL){
		return;
	}

	if (bufRx2[0] == '*') {
		if (funcs->parseBin != NULL) {
			funcs->parseBin(bufRx2);
		}
	} else if (bufRx2[0] != 0 && bufRx2[0] != '\r' && bufRx2[0] != '\n') {
		// Если второй байт последовательности = 0 либо переводу строки,
		// значит приняли односимвольную команду, иначе строковую.
		if (bufRx2[1] == 0 || bufRx2[1] == '\r' || bufRx2[1] == '\n') {
			if (funcs->parseChr != NULL) {
				funcs->parseChr(bufRx2);
			}
		} else if (funcs->parseStr != NULL) {
			funcs->parseStr(bufRx2);
		}
	}
}


