/*
 * LCD_LPH8731.h
 *
 *  Created on: May 27, 2024
 *      Author: Maltsev Yuriy
 *      на основе drzasiek,  styczeс 2012  drzasiek90@wp.pl  www.drzasiek.strefa.pl
 */
// Драйвер для цветного TFT дисплея LPH8731-3C от мобильника Siemens m55
// controller support 104x82px -> m55 101x80px.
// Оригинального даташита нету, но можно найти на родственные контроллеры.
// Поэтому писал используя константы и последовательность инициализации взятые с форумов.
// Дописал функции рисования. Таблицу символов взял с какого-то чужого исходника.

#ifndef SRC_LCD_LPH8731_H_
#define SRC_LCD_LPH8731_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define LCD_BUFF_LEN 100

#define COLOR_8

#ifdef COLOR_8
//8 bit format RRRGGGBB
	#define WHITE   0xFF
	#define BLACK   0x00
	#define RED     0xE0
	#define GREEN   0x1C
	#define BLUE    0x03
	#define CYAN    0x1F
	#define MAGENTA 0xE3
	#define YELLOW  0xFC
	#define BROWN   0xf1
	#define ORANGE  0xEC
	#define PINK    0xEA
#else
//12 bit
	#define WHITE   0x0FFF
	#define BLACK   0x0000
	#define RED     0x0F00
	#define GREEN   0x00F0
	#define BLUE    0x000F
	#define CYAN    0x00FF
	#define MAGENTA 0x0F0F
	#define YELLOW  0x0FF0
	#define BROWN   0x0B22
	#define ORANGE  0x0FA0
	#define PINK    0x0F9E
#endif

#define COLUMN_SMALL_MAX  16  // 0-16 количество столбцов для данного дисплея считая от 0
#define ROW_SMALL_MAX      9  // 0-9  количество строк для данного дисплея считая от 0

#define CMD (  0 )
#define DAT ( ~0 )

extern char buffLcd[LCD_BUFF_LEN];
extern uint16_t color_back;
extern uint16_t color_ink;

typedef enum {
	SMALL = 1, BIG
} size_print;

// **************************************************************
void lcd_init(void);
void lcd_test(void);
void lcd_prn(char*);
void lcd_write(uint8_t mode, uint8_t data);
void lcd_draw_pixel(uint8_t XPos, uint8_t YPos);
void lcd_draw_pixel_color(uint8_t XPos, uint8_t YPos, uint16_t Color);
void lcd_set_window(unsigned char xstart, unsigned char ystart, unsigned char xend, unsigned char yend);
void lcd_set_cursor(unsigned char row, unsigned char col);
void lcd_set_contrast(unsigned char contrast);
void lcd_clear(uint16_t Color);
void lcd_draw_line_x(uint8_t);
void lcd_draw_line(unsigned char x1,unsigned char y1, unsigned char x2, unsigned char y2);
void lcd_draw_rectangle(unsigned char x,unsigned char y, unsigned char x2, unsigned char y2, unsigned char size);
void lcd_draw_circle(unsigned char xc,unsigned char yc,unsigned char r);
void lcd_print_chr(char chr, uint8_t column, uint8_t row, size_print size, uint8_t fill );
void lcd_print_str(char *str, uint8_t column, uint8_t row, size_print size, uint8_t fill);
void lcd_print_str_p(const char* str_p, uint8_t column, uint8_t row, size_print size, uint8_t fill );
void lcd_print_bin(char b, uint8_t column, uint8_t row);

// **************************************************************
#ifdef __cplusplus
}
#endif

#endif /* SRC_LCD_LPH8731_H_ */
