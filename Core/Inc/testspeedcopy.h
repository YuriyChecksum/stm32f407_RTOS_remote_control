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

#include "main.h"
#include <stdio.h>    // printf
#include <string.h>   // strlen, strcmp, memcpy
#include <utils.h>

// Конфигурация теста
#define TEST_BUF_SIZE (512 * 1)
#define TEST_N (2000)

extern uint8_t bufTest1[TEST_BUF_SIZE];
extern uint8_t bufTest2[TEST_BUF_SIZE];
//extern DMA_HandleTypeDef* dma_mem2mem_speed;
//#define dma_mem2mem_speed &hdma_memtomem_dma2_stream1

// Прототипы функций
void testspeedcopy(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_TESTSPEEDCOPY_H_ */
