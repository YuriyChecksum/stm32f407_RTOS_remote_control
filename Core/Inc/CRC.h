/*
 * CRC.h
 *
 *  Created on: Oct 3, 2024
 *  Author: Maltsev Yuriy
 */
// функции вычисления CRC для разных случаев.
// писал на основе документации из вики, а так же из даташитов на разные датчики требующие подсчёта CRC
// в большинстве случаев указывается начальное инициализирующее число, полином, инверсия и разрядность.
// https://ru.wikibooks.org/wiki/Реализации_алгоритмов/Циклический_избыточный_код

#ifndef INC_CRC_H_
#define INC_CRC_H_

#ifndef _CRC_8_16_32_H
#define _CRC_8_16_32_H
#endif

#include <stdint.h>
#include <stddef.h>

// CRC-32 (табличная реализация)
extern const unsigned long crc_table[16];

// CRC-32
unsigned long crc32(uint8_t *data, size_t len);
void crc32stream(uint8_t data, unsigned long *crc);

// CRC-8 для TMC220x
void crc8_TMC220x(unsigned char *data, unsigned char len);

// CRC-8 (полином 0x31)
unsigned char crc8(unsigned char *data, unsigned int len);
uint8_t crc8_2(const uint8_t *data, size_t len);

// CRC-16 CCITT
unsigned short crc16(unsigned char *data, unsigned short len);

// CRC-16 Modbus
uint16_t crc16_modbus_rtu(unsigned char *data, unsigned char length);

#endif /* INC_CRC_H_ */
