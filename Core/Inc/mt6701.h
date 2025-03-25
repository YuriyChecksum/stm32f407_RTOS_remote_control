/*
 * mt6701.h
 *
 *  Created on: Sep 30, 2024
 *  Author: Maltsev Yuriy
 */
// MT6701 Hall Based Angle Position Encoder Sensor
// бесконтактный датчик угла поворота. Используется внешний радиально намагниченный круглый магнит

#ifndef INC_MT6701_H_
#define INC_MT6701_H_

#include "main.h"
#include <stdio.h>
#include "utils.h"

#define I2C_ADDR_MT6701    0x06

extern uint8_t mt6701Buf[2];

void mt6701_read_test();
uint16_t mt6701_read();

#endif /* INC_MT6701_H_ */
