/*
 * testspeedcopy.c
 *
 *  Created on: Mar 22, 2025
 *      Author: Maltsev Yuriy
 */

#include "testspeedcopy.h"

// Определение глобальных переменных
uint8_t bufTest1[TEST_BUF_SIZE] = {0};
uint8_t bufTest2[TEST_BUF_SIZE] = {0};
DMA_HandleTypeDef* dma_mem2mem_speed = &hdma_memtomem_dma2_stream1;

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
// Функция тестирования
void testspeedcopy() {
	uint32_t timestart = 0;
	uint32_t timeend = 0;
	printf("test1 ");
	HAL_Delay(100);
    // инициализация данных в исходном буфере
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

	// копирование данных из исходного буфера в целевой буфер в блокирующем режиме
	printf("test2 ");
	HAL_Delay(100);
    for (int i = 0; i < TEST_BUF_SIZE; i++) {
    	bufTest1[i] = (uint8_t)i;
    	bufTest2[i] = 0;
    }

	timestart = HAL_GetTick();
	for (int i = 0; i < TEST_N; i++) {
		HAL_DMA_Start(dma_mem2mem_speed, (uint32_t) bufTest1, (uint32_t) bufTest2, TEST_BUF_SIZE/4);
		HAL_DMA_PollForTransfer(dma_mem2mem_speed, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
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
