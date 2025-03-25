/*
 * mt6701.c
 *
 *  Created on: Sep 30, 2024
 *      Author: Maltsev Yuriy
 */

#include "mt6701.h"
#include "LCD_LPH8731.h"

uint8_t mt6701Buf[2];

uint16_t mt6701_read() {
	//if(HAL_I2C_IsDeviceReady(H_I2C, I2C_ADDR_MT6701 << 1, 1, 10) == HAL_OK)
	//	printf("Device found at address: 0x%02X on bus i2c: 2\r\n", address);
	//HAL_I2C_Master_Transmit(H_I2C, I2C_ADDR_MT6701 << 1, mt6701Buf, 1, 100);
	//HAL_I2C_Master_Receive(H_I2C, I2C_ADDR_MT6701 << 1, mt6701Buf, 2, 100);

	HAL_I2C_Mem_Read(&hi2c2, I2C_ADDR_MT6701 << 1, 3, I2C_MEMADD_SIZE_8BIT,
			mt6701Buf, 2, 100);
	return (mt6701Buf[0] << 6) | (mt6701Buf[1] >> 2);
}


void mt6701_read_test() {
	printf("mt6701 read\r\n");
	uint16_t rez_raw = mt6701_read();
	float deg = rez_raw * (360.0f / 16384.0f);
	printf("MT6701 = raw: %4d, deg: %.3f", rez_raw, deg);
	print_mem_8(mt6701Buf, 2);
	sprintf(buffLcd, " MT6701: %4d\n deg: %.3f", rez_raw, deg);
	lcd_print_str(buffLcd, 0, 4, 1, 1);
}
