/*
 * i2c.h
 *
 *  Created on: May 10, 2024
 *      Author: Admin
 */

#ifndef INC_I2C_H_
#define INC_I2C_H_

#include <stdint.h>
#include "utils.h"

#define I2C_ADDR_24C02 0x52  // 24C02 (pin A0 & A2 = GND, A1 = Vcc) adress: b1010010 = 0x52 = dec:82
#define AT24C02_EEPROM_SIZE        256
#define AT24C02_EEPROM_PAGE_SIZE     8

extern uint8_t eepromReadBuf[AT24C02_EEPROM_SIZE];

void scanI2C();
void i2c_sw_reset();
void eeprom_readall();

#endif /* INC_I2C_H_ */
