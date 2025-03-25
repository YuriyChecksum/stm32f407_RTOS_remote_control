/*
 * utils.c
 *
 *  Created on: May 10, 2024
 *  Author: Maltsev Yuriy
 */
#include "utils.h"

/*Выводит в Serial значения переменной в стиле HEX-редакторов.
 Принимает на вход адрес переменной uint32, значение uint16 и ширину адреса при печати.
 Если адрес кратен 16 (0x0f), то выводит новую строку и значение адреса,
 иначе просто продолжает выводить значение в HEX.
 Парамер w определяет как печатать адрес двухбайтным (true) или однобайтным (false).*/
void prnBuff_8(uint32_t addr, uint8_t val) {
	// начинаем с новой строки после каждых 16 прочитанных байт
	if ((addr & (uint32_t)0x0F) == 0) { // возможно медленней if((memaddr % 16 == 0))
		printf("\r\n%04X:  ", (uint16_t)addr);
	}
	printf("%02X ", val);
}

// Выводит содержимое буфера в Serial в стиле hex-редакторов
void print_mem_8(uint8_t* ptr, size_t len) {
	//Serial << "\n memory for '" << name << "':\t len: " << len << ", *ptr: " << (uint32_t)ptr << endl;
	for(int i=0; i<len; i++){
		if ((i & (uint32_t)0x0F) == 0) { // возможно медленней if((memaddr % 16 == 0))
			printf("\r\n%04X:  ", (uint16_t)i);
		}
		printf("%02X ", ptr[i]);
	}
	printf("\r\n\r\n");
}

// Преобразует входящий символ [0..F] в число int
uint8_t hex2byte(char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return 10 + c - 'a';
	else if (c >= 'A' && c <= 'F')
		return 10 + c - 'A';
	return 0;
}

/* Преобразует входящую строку из 4х шестнадцатеричных символов [0..F] в число uint16_t
 * uint16_t n = hex2uint("1BAA");
 * Не проверяет допустимые символы и окончание строки!
 */
uint16_t hex2uint(char *s) {
	uint16_t out = 0;
	out |= hex2byte(*s++) << 12;
	out |= hex2byte(*s++) << 8;
	out |= hex2byte(*s++) << 4;
	out |= hex2byte(*s++);
	return out;
}

/* Преобразует входящую строку из (n <= 8)
 * шестнадцатеричных символов [0..F] в число uint32_t
 * uint32_t n = hex2ul("0A1B2CAA", 8);
 */
uint32_t hex2ul(char *s, uint8_t n) {
	uint32_t out = 0;
	if (n > 8)
		n = 8;
	for (uint8_t i = 0; i < n; ++i) {
		char c = s[i];
		//if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
		if (!isxdigit(c)) {
			break;
		}
		out = out << 4;
		out |= hex2byte(c) & 0x0F;
	}
	return out;
}
