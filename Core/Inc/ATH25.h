/*
 * ATH25.h
 *
 *  Created on: Oct 11, 2024
 *  Author: Maltsev Yuriy
 */

#ifndef INC_ATH25_H_
#define INC_ATH25_H_


#include "main.h"

// лучше объявлять не в заголовочном, чтобы скрыть видимость
#define H_I2C &hi2c2 // handle i2c   &hi2c2  &hi2c1

#define I2C_ADDR_ATH25   0x38   // ATH25 - влажность и температура

/* i2c 38 r 0 0000 0007  -> 18 84 BE 06 3B 5A 9D
 * где 18 ([7 bit]=0 преобразование окончено)/ 84 BE 0 - влажность , 6 3B 5A - температура, 9D - CRC8.
 */

const uint8_t startConversionBytes[3] = { 0xAC, 0x33, 0x00}; // стартовая последовательность для запуска преобразования
uint8_t data[7] = { 0 };

//***********************************************//
HAL_StatusTypeDef ATH25_Read_Data(float* temperature, float* humidity);
void ATH25_start_conversion();
void ATH25_softReset();
void ATH25_init();
void ATH25_test();
//***********************************************//

// Посылаем start conversion Command {0xAC, 0x33, 0x00} и ждём 80ms
void ATH25_start_conversion() {
	HAL_I2C_Master_Transmit(H_I2C, I2C_ADDR_ATH25 << 1, (uint8_t*)startConversionBytes, 3, 100); // start conversion
	HAL_Delay(80); // wait > 80ms
}

// Посылаем Soft Reset Command 0xBA = 1011‘1010
void ATH25_softReset() {
	uint8_t value = 0xBA; // Soft Reset Command 0xBA 1011‘1010
	HAL_I2C_Master_Transmit(H_I2C, I2C_ADDR_ATH25 << 1, &value, 1, 100);
}

/* Посылаем Initialization Command 0x71
 * i2c 38 r 1 0071 0001
 After power-on, wait no less than 100ms. Before reading the temperature and ATH25_humidity value,
 get a byte of status word by sending 0x71. If the status word and 0x18 are not equal to 0x18,
 initialize the 0x1B, 0x1C, 0x1E registers, details Please refer to our official website routine for
 the initialization process; if they are equal, proceed to the next step.
 */
void ATH25_init() {
	HAL_Delay(100); // после подачи питания нужно подождать более 100мс

	//Посылаем Initialization Command 0x71
	uint8_t val = 0;
	HAL_StatusTypeDef st = HAL_I2C_Mem_Read(H_I2C, I2C_ADDR_ATH25 << 1, 0x71,
			I2C_MEMADD_SIZE_8BIT, &val, 1, 100);
	if (val == 0x18) {
		printf("ATH25 init\r\n");
	} else {
		printf("Error init ATH25, HAL stat:%d, answer status 0x%02x\r\n", st, val);
	}
}

// Функция для получения температуры и влажности
HAL_StatusTypeDef ATH25_Read_Data(float* temperature, float* humidity) {
	ATH25_start_conversion();
	//uint8_t data[7] = { 0 };
	uint8_t i = 50; // 100 * 10мс = 0.5сек таймаут
	do {
		//printf("ATH25 read data\r\n");
		HAL_Delay(10);
		HAL_StatusTypeDef st = HAL_I2C_Master_Receive(H_I2C, I2C_ADDR_ATH25 << 1, data, 7, 100);
		if (st != HAL_OK) {
			printf("Error: HAL status I2C_Mem_Read = %d, i2c=0x%02x\r\n", st, I2C_ADDR_ATH25);
			return HAL_ERROR;
		}
	} while (data[0] & (1 << 7) && --i);

	// timeout ?
	if (data[0] & (1 << 7)) {
		printf("Error: ATH25 conversion timeout\r\n");
		return HAL_ERROR;
	}

	// CRC8
	uint8_t crc_read = data[6];
	uint8_t crc_calc = Crc8(data, 6);
	if (crc_read != crc_calc){
		printf("Error: ATH25 crc: 0x%02x != 0x%02x\r\n", crc_read, crc_calc);
		return HAL_ERROR;
	}
	uint32_t hum_raw = data[1] << 12 | data[2] << 4 | data[3] >> 4;
	uint32_t temp_raw = (data[3] & 0x0f) << 16 | data[4] << 8 | data[5];

	/* как преобразовать значения в температуру и влажность
	 * 2^20 = 1048576;
	 * 200/1048576 = 5242.88;
	 * T = raw/2^20 * 200 - 50 = raw/5242.88f - 50
	 * H = raw/2^20 * 100 = raw/10485.76f
	 */
	*humidity = hum_raw / 10485.76f;
	*temperature = temp_raw / 5242.88f - 50; // приводить (float)temp_raw не нужно
	return HAL_OK;
}

void ATH25_test() {
	//ATH25_init();
	float ATH25_temperature, ATH25_humidity;
	if (HAL_OK == ATH25_Read_Data(&ATH25_temperature, &ATH25_humidity)) {
		printf("ATH25 temperature: %.2f, humidity: %.2f\r\n",
				ATH25_temperature, ATH25_humidity);
	}
}

#endif /* INC_ATH25_H_ */
