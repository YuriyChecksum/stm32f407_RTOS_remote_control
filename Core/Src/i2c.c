/*
 * i2c.c
 *
 *  Created on: May 10, 2024
 *  Author: Maltsev Yuriy
 */
#include "i2c.h"

#include "main.h"
#include <stdio.h>

/*
 I2C_ADDR_ATH25           0x38 // ATH25 влажность и температура
 I2C_ADDR_BMP280          0x76 // температура и давление
 BH1750_DEFAULT_I2CADDR   0x23 // освещённость в люксах
 BH1750_SECOND_I2CADDR    0x5C
 I2C_ADDR_MT6701          0x06 // MT6701 - магнитный энкодер абсолютного значения
 I2C_ADDR_24C02           0x52 // 24C02 (A0 & A2 = GND, A1 = Vcc) adress: b1010010 = 0x52 = dec:82
*/

void scanI2C() {
	printf("Scanning I2C buses 1...\r\n");
	for(uint8_t address = 1; address < 128; address++)
	{
		if(HAL_I2C_IsDeviceReady(&hi2c1, address << 1, 1, 10) == HAL_OK)
		{
			printf("Device found at address: 0x%02X on bus i2c: 1\r\n", address);
		}
	}

	printf("Scanning I2C buses 2...\r\n");
	for(uint8_t address = 1; address < 128; address++)
	{
		if(HAL_I2C_IsDeviceReady(&hi2c2, address << 1, 1, 10) == HAL_OK)
		{
			printf("Device found at address: 0x%02X on bus i2c: 2\r\n", address);
		}
	}
}

/*
 * Перезапускает контроллер I2C если он завис и выдаёт статус BUSY.
 * Для определения что завис будем получать BUSY, а если
 * читать состояния пинов порта они будут в 1.
 * */
void i2c_sw_reset(){
	;
	//п.п. 26.6.1 I2C Control register 1 (I2C_CR1) (стр. 772)
	SET_BIT(I2C1->CR1, I2C_CR1_SWRST); //: I2C Peripheral not under reset
	while (READ_BIT(I2C1->CR1, I2C_CR1_SWRST) == 0) ;
	CLEAR_BIT(I2C1->CR1, I2C_CR1_SWRST); //: I2C Peripheral not under reset
	while (READ_BIT(I2C1->CR1, I2C_CR1_SWRST)) ;
	/* Примечание: Этот бит можно использовать для повторной инициализации
	* периферийного устройства после ошибки или заблокированного состояния.
	* Например, если бит BUSY установлен и остается заблокированным из-за сбоя на шине,
	* бит SWRST можно использовать для выхода из этого состояния.*/
	//MX_I2C1_Init();
}

/*
void i2c_Init(void) {
    // Включение тактирования I2C1
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    // Настройка I2C1
    I2C1->CR1 |= I2C_CR1_SWRST;  // Сброс I2C1
    I2C1->CR1 &= ~I2C_CR1_SWRST; // Снятие сброса
    I2C1->CR2 |= (16 << I2C_CR2_FREQ_Pos); // Частота I2C 16 МГц

    I2C1->CCR = 80;   // Настройка CCR (Cofiguration Clock Register)
    I2C1->TRISE = 17; // Настройка TRISE
    I2C1->CR1 |= I2C_CR1_PE; // Включение I2C1
}*/

uint8_t eepromReadBuf[AT24C02_EEPROM_SIZE];

void eeprom_readall() {
	HAL_I2C_Mem_Read(&hi2c2, I2C_ADDR_24C02 << 1, 0, I2C_MEMADD_SIZE_8BIT, eepromReadBuf, AT24C02_EEPROM_SIZE, 100);
	print_mem_8(eepromReadBuf, AT24C02_EEPROM_SIZE);
}
