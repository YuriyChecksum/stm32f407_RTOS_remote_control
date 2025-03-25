/*
 * bmp280.h
 *
 *  Created on: Sep 30, 2024
 *  Author: Maltsev Yuriy
 */
// BMP280 - датчик давления и температуры

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

#define BMP280_BUF_SIZE   256

#define mmHg (133.32236842105263157894736842105f) // 101325/760 ≈ 133,322 Па.

typedef struct {
    uint8_t osrs_t;   // Oversampling для температуры
    uint8_t osrs_p;   // Oversampling для давления
    uint8_t mode;     // Режим работы
    uint8_t t_sb;     // Подъем времени
    uint8_t filter;   // Фильтр
} SensorConfig_t;

extern SensorConfig_t sensorConfig;
extern uint8_t bmp280Buf[BMP280_BUF_SIZE];
extern float temperature, pressure;

//***********************************************//
void bmp280_init();
void bmp280_softReset();
void BMP280_Write(uint8_t, uint8_t);
void bmp280_Write2(uint8_t, uint8_t);
void bmp280_ReadData(float* , float*);
void bmp280_Read_All();
uint8_t bmp280_Read(uint8_t);
void bmp280_ReadCalibrationData(void);
void bmp280_PrintCalibrationData(void);
void bmp280_ReadPT();
void initSensorConfig(SensorConfig_t *config, uint8_t osrs_t, uint8_t osrs_p, uint8_t mode, uint8_t t_sb, uint8_t filter);
void readSensorConfig(const SensorConfig_t *config);

/*
 команды для тестирования через терминал:
 i2c 76 w 1 00f4 0001 27
 i2c 76 w 1 00f4 0001 57
 i2c 76 r 1 00f5 0001
 i2c 76 w 1 00f5 0001 28

 status    0xF3 (read only)  = measuring[0] [bit 3] | im_update[0] [bit 0]
 ctrl_meas 0xF4 = osrs_t[2:0] | osrs_p[2:0] | mode[1:0]          |
 config    0xF5 = t_sb[2:0]   | filter[2:0] | [0r] | spi3w_en[0] |

Calibration data from sensor
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

#endif /* INC_BMP280_H_ */
