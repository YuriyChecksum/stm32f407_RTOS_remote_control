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
//#include <stdio.h>

void mt6701_read_test();
uint16_t mt6701_read();

// лучше сделать приватным
#define H_I2C &hi2c2 // handle i2c   &hi2c2  &hi2c1

// MT6701 - магнитный энкодер абсолютного значения
#define I2C_ADDR_MT6701    0x06

//***********************************************//
uint8_t mt6701Buf[2];

uint16_t mt6701_read() {
	//if(HAL_I2C_IsDeviceReady(H_I2C, I2C_ADDR_MT6701 << 1, 1, 10) == HAL_OK)
	//	printf("Device found at address: 0x%02X on bus i2c: 2\r\n", address);
	//HAL_I2C_Master_Transmit(H_I2C, I2C_ADDR_MT6701 << 1, mt6701Buf, 1, 100);
	//HAL_I2C_Master_Receive(H_I2C, I2C_ADDR_MT6701 << 1, mt6701Buf, 2, 100);
	
	HAL_I2C_Mem_Read(H_I2C, I2C_ADDR_MT6701 << 1, 3, I2C_MEMADD_SIZE_8BIT,
			mt6701Buf, 2, 100);
	return (mt6701Buf[0] << 6) | (mt6701Buf[1] >> 2);
}


void mt6701_read_test() {
	printf("mt6701 read\r\n");
	uint16_t rez_raw = mt6701_read();
	float deg = rez_raw * (360.0f / 16384.0f);
	printf("MT6701 = raw: %4d, deg: %.3f", rez_raw, deg);
	print_mem_8(mt6701Buf, 2);
}


#endif /* INC_MT6701_H_ */
