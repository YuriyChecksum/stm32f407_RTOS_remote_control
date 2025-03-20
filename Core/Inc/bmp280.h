/*
 * bmp280.h
 *
 *  Created on: Sep 30, 2024
 *  Author: Maltsev Yuriy
 */

/*
 Нужные параметры инициализации из даташита на датчик давления BMP280
The standby time is determined by the contents of the t_sb[2:0] bits in control register 0xF5
according to the table below:
+---------+---------------+
t_sb[1:0] | tstandby [ms] |
+---------+---------------+
  000     |      0.5      |
  001     |     62.5      |
  010     |    125        |
  011     |    250        |
  100     |    500        |
  101     |   1000        |
  110     |   2000        |
  111     |   4000        |
+---------+---------------+
==============================================
Temperature measurement can be enabled or skipped.

Table 5: osrs_t settings
+-------------+---------------------------+--------------------------------+
osrs_t[2:0]   | Temperature oversampling  | Typical temperature resolution |
+-------------+---------------------------+--------------------------------+
000           |  Skipped (output 0x80000) |                   –            |
001           |          ×1               |         16 bit / 0.0050 °C     |
010           |          ×2               |         17 bit / 0.0025 °C     |
011           |          ×4               |         18 bit / 0.0012 °C     |
100           |          ×8               |         19 bit / 0.0006 °C     |
101, 110, 111 |         ×16               |         20 bit / 0.0003 °C     |
+-------------+---------------------------+--------------------------------+

It is recommended to base the value of osrs_t on the selected value of osrs_p as per Table 4.
Temperature oversampling above ×2 is possible, but will not significantly improve the accuracy
of the pressure output any further. The reason for this is that the noise of the compensated
pressure value depends more on the raw pressure than on the raw temperature noise.
Following the recommended setting will result in an optimal noise-to-power ratio.
==============================================
3.3.1 Pressure measurement
Table 4: osrs_p settings
|----------------------|-----------------|------------------|------------------|
| Oversampling setting | Pressure        | Typical pressure | Recommended temp |
|                      | oversampling    |    resolution    |   oversampling   |
|----------------------|-----------------|------------------|------------------|
| Pressure measurement | Skipped (output |         -        |    As needed     |
|       skipped        | set to 0x80000) |                  |                  |
|----------------------|-----------------|------------------|------------------|
| Ultra low power      |      ×1         | 16 bit / 2.62 Pa |     ×1           |
| Low power            |      ×2         | 17 bit / 1.31 Pa |     ×1           |
| Standard resolution  |      ×4         | 18 bit / 0.66 Pa |     ×1           |
| High resolution      |      ×8         | 19 bit / 0.33 Pa |     ×1           |
| Ultra high resolution|     ×16         | 20 bit / 0.16 Pa |     ×2           |
|----------------------------------------|------------------|------------------|


Table 10: mode settings
mode[1:0] | Mode               |
00        | Sleep mode         |
01        | and 10 Forced mode |
11        | Normal mode        |

==============================================
The IIR filter can be configured using the filter[2:0] bits in control register 0xF5 with the following
options:
6 Since most pressure sensors do not sample continuously, filtering can suffer from signals with a frequency higher than the
sampling rate of the sensor. E.g. environmental fluctuations caused by windows being opened and closed might have a frequency
<5 Hz. Consequently, a sampling rate of ODR = 10 Hz is sufficient to obey the Nyquist theorem.

Filter settings
Filter coefficient | Samples to reach ≥75 % of step response
Filter off |  1   |
2          |  2   |
4          |  5   |
8          | 11   |
16         | 22   |
==============================================
3.8.2 Measurement Rate in Normal Mode
The following table explains which measurement rates can be expected in normal mode based on oversampling setting and
Table 14: Typical Output Data Rate (ODR) in Normal Mode [Hz]
+------------------------+--------+--------+--------+--------+--------+--------+--------+--------+
| Oversampling Setting   | 0.5    | 62.5   | 125    | 250    | 500    | 1000   | 2000   | 4000   |
+------------------------+--------+--------+--------+--------+--------+--------+--------+--------+
| Ultra low power        | 166.67 | 14.71  | 7.66   | 3.91   | 1.98   | 0.99   | 0.50   | 0.25   |
| Low power              | 125.00 | 14.29  | 7.55   | 3.88   | 1.97   | 0.99   | 0.50   | 0.25   |
| Standard resolution    | 83.33  | 13.51  | 7.33   | 3.82   | 1.96   | 0.99   | 0.50   | 0.25   |
| High resolution        | 50.00  | 12.20  | 6.92   | 3.71   | 1.92   | 0.98   | 0.50   | 0.25   |
| Ultra high resolution  | 26.32  | 10.00  | 6.15   | 3.48   | 1.86   | 0.96   | 0.49   | 0.25   |
+------------------------+--------+--------+--------+--------+--------+--------+--------+--------+

Table 15: Sensor Timing According to Recommended Settings (Based on Use Cases)
+----------------------------+-------+-----------------------+--------+--------+--------------------+--------+------+--------+
| Use Case                   | Mode  | Oversampling Setting  | osrs_p | osrs_t | IIR filter coeff.  | Timing | ODR  | BW     |
|                            |       |                       |        |        | (see 3.3.3)        |        | [Hz] | [Hz]   |
+----------------------------+-------+-----------------------+--------+--------+--------------------+--------+------+--------+
| Handheld device low-power  | Normal| Ultra high resolution | x16    | x2     | 4                  | 62.5 ms| 10.0 | 0.92   |
| (e.g., Android)            |       |                       |        |        |                    |        |      |        |
| Handheld device dynamic    | Normal| Standard resolution   | x4     | x1     | 16                 | 0.5 ms | 83.3 | 1.75   |
| (e.g., Android)            |       |                       |        |        |                    |        |      |        |
| Weather monitoring         | Forced| Ultra low power       | x1     | x1     | Off                | 1/min  | 1/60 | full   |
| (lowest power)             |       |                       |        |        |                    |        |      |        |
| Elevator / floor change    | Normal| Standard resolution   | x4     | x1     | 4                  | 125 ms | 7.3  | 0.67   |
| detection                  |       |                       |        |        |                    |        |      |        |
| Drop detection             | Normal| Low power             | x2     | x1     | Off                | 0.5 ms | 125  | full   |
| Indoor navigation          | Normal| Ultra high resolution | x16    | x2     | 16                 | 0.5 ms | 26.3 | 0.55   |
+----------------------------+-------+-----------------------+--------+--------+--------------------+--------+------+--------+
 * 
1 мм рт. ст. = 101325 / 760  ≈ 133,322 Па.
Стандартное атмосферное давление принято равным (точно) 760 мм рт.ст., или 101 325 Па
1 мм рт.ст. = 13,5951 мм вод.ст.

+------------------+----------------+----------------------+---------------------+-------------------+---------------------+---------------------+----------------------------+
|                  |Паспкаль(Pa, Пa)| Бар (bar, бар)       | Техн. атм-а (at, ат)|Физ. атм.(atm, атм)| мм рт. ст., мм Hg   | мм вод. ст., мм H2O)| Фунт-сила на кв. дюйм (psi)|
+------------------+----------------+----------------------+---------------------+-------------------+---------------------+---------------------+----------------------------+
| 1 Па             | 1              | 10^-5                | 1.01972 × 10^-5     | 9.869210 × 10^-6  | 7.5006 × 10^-3      | 0.101972            | 1.4504 × 10^-4             |
| 1 бар            | 10^5           | 1                    | 1.01972             | 0.98692           | 750.06              | 10197.2             | 14.504                     |
| 1 ат             | 98066.5        | 0.980665             | 1                   | 0.96784           | 735.56              | 10^4                | 14.223                     |
| 1 атм            | 101325         | 1.01325              | 1.03323             | 1                 | 760                 | 10332.3             | 14.696                     |
| 1 мм рт. ст.     | 133.322        | 1.33321 × 10^-3      | 1.3595 × 10^-3      | 1.3158 × 10^-3    | 1                   | 13.595              | 0.019337                   |
| 1 мм вод. ст.    | 9.80665        | 9.80665 × 10^-5      | 10^4                | 9.678410 × 10^-5  | 0.073556            | 1                   | 1.4223 × 10^-3             |
| 1 psi            | 6894.76        | 0.068948             | 0.070307            | 0.068046          | 51.715              | 703.07              | 1                          |
+------------------+----------------+----------------------+---------------------+-------------------+---------------------+---------------------+----------------------------+

 */
#ifndef INC_BMP280_H_
#define INC_BMP280_H_

#include "main.h"

// лучше объявлять не в заголовочном, чтобы скрыть видимость
#define H_I2C &hi2c2 // handle i2c   &hi2c2  &hi2c1

// BMP280
#define I2C_ADDR_BMP280   0x76
#define BMP280_I2C_ADDRESS 0x76 // Адрес BMP280 на шине I2C

#define BMP280_CTRL_CALIB 0x88 // 88..A1
#define BMP280_CTRL_ID    0xD0
#define BMP280_CTRL_RESET 0xE0
#define BMP280_CTRL_STAT  0xF3
#define BMP280_CTRL_MEAS  0xF4
#define BMP280_CTRL_CONF  0xF5
#define BMP280_REG_PRESS  0xF7 // F7..F9
#define BMP280_REG_TEMP   0xFA // FA..FC

#define BMP280_BUF_SIZE        256

#define mmHg (133.32236842105263157894736842105f) // 101325/760 ≈ 133,322 Па.
uint8_t bmp280Buf[BMP280_BUF_SIZE];

float temperature, pressure;

// Калибровочные параметры
static uint16_t dig_T1, dig_P1;
static  int16_t dig_T2, dig_T3, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;

// Определение структуры
typedef struct {
    uint8_t osrs_t;   // Oversampling для температуры
    uint8_t osrs_p;   // Oversampling для давления
    uint8_t mode;     // Режим работы
    uint8_t t_sb;     // Подъем времени
    uint8_t filter;   // Фильтр
} SensorConfig;

SensorConfig sensorConfig;

//***********************************************//
void bmp280_init();
void bmp280_softReset();
void BMP280_Write(uint8_t, uint8_t);
void BMP280_Write2(uint8_t, uint8_t);
void BMP280_Read_Data(float* , float*);
void BMP280_Read_All();
uint8_t BMP280_Read(uint8_t);
void BMP280_Read_Calibration_Data(void);
void BMP280_Print_Calibration_Data(void);
void BMP280_Read_PT();
void initSensorConfig(SensorConfig *config, uint8_t osrs_t, uint8_t osrs_p, uint8_t mode, uint8_t t_sb, uint8_t filter);
void readSensorConfig(const SensorConfig *config);

//***********************************************//
// команды для тестирования через COM порт:
// i2c 76 w 1 00f4 0001 27
// i2c 76 w 1 00f4 0001 57
// i2c 76 r 1 00f5 0001
// i2c 76 w 1 00f5 0001 28
//
// status    0xF3 (read only)  = measuring[0] [bit 3] | im_update[0] [bit 0]
// ctrl_meas 0xF4 = osrs_t[2:0] | osrs_p[2:0] | mode[1:0]          |
// config    0xF5 = t_sb[2:0]   | filter[2:0] | [0r] | spi3w_en[0] |

void initSensorConfig(SensorConfig *config, uint8_t osrs_t, uint8_t osrs_p, uint8_t mode, uint8_t t_sb, uint8_t filter) {
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
void readSensorConfig(const SensorConfig *config) {
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
	BMP280_Read_Calibration_Data();
}

/* The “reset” register contains the soft reset word reset[7:0]. If the value 0xB6 is written to the
register, the device is reset using the complete power-on-reset procedure. Writing other values
than 0xB6 has no effect. The readout value is always 0x00.  */
void bmp280_softReset() {
	BMP280_Write(BMP280_CTRL_RESET, 0xb6);
}

void BMP280_Write(uint8_t reg, uint8_t data) {
	HAL_I2C_Mem_Write(H_I2C, I2C_ADDR_BMP280 << 1, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

// Функция для записи в регистр
void BMP280_Write2(uint8_t reg, uint8_t value) {
    uint8_t data[2];
    data[0] = reg;
    data[1] = value;
    HAL_I2C_Master_Transmit(H_I2C, I2C_ADDR_BMP280 << 1, data, 2, HAL_MAX_DELAY);
}

// Функция для получения температуры и давления
// Не забудьте запустить непрерывное преобразование перед чтением данных
void BMP280_Read_Data(float* temperature, float* pressure) {
	//BMP280_Write(BMP280_CTRL_MEAS, 0b00100111);
	//HAL_Delay(100);

	// if mode settings = Forced mode
	if (sensorConfig.mode == 1 || sensorConfig.mode == 2) {
		uint8_t _stat = 0;
		int n = 50;
		while (n--) {
			_stat = BMP280_Read(BMP280_CTRL_STAT);
			if ((_stat & 0b1001) == 0) {
				break;
			}
			printf("w\r\n");
			HAL_Delay(20);
		}
	}

    // Чтение данных
    uint8_t data[6];
    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(H_I2C, I2C_ADDR_BMP280 << 1, 0xF7, I2C_MEMADD_SIZE_8BIT, data, 6, HAL_MAX_DELAY);
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

// '_i2c 76 r 1 0000 0100 '
void BMP280_Read_All() {
	HAL_I2C_Mem_Read(H_I2C, I2C_ADDR_BMP280 << 1, 0, I2C_MEMADD_SIZE_8BIT,	bmp280Buf, BMP280_BUF_SIZE, 100);

	print_mem_8(bmp280Buf, BMP280_BUF_SIZE);
	//uint32_t p = ((bmp280Buf[BMP280_REG_PRESS] << 16) | (bmp280Buf[BMP280_REG_PRESS+1] << 8) | bmp280Buf[BMP280_REG_PRESS+2]) >> 4;
	//uint32_t t = ((bmp280Buf[BMP280_REG_TEMP] << 16)  | (bmp280Buf[BMP280_REG_TEMP+1] << 8)  | bmp280Buf[BMP280_REG_TEMP+2]) >> 4;
	//printf("T: %f, P: %f\r\n", (float)t, (float)p);

	// Чтение данных температуры и давления
	BMP280_Read_PT();

	BMP280_Print_Calibration_Data();
}

// Функция для чтения из регистра
// dig_T1 = (BMP280_Read(0x88) | (BMP280_Read(0x89) << 8));
uint8_t BMP280_Read(uint8_t reg) {
    uint8_t value;
    HAL_I2C_Master_Transmit(H_I2C, I2C_ADDR_BMP280 << 1, &reg, 1, HAL_MAX_DELAY);
    HAL_I2C_Master_Receive (H_I2C, I2C_ADDR_BMP280 << 1, &value, 1, HAL_MAX_DELAY);
    return value;
}

// Функция для чтения калибровочных данных зашитых на заводе
void BMP280_Read_Calibration_Data(void) {
    // Чтение данных
    uint8_t calData[24];
    HAL_I2C_Mem_Read(H_I2C, I2C_ADDR_BMP280 << 1, BMP280_CTRL_CALIB, I2C_MEMADD_SIZE_8BIT, calData, 24, 100);

    // лучше инкрементировать указатель, но тогда ругается компилятор
    dig_T1 = (calData[0] | (calData[1] << 8));
    dig_T2 = (calData[2] | (calData[3] << 8));
    dig_T3 = (calData[4] | (calData[5] << 8));
    dig_P1 = (calData[6] | (calData[7] << 8));
    dig_P2 = (calData[8] | (calData[9] << 8));
    dig_P3 = (calData[10] | (calData[11] << 8));
    dig_P4 = (calData[12] | (calData[13] << 8));
    dig_P5 = (calData[14] | (calData[15] << 8));
    dig_P6 = (calData[16] | (calData[17] << 8));
    dig_P7 = (calData[18] | (calData[19] << 8));
    dig_P8 = (calData[20] | (calData[21] << 8));
    dig_P9 = (calData[22] | (calData[23] << 8));

//    dig_T1 = (BMP280_Read(0x88) | (BMP280_Read(0x89) << 8));
//    dig_T2 = (BMP280_Read(0x8A) | (BMP280_Read(0x8B) << 8));
//    dig_T3 = (BMP280_Read(0x8C) | (BMP280_Read(0x8D) << 8));
//    dig_P1 = (BMP280_Read(0x8E) | (BMP280_Read(0x8F) << 8));
//    dig_P2 = (BMP280_Read(0x90) | (BMP280_Read(0x91) << 8));
//    dig_P3 = (BMP280_Read(0x92) | (BMP280_Read(0x93) << 8));
//    dig_P4 = (BMP280_Read(0x94) | (BMP280_Read(0x95) << 8));
//    dig_P5 = (BMP280_Read(0x96) | (BMP280_Read(0x97) << 8));
//    dig_P6 = (BMP280_Read(0x98) | (BMP280_Read(0x99) << 8));
//    dig_P7 = (BMP280_Read(0x9A) | (BMP280_Read(0x9B) << 8));
//    dig_P8 = (BMP280_Read(0x9C) | (BMP280_Read(0x9D) << 8));
//    dig_P9 = (BMP280_Read(0x9E) | (BMP280_Read(0x9F) << 8));
    //reserved = (BMP280_Read(0xA0) | (BMP280_Read(0xA1) << 8)); // reserved
}

void BMP280_Print_Calibration_Data(void) {
	printf("Calibration_Data\r\n");
	printf(" T1: %d\r\n T2: %d\r\n T3: %d\r\n", dig_T1, dig_T2, dig_T3);
	printf(" P1: %d\r\n P2: %d\r\n P3: %d\r\n", dig_P1, dig_P2, dig_P3);
	printf(" P4: %d\r\n P5: %d\r\n P6: %d\r\n", dig_P4, dig_P5, dig_P6);
	printf(" P7: %d\r\n P8: %d\r\n P9: %d\r\n", dig_P7, dig_P8, dig_P9);
}

// Чтение данных температуры и давления
void BMP280_Read_PT() {
	BMP280_Read_Data(&temperature, &pressure);
	printf("T2: %f, P2: %f, P2(mm рт. ст.): %f\r\n", temperature, pressure, pressure/mmHg);
}

/* Calibration data from sensor
 adc_T: 536912.000000, adc_P: 317776.000000
 T2: 27.820000, P2: 101068.406250
 Calibration_Data
  T1: 27918
  T2: 25864
  T3: 50
  P1: 37971
  P2: -10728
  P3: 3024
  P4: 7458
  P5: -189
  P6: -7
  P7: 15500
  P8: -14600
  P9: 6000
 */
//printf("mt6701 read\r\n");
//uint16_t rez_raw = mt6701_read();
//float deg = rez_raw * (360.0f / 16384.0f);
//printf("MT6701 = raw: %4d, deg: %.3f", rez_raw, deg);
//print_mem_8(mt6701Buf, 2);

#endif /* INC_BMP280_H_ */
