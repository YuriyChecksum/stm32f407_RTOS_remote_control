
#include "buttons.h"

/*  GPIO  */
// имена пинов прописаны в main.h в разделе Private defines, можно указывать прямо в GUI
#define KB_GPIO_Port  BT_U_GPIO_Port //GPIOD
#define PIN_UP        BT_U_Pin   // GPIO_PIN_10
#define PIN_SET       BT_SET_Pin // GPIO_PIN_11
#define PIN_DOWN      BT_D_Pin   // GPIO_PIN_12
#define PIN_RIGHT     BT_R_Pin   // GPIO_PIN_13
#define PIN_MID       BT_M_Pin   // GPIO_PIN_14
#define PIN_LEFT      BT_L_Pin   // GPIO_PIN_15
//#define PIN_RESET     BT_RES_Pin // GPIO_PIN_??

// #define KB_CHECK(_KB_PIN, _KB_OUT) (((gpioIDR & _KB_PIN) < (GPIO_lastState & _KB_PIN)) ? _KB_OUT : 0)
// out |= KB_CHECK(KB_PIN_LEFT,  KB_LEFT);
// out |= KB_CHECK(KB_PIN_RIGHT, KB_RIGHT);
// out |= KB_CHECK(KB_PIN_UP,    KB_UP);
// out |= KB_CHECK(KB_PIN_DOWN,  KB_DOWN);
// out |= KB_CHECK(KB_PIN_SET,   KB_SET);

/*
GPIO_PinState lastState = GPIO_PIN_RESET; // хранит состояние кнопки
GPIO_PinState pinState = HAL_GPIO_ReadPin(But1_GPIO_Port, But1_Pin);
if(pinState != lastState) {
	_outln( pinState ? "1" : "0");
	lastState = pinState;
}*/

/*
# макросы
#define SET_BIT(REG, BIT)     ((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))
#define READ_BIT(REG, BIT)    ((REG) & (BIT))
#define CLEAR_REG(REG)        ((REG) = (0x0))
#define WRITE_REG(REG, VAL)   ((REG) = (VAL))
#define READ_REG(REG)         ((REG))
#define MODIFY_REG(REG, CLEARMASK, SETMASK)  WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))
#define POSITION_VAL(VAL)     (__CLZ(__RBIT(VAL)))
*/

/*
(HAL_GPIO_ReadPin(But1_GPIO_Port, But1_Pin) == GPIO_PIN_SET) ? _outln("1") : _outln("0");
if ((KB_GPIO_Port->IDR & But1_Pin) != (uint32_t)GPIO_PIN_RESET) {}  // взято из HAL_GPIO_ReadPin
*/

uint32_t GPIO_lastState = 0u;
uint16_t KB_last = KB_NULL;

/*
 * Если кнопки на разных портах, то придётся переписывать функцию
 * if (Keyboard_read() & KB_LEFT)  { printf(" KB_LEFT");}
 */
uint16_t Keyboard_read(){

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

	// out |= ((gpioIDR & PIN_LEFT) < (GPIO_lastState & PIN_LEFT)) ? KB_LEFT : 0;

	GPIO_lastState = gpioIDR;
	KB_last = out;
	return out;
}

/* Тестирует нажатия кнопок выводом значений в консоль */
void Keyboard_test(uint16_t kb) {
	if (kb == KB_NULL)  { return;} // не нажато ни одной, либо нажата и удерживается
	if (kb & KB_LEFT)  { printf(" KB_LEFT");}
	if (kb & KB_RIGHT) { printf(" KB_RIGHT");}
	if (kb & KB_UP)    { printf(" KB_UP");}
	if (kb & KB_DOWN)  { printf(" KB_DOWN");}
	if (kb & KB_MID)   { printf(" KB_MID");}
	if (kb & KB_SET)   { printf(" KB_SET");}
	if (kb & KB_RESET) { printf(" KB_RESET");}
	if (kb != 0)       { printf(", %x\r\n", kb);} // нажата любая кнопка
}

// **************************************************************
