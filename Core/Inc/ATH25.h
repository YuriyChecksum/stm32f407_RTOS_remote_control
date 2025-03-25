/*
 * ATH25.h
 *
 *  Created on: Oct 11, 2024
 *  Author: Maltsev Yuriy
 */
/* ATH25 - влажность и температура.
 * Команда в терминале для чтения: i2c 38 r 0 0000 0007
 * Ответ 7 байт -> 18 84 BE 06 3B 5A 9D
 * где 18 ([7 bit]=0 преобразование окончено),
 * 84 BE 0 - влажность,
 * 6 3B 5A - температура,
 * 9D - CRC8.
 */
#ifndef INC_ATH25_H_
#define INC_ATH25_H_

#include "main.h"

#define I2C_ADDR_ATH25  ( 0x38 )

HAL_StatusTypeDef ATH25_Read_Data(float* temperature, float* humidity);
void ATH25_start_conversion();
void ATH25_softReset();
void ATH25_init();
void ATH25_test();

#endif /* INC_ATH25_H_ */
