/*
 * eeprom.h
 *
 *  Created on: May 10, 2024
 *  Author: Maltsev Yuriy
 */

#ifndef INC_EEPROM_H_
#define INC_EEPROM_H_

#include "main.h"
#include <stdbool.h>
#include <ctype.h>   // isxdigit isspace toupper
#include <stdio.h>   // printf

#define I2C_ADDR_24C02    0x52  // 24C02 (A0 & A2 = GND, A1 = Vcc) adress: b1010010 = 0x52 = dec:82
#define AT24C02_EEPROM_SIZE        256
#define AT24C02_EEPROM_PAGE_SIZE     8

extern uint8_t eepromReadBuf[];
//uint8_t eepromReadBuf[AT24C02_EEPROM_SIZE];

void eeprom_readall();

void prnBuff_8(uint32_t, uint8_t);
void print_mem_8(uint8_t*, size_t);
uint8_t hex2byte(char);
uint16_t hex2uint(char*);
uint32_t hex2ul(char*, uint8_t);
uint8_t hex2byte2(uint8_t, uint8_t);

#endif /* INC_EEPROM_H_ */
