/*
 * bmp280.c
 *
 *  Created on: Mar 22, 2025
 *      Author: Maltsev Yuriy
 */
#include "bmp280.h"
#include <stdio.h>
#include <utils.h>

#define hi2c_bmp280 &hi2c2

// Калибровочные параметры для вычисления давления по мануалу
static uint16_t dig_T1, dig_P1;
static  int16_t dig_T2, dig_T3, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;

SensorConfig_t sensorConfig;
uint8_t bmp280Buf[BMP280_BUF_SIZE];
float temperature, pressure;

void initSensorConfig(SensorConfig_t *config, uint8_t osrs_t, uint8_t osrs_p, uint8_t mode, uint8_t t_sb, uint8_t filter) {
    config->osrs_t = osrs_t;
    config->osrs_p = osrs_p;
    config->mode = mode;
    config->t_sb = t_sb;
    config->filter = filter;

    // Запуск преобразования
	// 0xF4 = 0x57 (t2 p16)
	BMP280_Write(BMP280_CTRL_MEAS, osrs_t << 5 | osrs_p << 2 | mode); // 0b00100111 = 0x27 Режим: нормальный, температура и давление
    BMP280_Write(BMP280_CTRL_CONF, t_sb << 5 | filter << 2); //0x28
}

// Функция чтения значений из структуры
// SensorConfig<LF>osrs_t: 2<LF>osrs_p: 4<LF>mode: 3<LF>t_sb: 1<LF>filter: 4<LF>T: 26.92; P1: 101324.45; P2: 759.9958;
void readSensorConfig(const SensorConfig_t *config) {
	printf("SensorConfig:\r\n");
	printf("osrs_t: %d\r\n", config->osrs_t);
    printf("osrs_p: %d\r\n", config->osrs_p);
    printf("mode:   %d\r\n", config->mode);
    printf("t_sb:   %d\r\n", config->t_sb);
    printf("filter: %d\r\n", config->filter);
    printf("BMP280_CTRL_MEAS 0xF4: 0x%02x\r\n", config->osrs_t << 5 | config->osrs_p << 2 | config->mode);
    printf("BMP280_CTRL_CONF 0xF5: 0x%02x\r\n", config->t_sb << 5 | config->filter << 2);
}

void bmp280_init() {
    // Инициализация структуры
    initSensorConfig(&sensorConfig, 0b010, 0b100, 0b11, 0b001, 0b100);
    // 0b010, 0b100, 0b11, 0b001, 0b100 = | t ×2 | p x8 | Normal mode | tstandby 62.5 ms | Filter x16? |
    // Чтение значений
    readSensorConfig(&sensorConfig);

    // Запуск преобразования
	// 0xF4 = 0x57 (t2 p16)
	//BMP280_Write(BMP280_CTRL_MEAS, 0b00100111); // 0b00100111 = 0x27 Режим: нормальный, температура и давление
    //BMP280_Write(BMP280_CTRL_CONF, 0x28);

    // Ожидание завершения преобразования
    HAL_Delay(100); // Время задержки для завершения

	// Чтение калибровочных данных
	bmp280_ReadCalibrationData();
}

/* The “reset” register contains the soft reset word reset[7:0]. If the value 0xB6 is written to the
register, the device is reset using the complete power-on-reset procedure. Writing other values
than 0xB6 has no effect. The readout value is always 0x00.  */
void bmp280_softReset() {
	BMP280_Write(BMP280_CTRL_RESET, 0xb6);
}

void BMP280_Write(uint8_t reg, uint8_t data) {
	HAL_I2C_Mem_Write(hi2c_bmp280, I2C_ADDR_BMP280 << 1, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

// Функция для записи в регистр
void bmp280_Write2(uint8_t reg, uint8_t value) {
    uint8_t data[2];
    data[0] = reg;
    data[1] = value;
    HAL_I2C_Master_Transmit(hi2c_bmp280, I2C_ADDR_BMP280 << 1, data, 2, HAL_MAX_DELAY);
}

// Функция для получения температуры и давления
// Не забудьте запустить непрерывное преобразование перед чтением данных
void bmp280_ReadData(float* temperature, float* pressure) {
	//BMP280_Write(BMP280_CTRL_MEAS, 0b00100111);
	//HAL_Delay(100);

	// if mode settings = Forced mode
	if (sensorConfig.mode == 1 || sensorConfig.mode == 2) {
		uint8_t _stat = 0;
		int n = 50;
		while (n--) {
			_stat = bmp280_Read(BMP280_CTRL_STAT);
			if ((_stat & 0b1001) == 0) {
				break;
			}
			printf("w\r\n");
			HAL_Delay(20);
		}
	}

    // Чтение данных
    uint8_t data[6];
    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(hi2c_bmp280, I2C_ADDR_BMP280 << 1, 0xF7, I2C_MEMADD_SIZE_8BIT, data, 6, HAL_MAX_DELAY);
	if (st != HAL_OK) {
		printf("Error: HAL status I2C_Mem_Read = %d", st);
		return;
	}

	// Формулы преобразования и пример кода есть в даташите. Здесь адаптированный код из даташита
    // Обработка данных давления и температуры
    int32_t adc_P = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    int32_t adc_T = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);
    //printf("adc_T: %f, adc_P: %f\r\n", (float)adc_T, (float)adc_P);

    // Преобразование температуры
    int32_t var1_T, var2_T, t;
	var1_T = (((adc_T >> 3) - ((int32_t) dig_T1 << 1)) * (int32_t)dig_T2) >> 11;
	var2_T = (((((adc_T >> 4) - (int32_t) dig_T1) * ((adc_T >> 4) - (int32_t) dig_T1)) >> 12) * (int32_t) dig_T3) >> 14;

    t = var1_T + var2_T;
    *temperature = (float)((t * 5 + 128) >> 8) / 100.0; //int32_t,  Преобразование в °C
    // ---------------------------------------------------- //
    // Преобразование давления
    // Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
    // Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
    //int64_t p;
    int64_t var1, var2, p;
    var1 = ((int64_t)t) - 128000;
    var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
    var1 = ((((int64_t)1) << 47) + var1) * ((int64_t)dig_P1) >> 33;

    if (var1 == 0) {
    	*pressure = 0;
    	printf("Error: divide by zero");
        return; // Обработка ошибки деления на ноль
    }

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dig_P8) * p) >> 19;

    p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4); // p - unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits)
    *pressure = (float)p / 256.0; // Преобразование в Pa
}

// Чтение из регистра
// dig_T1 = (BMP280_Read(0x88) | (BMP280_Read(0x89) << 8));
uint8_t bmp280_Read(uint8_t reg) {
    uint8_t value;
    HAL_I2C_Master_Transmit(hi2c_bmp280, I2C_ADDR_BMP280 << 1, &reg, 1, HAL_MAX_DELAY);
    HAL_I2C_Master_Receive (hi2c_bmp280, I2C_ADDR_BMP280 << 1, &value, 1, HAL_MAX_DELAY);
    return value;
}

// Функция для чтения калибровочных данных зашитых на заводе
void bmp280_ReadCalibrationData(void) {
    // Чтение данных
    uint8_t calData[24];
    HAL_I2C_Mem_Read(hi2c_bmp280, I2C_ADDR_BMP280 << 1, BMP280_CTRL_CALIB, I2C_MEMADD_SIZE_8BIT, calData, 24, 100);

    // лучше инкрементировать указатель, но тогда ругается компилятор
    dig_T1 = (calData[0] | (calData[1] << 8));      // address 0x88 0x89
    dig_T2 = (calData[2] | (calData[3] << 8));      // address 0x8A 0x8B
    dig_T3 = (calData[4] | (calData[5] << 8));      // address 0x8C 0x8D
    dig_P1 = (calData[6] | (calData[7] << 8));      // address 0x8E 0x8F
    dig_P2 = (calData[8] | (calData[9] << 8));      // address 0x90 0x91
    dig_P3 = (calData[10] | (calData[11] << 8));    // address 0x92 0x93
    dig_P4 = (calData[12] | (calData[13] << 8));    // address 0x94 0x95
    dig_P5 = (calData[14] | (calData[15] << 8));    // address 0x96 0x97
    dig_P6 = (calData[16] | (calData[17] << 8));    // address 0x98 0x99
    dig_P7 = (calData[18] | (calData[19] << 8));    // address 0x9A 0x9B
    dig_P8 = (calData[20] | (calData[21] << 8));    // address 0x9C 0x9D
    dig_P9 = (calData[22] | (calData[23] << 8));    // address 0x9E 0x9F
//  reserved = (BMP280_Read(0xA0) | (BMP280_Read(0xA1) << 8)); // reserved
}

void bmp280_PrintCalibrationData(void) {
	printf("Calibration_Data\r\n");
	printf(" T1: %d\r\n T2: %d\r\n T3: %d\r\n", dig_T1, dig_T2, dig_T3);
	printf(" P1: %d\r\n P2: %d\r\n P3: %d\r\n", dig_P1, dig_P2, dig_P3);
	printf(" P4: %d\r\n P5: %d\r\n P6: %d\r\n", dig_P4, dig_P5, dig_P6);
	printf(" P7: %d\r\n P8: %d\r\n P9: %d\r\n", dig_P7, dig_P8, dig_P9);
}

// Чтение данных температуры и давления
void bmp280_ReadPT() {
	bmp280_ReadData(&temperature, &pressure);
	printf("T2: %f, P2: %f, P2(mm рт. ст.): %f\r\n", temperature, pressure, pressure/mmHg);
}

// '_i2c 76 r 1 0000 0100 '
void bmp280_Read_All() {
	HAL_I2C_Mem_Read(hi2c_bmp280, I2C_ADDR_BMP280 << 1, 0, I2C_MEMADD_SIZE_8BIT,	bmp280Buf, BMP280_BUF_SIZE, 100);

	print_mem_8(bmp280Buf, BMP280_BUF_SIZE);
	//uint32_t p = ((bmp280Buf[BMP280_REG_PRESS] << 16) | (bmp280Buf[BMP280_REG_PRESS+1] << 8) | bmp280Buf[BMP280_REG_PRESS+2]) >> 4;
	//uint32_t t = ((bmp280Buf[BMP280_REG_TEMP] << 16)  | (bmp280Buf[BMP280_REG_TEMP+1] << 8)  | bmp280Buf[BMP280_REG_TEMP+2]) >> 4;
	//printf("T: %f, P: %f\r\n", (float)t, (float)p);

	// Чтение данных температуры и давления
	bmp280_ReadPT();

	bmp280_PrintCalibrationData();
}





