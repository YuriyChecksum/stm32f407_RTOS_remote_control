
#include "buttons.h"
#include <stdio.h>

/*  GPIO  */
// имена пинов в main.h в разделе Private defines, можно прописать в GUI
#define KB_GPIO_Port  BT_U_GPIO_Port //GPIOD
#define PIN_UP        BT_U_Pin   // GPIO_PIN_10
#define PIN_SET       BT_SET_Pin // GPIO_PIN_11
#define PIN_DOWN      BT_D_Pin   // GPIO_PIN_12
#define PIN_RIGHT     BT_R_Pin   // GPIO_PIN_13
#define PIN_MID       BT_M_Pin   // GPIO_PIN_14
#define PIN_LEFT      BT_L_Pin   // GPIO_PIN_15
//#define PIN_RESET     BT_RS_Pin // GPIO_PIN_?? не разведён

uint32_t GPIO_lastState = 0u;
uint16_t KB_last = KB_NULL;

/*
 * Если кнопки подключены на разных портах, то придётся переписывать функцию.
 * И добавить программную защиту от дребезга,
 * если не подключены шунтирующие конденсаторы к выводам.
 */
uint16_t buttons_read(){

	uint32_t gpioIDR = KB_GPIO_Port->IDR & (PIN_DOWN|PIN_UP|PIN_LEFT|PIN_RIGHT|PIN_SET|PIN_MID);
	if (gpioIDR == GPIO_lastState)
		return KB_NULL;

	uint16_t out = KB_NULL;
	if ((gpioIDR & PIN_LEFT)  < (GPIO_lastState & PIN_LEFT))  out |= KB_LEFT;
	if ((gpioIDR & PIN_RIGHT) < (GPIO_lastState & PIN_RIGHT)) out |= KB_RIGHT;
	if ((gpioIDR & PIN_UP)    < (GPIO_lastState & PIN_UP))    out |= KB_UP;
	if ((gpioIDR & PIN_DOWN)  < (GPIO_lastState & PIN_DOWN))  out |= KB_DOWN;
	if ((gpioIDR & PIN_MID)   < (GPIO_lastState & PIN_MID))   out |= KB_MID;
	if ((gpioIDR & PIN_SET)   < (GPIO_lastState & PIN_SET))   out |= KB_SET;
	//if ((gpioIDR & PIN_RESET) < (GPIO_lastState & PIN_RESET)) out |= KB_RESET;

	GPIO_lastState = gpioIDR;
	KB_last = out;
	return out;
}

/* Тестирует нажатия кнопок выводом значений в консоль */
void buttons_test_msg(uint16_t kb) {
	if (kb == KB_NULL)  { return;} // не нажато ни одной, либо нажата и удерживается
	if (kb & KB_LEFT)  { printf(" KB_LEFT");}
	if (kb & KB_RIGHT) { printf(" KB_RIGHT");}
	if (kb & KB_UP)    { printf(" KB_UP");}
	if (kb & KB_DOWN)  { printf(" KB_DOWN");}
	if (kb & KB_MID)   { printf(" KB_MID");}
	if (kb & KB_SET)   { printf(" KB_SET");}
	if (kb & KB_RESET) { printf(" KB_RESET");}
	// при любом нажатии вывести hex значение переменной
	if (kb != 0)       { printf(", %x\r\n", kb);}
}

