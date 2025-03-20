/*
 * LCD_LPH8731.h
 *
 *  Created on: May 27, 2024
 *      Author: Admin
 *      на основе drzasiek,  styczeс 2012  drzasiek90@wp.pl  www.drzasiek.strefa.pl
 */

#ifndef SRC_LCD_LPH8731_H_
#define SRC_LCD_LPH8731_H_

#ifdef __cplusplus
extern "C" {
#endif
// **************************************************************
#include "main.h"
#include <stdio.h>    // printf

// **************************************************************
#define COLOR_8 // LCD_COLOR_8

#ifdef COLOR_8
//8bit
	#define WHITE 0xFF
	#define BLACK 0x00
	#define RED   0xE0
	#define GREEN 0x1C
	#define BLUE  0x03
	#define CYAN  0x1F
	#define MAGENTA 0xE3
	#define YELLOW 0xFC
	#define BROWN 0xf1
	#define ORANGE 0xEC
	#define PINK  0xEA
#else
//256bit
	#define WHITE 0xFFF
	#define BLACK 0x000
	#define RED 0xF00
	#define GREEN 0x0F0
	#define BLUE 0x00F
	#define CYAN 0x0FF
	#define MAGENTA 0xF0F
	#define YELLOW 0xFF0
	#define BROWN 0xB22
	#define ORANGE 0xFA0
	#define PINK 0xF9E
#endif

#define COLUMN_SMALL_MAX  16  // 0-16  количество столбцов для данного дисплея считая от 0
#define ROW_SMALL_MAX      9  // 0- 9  количество строк для данного дисплея считая от 0

#define CMD (  0 )
#define DAT ( ~0 )	//TRUE,  это работает на разноразрядных машинах

//typedef unsigned char U8;
//typedef unsigned int  U16;
//typedef unsigned long U32;

extern uint16_t ColorBack;
extern uint16_t ColorInk;

typedef enum {
	SMALL = 1, BIG
} size_print;

// **************************************************************
void Lcd_test(void);

void lcd_prn(char*);

void WriteToLCD(uint8_t w, uint8_t datt);
void InitLCD(void) /*siemens m55 lph8731-3C */;
void DrawPixel(uint8_t XPos, uint8_t YPos);
void DrawPixel_col(uint8_t XPos, uint8_t YPos, uint16_t Color);
void window_lcd(unsigned char xstart, unsigned char ystart, unsigned char xend, unsigned char yend);
void cursor_lcd(unsigned char row, unsigned char col);
void clear_lcd(uint16_t Color);
void contrast_lcd_set(unsigned char contrast);
void line_lcd(unsigned char x1,unsigned char y1, unsigned char x2, unsigned char y2);
void DrawLineX(uint8_t);
void Rectangle_lcd(unsigned char x,unsigned char y, unsigned char x2, unsigned char y2, unsigned char size);
void Circle_lcd(unsigned char xc,unsigned char yc,unsigned char r);
void printchr_lcd(char chr, uint8_t column, uint8_t row, size_print size, uint8_t fill );
void printstr_lcd(char *str, uint8_t column, uint8_t row, size_print size, uint8_t fill);
void printstr_lcd_p(const char* str_p, uint8_t column, uint8_t row, size_print size, uint8_t fill );
void print_bin_lcd(char b, uint8_t column, uint8_t row);
// **************************************************************
#ifdef __cplusplus
}
#endif

#endif /* SRC_LCD_LPH8731_H_ */
