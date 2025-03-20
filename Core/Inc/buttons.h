/*
 * buttons.h
 *
 *  Created on: May 10, 2024
 *  Author: Maltsev Yuriy
 *  Version 1.1
 */

#ifndef INC_BUTTONS_H_
#define INC_BUTTONS_H_

#include "main.h"
#include <stdio.h>    // printf
// **************************************************************

/**
  * @brief  Keyboard or joystic keys enumeration
  */
typedef enum
{
  KB_NULL    = 0u,
  KB_LEFT    = ((uint16_t)0x0001), // ((uint16_t)(1 << 0))
  KB_RIGHT   = ((uint16_t)0x0002), // ((uint16_t)(1 << 1))
  KB_UP      = ((uint16_t)0x0004), // ((uint16_t)(1 << 2))
  KB_DOWN    = ((uint16_t)0x0008), // ((uint16_t)(1 << 3))
  KB_MID     = ((uint16_t)0x0010), // ((uint16_t)(1 << 4))
  KB_SET     = ((uint16_t)0x0020), // ((uint16_t)(1 << 5))
  KB_RESET   = ((uint16_t)0x0040), // ((uint16_t)(1 << 6))
} KB_State;


/* Exported functions prototypes ---------------------------------------------*/
uint16_t Keyboard_read();
void Keyboard_test(uint16_t);

#endif /* INC_BUTTONS_H_ */
