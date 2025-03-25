/*
 * utils.h
 *
 *  Created on: May 10, 2024
 *  Author: Maltsev Yuriy
 */

#ifndef INC_UTILS_H_
#define INC_UTILS_H_

#include "main.h"
#include <stdbool.h>
#include <ctype.h>   // isxdigit isspace toupper
#include <stdio.h>

void prnBuff_8(uint32_t, uint8_t);
void print_mem_8(uint8_t*, size_t);
uint8_t hex2byte(char);
uint16_t hex2uint(char*);
uint32_t hex2ul(char*, uint8_t);
uint8_t hex2byte2(uint8_t, uint8_t);

#endif /* INC_UTILS_H_ */
