/*
 * i2c.h
 *
 *  Created on: May 10, 2024
 *      Author: Admin
 */

#ifndef INC_I2C_H_
#define INC_I2C_H_

//#include <string.h> // strlen()

//#include "stm32f1xx_hal.h"
//#include "stm32f1xx_hal_conf.h"
//#include "main.c"

//extern I2C_HandleTypeDef hi2c1;
//extern I2C_HandleTypeDef hi2c2;

//extern char sprintfBuffer[256];
//extern void _out(const char*);
//extern void _outln(const char*);

void scanI2C();
void i2c_sw_reset();

#endif /* INC_I2C_H_ */
