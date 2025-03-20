/*
 * testspeedcopy.h
 *
 *  Created on: May 30, 2024
 *  Author: Maltsev Yuriy
 */
// функции для тестирования скорости копирования буфера данных на разных контроллерах
// и разными методами полингом и через DMA побайтно и DMA int16
// нужно для определения какой объЄм данных можем максимально прогонять через интерфейсы с PC

#ifndef INC_TESTSPEEDCOPY_H_
#define INC_TESTSPEEDCOPY_H_

#ifdef __cplusplus
extern "C" {
#endif
// **************************************************************
#include "main.h"
#include <stdio.h>    // printf

// **************************************************************
/*----------------------------- testspeedcopy ------------------------------------*/

#define TEST_BUF_SIZE (512 * 1)
#define TEST_N (2000)
uint8_t bufTest1[TEST_BUF_SIZE] = { 0 };
uint8_t bufTest2[TEST_BUF_SIZE] = { 0 };
#define HDMA_MEM2MEM &hdma_memtomem_dma2_stream1

/* STM32F103  (72 МГц)
 * DMA в блокирующем режиме byte to byte, приоритет средний.
 memcpy()         N 1000, len 1024, 128 (ms)   8    МГц
 DMA memtomem      N 1000, len 1024,  89 (ms)  11.5  МГц
 memcpy()          N 2000, len  512, 129 (ms),  7.93 MHz
 DMA memtomem      N 2000, len  512,  91 (ms), 11.25 MHz

 DMA в блокирующем режиме uint32 to uint32, приоритет средний.
 memcpy()          N 2000, len  512, 129 (ms), 7.93 MHz
 DMA memtomem      N 2000, len  512,  27 (ms), 37.92 MHz

 STM32F407ve  (168 МГц)
 DMA в блокирующем режиме uint32 to uint32, приоритет низкий.
 memcpy()          N 2000, len  512,  31 (ms), 33.0322572.2 MHz
 DMA memtomem      N 2000, len  512,  11 (ms), 93.0909122.2 MHz
 */
void testspeedcopy() {
	uint32_t timestart = 0;
	uint32_t timeend = 0;
	printf("test1 ");
	HAL_Delay(100);
    // »нициализация данных в исходном буфере
    for (int i = 0; i < TEST_BUF_SIZE; i++) {
    	bufTest1[i] = (uint8_t)i;
    }
	timestart = HAL_GetTick();
	for (int i = 0; i < TEST_N; i++) {
		memcpy(bufTest2, bufTest1, TEST_BUF_SIZE);
	}
	timeend = HAL_GetTick();
	printf("test memcpy() N %4d, len %4d, %3d (ms), %f2.2 MHz\r\n", TEST_N,
			TEST_BUF_SIZE, (int) (timeend - timestart),
			(float) (1/ (float) ((float) (timeend - timestart) * 1000/ (float) (TEST_N * TEST_BUF_SIZE))));
	HAL_Delay(200);
	//hdma_memtomem_dma1_channel2

	//  опирование данных из исходного буфера в целевой буфер в блокирующем режиме
	printf("test2 ");
	HAL_Delay(100);
    for (int i = 0; i < TEST_BUF_SIZE; i++) {
    	bufTest1[i] = (uint8_t)i;
    	bufTest2[i] = 0;
    }

	timestart = HAL_GetTick();
	for (int i = 0; i < TEST_N; i++) {
		HAL_DMA_Start(HDMA_MEM2MEM, (uint32_t) bufTest1, (uint32_t) bufTest2, TEST_BUF_SIZE/4);
		HAL_DMA_PollForTransfer(HDMA_MEM2MEM, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
	}
	timeend = HAL_GetTick();
	printf("test DMA memtomem N %4d, len %4d, %3d (ms), %f2.2 MHz\r\n", TEST_N,
			TEST_BUF_SIZE, (int) (timeend - timestart),
			(float) (1/ (float) ((float) (timeend - timestart) * 1000/ (float) (TEST_N * TEST_BUF_SIZE))));
	print_mem_8(bufTest1, TEST_BUF_SIZE);
	printf("\r\n\r\n");
	print_mem_8(bufTest2, TEST_BUF_SIZE);
	printf("\r\n\r\n");
}
/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* INC_TESTSPEEDCOPY_H_ */
