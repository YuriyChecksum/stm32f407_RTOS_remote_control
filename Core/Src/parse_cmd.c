/*
 * parse_cmd.c
 *
 *  Created on: Mar 21, 2025
 *      Author: Maltsev Yuriy
 */
#include "parse_cmd.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>   // atoi
#include "utils.h"
#include "uart.h"
#include "crc.h"
#include "LCD_LPH8731.h"
#include "buttons.h"
#include "i2c.h"
#include "bmp280.h"
#include "ATH25.h"
#include "testspeedcopy.h"
#include "task_05.h"
//#include "mt6701.h"

#define hI2C &hi2c2

// парсим односимвольные команды
void command_chr(char *str) {
	char c = *str;
	switch (c) {
	case 'c':
		printf("Test LCD LPH8731\r\n");
		lcd_test();
		break;
	case 'r':
#ifdef INC_MT6701_H_
		printf("mt6701 read\r\n");
		mt6701_read_test();
#endif
#ifdef INC_BMP280_H_
		printf("BMP280 read\r\n");
		bmp280_Read_All();
#endif
		break;
	case 'R':
#ifdef INC_BMP280_H_
		printf("BMP280 read press and temp\r\n");
		bmp280_ReadPT();
#endif
		break;
	case 't':
#ifdef INC_TESTSPEEDCOPY_H_
		printf("Test speed data copy\r\n");
		testspeedcopy();
#endif
		break;
	case 'h':
#ifdef INC_ATH25_H_
		printf("ATH25 Test\r\n");
		ATH25_test();
#endif
		break;
	case 's':
		printf("Scan I2C\r\n");
		scanI2C();
		break;
	case 'q':
		printf("TIM4 stop\r\n");
		HAL_TIM_OC_Stop(&htim4, TIM_CHANNEL_4);
		break;
	case 'Q':
		printf("TIM4 run\r\n");
		HAL_TIM_OC_Start(&htim4, TIM_CHANNEL_4); // запускаем таймер
		break;
	case 'e':
		printf("Read EEPROM 24C02\r\n");
		//eeprom_readall();
		break;
	case '1':
		printf("Command: Keyboard_test\r\n");
		uint16_t kb = buttons_read();
		printf(" %x\r\n", kb);
		buttons_test_msg(kb);
		break;
	default:
		break;
	}
}

/* парсим бинарные команды
 * (хорошо подходит для передачи больших фреймов данных, например динамическая подсветка RGB ленты).
 * Принять бинарный поток данных без символа \n для управления адресной светодиодной лентой
 * message format: ['*'][idx_led (1 byte)][size data (2 bytes)][data r,g,b (3*n bytes)]
 */
void command_bin(char *str) {
	USART2_sendstr("Recive bin\r\n");

	/*
	// Пример для RGB ленты
	uint16_t len, count, i;
	uint16_t n;
	len = 0;
	count = 0;
	n = 0;
	i = 1; // пропустим первый символ '*'
	if (lenRxData >= 3) {
		n = (uint8_t) str[i++];
		len = ((uint8_t) str[i++] << 8) | (uint8_t) str[i++];
	}
	while (i < lenRxData && count < len) {
		count += 3;
		if (n <= strip.numPixels()) {
			//(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
			strip.setPixelColor(n++, str[i++], str[i++], str[i++]);
		}
	}
	*/
}

bool nextArg(char **token, uint8_t len, uint8_t isHex) {
	*token = strtok(NULL, " ");
	//printf("nextArg: %s\r\n", *token);
	if (*token == NULL) {
		printf("arg is NULL\r\n");
		return false;
	} else if (strlen(*token) != len) {
		printf("arg len != %d, token: \'%s\'\r\n", len, *token);
		return false;
	} else if (isHex == 1) {
		for (uint8_t i = 0; i < len; ++i) {
			char ch = (*token)[i];
			if (!isxdigit(ch)) { // проверяет, является ли c шестнадцатеричной цифрой
				printf("arg not hex: \'%s\'\r\n", *token);
				return false;
			}
		}
	}
	return true;
}


/** Команды для управления переферией через UART
_i2c bus_addr7_hex  'w/r' len_addr[0..2] reg_addr16_hex len_data16_hex *data_hex
_i2c 52 w 1 0010 0003 0A1B2C
_i2c 52 r 1 0000 0020
_i2c 52 r 0 0000 0010

_i2c 57 w 2 0010 0003 0A1B2C
_i2c 57 r 2 0000 00ff

DS3231
_i2c 68 r 1 0000 0013                       DS3231 read all registers
_i2c 68 w 1 0000 0007 00000000000000        DS3231 clear datetime
_i2c 68 w 1 0000 0007 00521800030423        DS3231 write datetime dt[0:6] c:m:h:00:d:m:y
_i2c 68 w 1 0000 0003 005218                DS3231 write time dt[0:2] c:m:h

INA226
_i2c 40 r 1 0001 0002   INA226 SHUNT_VOLTAGE  sign16 (1 LSB = 2.5 uV) R=0.1 Om
_i2c 40 r 1 0002 0002   INA226 BUS_VOLTAGE  unsign16 (1 LSB = 1.25 mV)

адреса:
#define I2C_ADDR_24C02  0x52  // AT24C02, (A0 & A2 = GND, A1 = Vcc) I2C address: 0x52 = b1010010 = dec: 82
#define I2C_ADDR_24C32  0x57  // AT24C32,I2C address 0x57, bin: 01010111, dec: 87  (4096 x 8) page 32 bytes
#define I2C_ADDR_INA226 0x40  // INA226, I2C address 0x40, bin: 01000000
#define I2C_ADDR_OLED   0x3C  // OLED,   I2C address 0x3C, bin: 00111100, dec: 60
#define I2C_ADDR_DS3231 0x68  // RTC_DS3231, address 0x68, bin: 01101000, dec: 104
*/

// парсим строковые команды
void command_str(char *str) {
	USART2_sendstr("Recive: \'");
	USART2_sendstr(str); // echo
	USART2_sendstr("\'\r\n");

	char *token; // указатель на подстроку в str после вызова strtok
	token = strtok(str, " "); // get command
	// TO DO: не понимает команды, если в конце не добавить пробел

	if (token == NULL) {
		printf("cmd token is NULL. str=%s\r\n", str);
		return;
	} else {
		printf("cmd token: \'%s\'\r\n", token);
	}

	if (strcmp(token, "test") == 0) {
		uint32_t startTime = HAL_GetTick();
		printf("cmd: test\r\ntime(ms): %d\r\n", (int) (HAL_GetTick() - startTime));
	} else if (strcmp(token, "task") == 0) {
		token = strtok(NULL, " ");
		if (token == NULL) {
			printf("task arg is NULL\r\n");
			return;
		}
		if (!strcmp(token, "bmp280")) {
			if (!nextArg(&token, 1, 0))
				return;
			char cmd = *token;
			printf("cmd arg: \'%c\'\r\n", cmd);

			switch (cmd) {
			case 'p':
				printf("cmd pause task change\r\n");
				// pause();
				printf("Task BMP280 Run/Stop\r\n");
				// if (t_BMP280 != NULL && t_BMP280->task != NULL) {
				// 	t_BMP280->pause = (t_BMP280->pause) ? 0 : 1;
				// }
				break;
			case 't':
				token = strtok(NULL, " ");
				// uint32_t period = (uint32_t) atoi(token);
				// if (t_BMP280 != NULL && t_BMP280->task != NULL && period > 0) {
				// 	t_BMP280->period = period;
				// }
				break;
			default:
				break;
			}
		} else {
			printf("task name not exist\r\n");
			return;
		}
	} else if (strcmp(token, "i2c") == 0) {
		uint32_t startTime = HAL_GetTick();

		if(!nextArg(&token, 2, 1)) return;
		uint8_t busaddr = (hex2byte(token[0]) << 4) | hex2byte(token[1]); // адрес микросхемы
		printf("arg1 bus_addr: %d, 0x%02x\r\n", busaddr, busaddr);

		if(!nextArg(&token, 1, 0)) return;
		char wr = *token;
		printf("arg2 wr: \'%c\'\r\n", wr);

		if(!nextArg(&token, 1, 0)) return;
		uint8_t memaddrsize = (uint8_t) atoi(token); //memaddrsize=0,1,2
		printf("arg3 mem addr size: %d\r\n", memaddrsize);

		if(!nextArg(&token, 4, 1)) return; // адрес регистра
		//uint16_t memaddr = hex2ul(token, 4);
		uint16_t memaddr = (hex2byte(token[0]) << 12) | (hex2byte(token[1]) << 8)
				| (hex2byte(token[2]) << 4) | hex2byte(token[3]);
		printf("arg4 addr reg: %d, 0x%04x\r\n", memaddr, memaddr);

		//if(!nextArg(&token, 4, 1)) return; // пока так, нужно сделать чтобы не искал \0
		token = strtok(NULL, " ");
		uint16_t len_data = hex2ul(token, 4);
		printf("arg5 len_data: %d, 0x%04x\r\n", len_data, len_data);

		/* write */
		if ((wr == 'w') || (wr == 'W')) {
			token += 5; // с учётом длины предыдущего параметра len_data
			uint8_t buffW[len_data];
			for (int i = 0; i < len_data; ++i) {
				char chH = *token++;
				/* isspace(c): проверяет на ' ', '\n', '\r', перевод страницы ('\f'),
				 * горизонтальная ('\t') или вертикальная ('\v') табуляция.
				 *
				 * isxdigit(c) - является ли c шестнадцатеричной цифрой
				 * */
				if (!isxdigit(chH)) {
					printf("Error: данных меньше чем указывалось. len_data: %d, i: %d\r\n",
							len_data, i);
					print_mem_8(buffW, len_data);
					return;
				}
				char chL = *token++;
				if (!isxdigit(chL)) {
					printf("Error: данных меньше чем указывалось. len_data: %d, i: %d\r\n",
							len_data, i);
					print_mem_8(buffW, len_data);
					return;
				}
				buffW[i] = (hex2byte(chH) << 4) | hex2byte(chL);
			}

			// if (len_data != 1) {
			// 	printf("Error: arg5 len_data != 1\r\n");
			// 	return;
			// }
			// if (!nextArg(&token, 2)) return;
			// uint8_t data = (hex2int(token[0]) << 4) | hex2int(token[1]);
			// printf("arg data: %d, 0x%02x\r\n", data, data);

			uint32_t crc = ~0L; // crc Init  : 0xFFFFFFFF
			printf("data1: [");
			for (int i = 0; i < len_data; i++) {
				printf("%02X ", buffW[i]);
				crc32stream(buffW[i], &crc);
			}
			printf("]\r\nCRC START\r\n%x\r\nCRC END\r\n", (unsigned int)crc);

			printf("\r\ndata2: [");
			print_mem_8(buffW, len_data);
			uint32_t crc2 = crc32(buffW, len_data);
			printf("]\r\nCRC START\r\n%x\r\nCRC END\r\n", (unsigned int)crc2);

			if (len_data == 1)
				printf("\r\nWrite to I2C: [dev] 0x%02x -> [mem] 0x%02x = [data] 0x%02x\r\n",
						busaddr, memaddr, buffW[0]);
			else
				printf("\r\nWrite to I2C: [dev] 0x%02x -> [mem] 0x%02x..0x%02x\r\n",
						busaddr, memaddr, memaddr + len_data);

			if (memaddrsize == 1) {
				HAL_I2C_Mem_Write(hI2C, busaddr << 1, memaddr, I2C_MEMADD_SIZE_8BIT,  buffW, len_data, 100);
			} else if (memaddrsize == 2) {
				HAL_I2C_Mem_Write(hI2C, busaddr << 1, memaddr, I2C_MEMADD_SIZE_16BIT, buffW, len_data, 100);
			} else {
				printf("Error: memaddrsize not 1,2: %d", memaddrsize);
			}

			// TO DO: убрать после отладки BMP280
			if (busaddr == 0x76) {
				// bmp280_write(memaddr, data);
				HAL_Delay(100);
				bmp280_Read_All();
			}
		}
		/* read */	// example: i2c 76 r 1 0000 0100    i2c 76 r 1 00f5 0001
		else if ((wr == 'r') || (wr == 'R')) {
			if (len_data == 0) {
				printf("Error: len_data = 0\r\n");
				return;
			}
			uint8_t readbuf[len_data];

			if (memaddrsize == 0) { // принять по i2c не указывая адрес ячейки, полезно при чтении станицами и для некоторых микросхем
				HAL_StatusTypeDef st = HAL_I2C_Master_Receive(hI2C, busaddr << 1, readbuf, len_data, 200); // HAL_MAX_DELAY
				if (st != HAL_OK) {
					printf("Error: HAL status HAL_I2C_Master_Receive = %d", st);
					return;
				}
			} else if (memaddrsize == 1) {
				//HAL_I2C_Mem_Write(H_I2C, busaddr << 1, memaddr, I2C_MEMADD_SIZE_8BIT,  buffW, len_data, 100);
				HAL_StatusTypeDef st = HAL_I2C_Mem_Read(hI2C, busaddr << 1, memaddr, I2C_MEMADD_SIZE_8BIT, readbuf, len_data, 200);
				if (st != HAL_OK) {
					printf("Error: HAL status I2C_Mem_Read = %d", st);
					return;
				}
			} else if (memaddrsize == 2) {
				HAL_StatusTypeDef st = HAL_I2C_Mem_Read(hI2C, busaddr << 1, memaddr, I2C_MEMADD_SIZE_16BIT, readbuf, len_data, 200);
				if (st != HAL_OK) {
					printf("Error: HAL status I2C_Mem_Read = %d", st);
					return;
				}
			} else {
				printf("Error: memaddrsize not 0,1,2: = %d", memaddrsize);
				return;
			}

			unsigned long crc = crc32(readbuf, len_data);
			printf("DATA START");
			print_mem_8(readbuf, len_data);
	        printf("DATA END\r\nCRC START\r\n%x\r\nCRC END\r\n", (unsigned int)crc);
		}

		uint32_t endTime = HAL_GetTick();
		printf("Time(ms): %d\r\n\r\n", (int)(endTime - startTime));
	} else if (strcmp(token, "i2c2") == 0) {
		printf("cmd: i2c2\r\n");
	} else {
		printf("cmd not exist. token: \'%s\'\r\n", token);
		return;
	}
}

