/*
 * LCD_LPH8731.c
 *
 *  Created on: May 27, 2024
 *  Author: Maltsev Yuriy
 */
// Драйвер для цветного TFT дисплея LPH8731 от мобильника Siemens.
// Оригинального даташита нету, но можно найти на родственные контроллеры.
// Поэтому писал используя константы и последовательность инициализации с форумов.
// Дописал функции рисования, таблицу символов взял с какого-то чужого исходника.

#include "LCD_LPH8731.h"

/*
#define LCD_PORT        PORTB
#define LCD_DDR         DDRB
#define LCD_SCLK 	    (1<<PB7)
#define LCD_MOSI 		(1<<PB5) //master out slave inp
#define LCD_MISO		(1<<PB6)
#define LCD_RS          (1<<PB4)
#define LCD_RESET       (1<<PB3)
#define LCD_CS 	        (1<<PB2)
#define LCD_POWER 	    (1<<PB1)
*/

//#define sbi(port, bit) (port) |= (1 << (bit))  // Set \c bit in IO port \c port.
//#define cbi(port, bit) (port) &= ~(1 << (bit)) // Clear \c bit in IO port \c port.
#define sbi(port, pin) HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
#define cbi(port, pin) HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);

#define LCD_Port LCD_CS_GPIO_Port // GPIOE
#define PIN_CS   LCD_CS_Pin       // GPIO_PIN_11  // chip select
#define PIN_RES  LCD_RES_Pin      // GPIO_PIN_12  // reset
#define PIN_RS   LCD_RS_Pin       // GPIO_PIN_13  // select data|cmd
#define PIN_CLK  LCD_CLK_Pin      // GPIO_PIN_14  // serial clock
#define PIN_DAT  LCD_DAT_Pin      // GPIO_PIN_15  // serial data

#define SOFTWARE_SPI // SOFTWARE_SPI LCD_SOFTWARE_SPI

#define _WAIT 0
uint16_t ColorBack;
uint16_t ColorInk;

// Задержка в мс
void _delay_ms(uint16_t t){
	HAL_Delay(t);
}

// задержка для интерфейса N тактов
void _delay_us(uint16_t n) {
	while (n--) {
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
	}
}

char pgm_read_byte(const char* ptr) {
	return *ptr;
}

// функция тестирования кода и дисплея.
// Инициализирует, заливает цветами, рисует, выводит текст разного размера
void Lcd_test() {
	InitLCD();
	//test_lcd();
	clear_lcd(BLACK);
	_delay_ms(1000);
	clear_lcd(WHITE);
	_delay_ms(1000);
	clear_lcd(RED);
	_delay_ms(1000);
	clear_lcd(GREEN);
	_delay_ms(1000);
	clear_lcd(BLUE);
	_delay_ms(1000);
	clear_lcd(0xfff-1);
	_delay_ms(1000);
	clear_lcd(1);
	_delay_ms(1000);

	clear_lcd(BROWN);
	ColorInk=WHITE;
	char str1[] = " LCD Test!  TEXT _test1234567890-=+)";
	printstr_lcd(str1, 0, 0, 1, 0);

	_delay_ms(4000);

	clear_lcd(WHITE);
	ColorInk=BLACK;
	line_lcd(10, 10, 90, 10);
	ColorInk=BLUE;
	line_lcd(10, 14, 90, 14);
	ColorInk=GREEN;
	line_lcd(5, 5, 90, 40);
	ColorInk=RED;
	line_lcd(5, 15, 70, 60);
	line_lcd(0, 30, 100, 30);
	line_lcd(30, 0, 30, 79);
	_delay_ms(4000);

	Rectangle_lcd(5, 15, 20, 60, 3);
	ColorInk=GREEN;
	Rectangle_lcd(25, 15, 35, 40, 0);
	ColorInk=BLUE;
	Rectangle_lcd(42, 1, 50, 70, 1);
	Rectangle_lcd(0, 0, 100, 79, 1);
	_delay_ms(4000);
	ColorInk=RED;
	Circle_lcd(60, 28, 20);
	Circle_lcd(20, 50, 30);

	ColorInk = BROWN;
	uint8_t x2 = 97;
	uint8_t y2 =  0;
	while (y2 < 79)
	{
		line_lcd(6, 60, x2, y2);
		y2 += 3;
	}
	//union st6 {
		//uint8_t x = 10;
		//uint8_t y = 15;
		//};
	//uint8_t * p = st6->x;

	_delay_ms(2000);
	//printstr_p(PSTR(" str_lcd Привет! (LCD Test)\n"));

	ColorInk=BLACK;
	char str[] = " LCD Test! )";
	printstr_lcd(str, 1, 1, 1, 0);

	char *text_buf[3] = { "string 1 ",
					" TEXT _test1234567890-=+({//|\\})!qwertyuiop[]asdfghjkl;'zxcvbnm,./?",
					"AbcE23_asdfghjkl;qwerty" };
	_delay_ms(2000);

	ColorInk=WHITE;
	Rectangle_lcd(0,0,100,40,0);

	ColorInk=BLACK;
	ColorBack=YELLOW;
	printchr_lcd('2', 0, 0, 1, 1);
	ColorBack=0b00010000; //RRRGGGBB
	printstr_lcd(text_buf[0], 1, 0, 1, 1);
	printstr_lcd(text_buf[1], 0, 1, 1, 0);
	_delay_ms(2000);

	ColorBack=YELLOW;
	printstr_lcd(text_buf[2], 1, 3, 2, 1); //размер 2, макс 4 строки, 8 столбц
	_delay_ms(2000);


	//DrawManyPixels
	for (int i = 0; i < 101; i++)
		for (int j = 0; j < 80; j++) {
			DrawPixel_col(i, j, (i << 4) + (j << 4)); }
#ifndef COLOR_8
	for (uint16_t i=0; i<127; i++){
		clear_lcd(i<<5);
		_delay_ms(40);
	}

	for (uint16_t i=0; i<16; i++){
		clear_lcd(i);
		_delay_ms(40);
	}
	for (uint16_t i=0; i<16; i++){
		clear_lcd(i<<4);
		_delay_ms(40);
	}
	for (uint16_t i=0; i<16; i++){
		clear_lcd(i<<8);
		_delay_ms(40);
	}
	for (uint16_t i=0; i<16; i++){
		clear_lcd(i+(i<<4)+(i<<8));
		_delay_ms(40);
	}
#endif


	DrawPixel_col(0, 3, RED);
	DrawPixel_col(1, 4, BLACK);
	DrawPixel_col(2, 5, WHITE);
	DrawPixel_col(3, 6, YELLOW);
	DrawPixel_col(4, 7, RED);
	DrawPixel_col(5, 8, BLUE);

	for (uint8_t i=0; i<80; i++){
		DrawPixel_col(i, i, ((uint16_t)i)<<5);
	}

	_delay_ms(3000);
	clear_lcd(WHITE);

	window_lcd(0, 0, 100, 79);
	uint8_t i = 80;
	do
	{
		DrawPixel_col(i+10, 10, YELLOW);
		DrawPixel_col(i+10, 69, BLUE);
		DrawPixel_col(30, i, RED);
		DrawPixel_col(90, i, GREEN);
	} while (--i);
	_delay_ms(3000);

	uint8_t y = 0;
	for (uint16_t i=0; i<16; i++){
		ColorInk=i;
		DrawLineX(y);
		y++;
	}
	for (uint16_t i=0; i<16; i++){
		ColorInk= i<<4;
		DrawLineX(y);
		y++;
	}
	for (uint16_t i=0; i<16; i++){
		ColorInk=i<<8;
		DrawLineX(y);
		y++;
	}
	for (uint16_t i=0; i<16; i++){
		ColorInk=i+(i<<4)+(i<<8);
		DrawLineX(y);
		y++;
	}
	for (uint16_t i=0; i<16; i++){
		ColorInk=i+(i<<4)+0xf00;
		DrawLineX(y);
		y++;
	}
	_delay_ms(3000);

	//printstr_p(PSTR("\n Mode test 2 LCD...\n"));

	WriteToLCD( CMD, 0x21);    //Inversion ON
	_delay_ms(1000);
	WriteToLCD( CMD, 0x20);    //Inversion OFF
	_delay_ms(1000);
	WriteToLCD( CMD, 0x22);    //All pixels OFF / restore-cmd Normal display ON
	_delay_ms(1000);
	WriteToLCD( CMD, 0x23);    //All pixels ON  / restore-cmd Normal display ON
	_delay_ms(1000);
	WriteToLCD( CMD, 0x13);    //Normal display ON


	window_lcd(0, 0, 100, 79);

	for (uint16_t i=0; i<80; i++){
		ColorInk = (i<<1)+i;
		DrawLineX(i);
	}
	_delay_ms(4000);

	clear_lcd(WHITE);
	for (uint16_t i=0; i<80; i++){
		ColorInk = (i<<1)+i;
		line_lcd(3, i, 95, i);
	}

	////тест больших символов
	//ColorBack = BLUE;
	//printstr_lcd("123456789",0,0,BIG,1);
	clear_lcd(BLACK);
	ColorBack = BLACK;
	ColorInk = WHITE;
	//-------------------------
}

void lcd_prn(char* str) {
	/*
	// clear_lcd( uint16_t Color )
	window_lcd(0, 0, 100, 79);
	WriteToLCD( CMD, 0x2C );	// WR_MEM
	for(uint16_t i = 0; i < 8080; i++)	// 101x80
	{
		WriteToLCD(DAT, (uint8_t)Color);
	}
	ColorInk=WHITE;
	Rectangle_lcd(0,0,100,40,0);

	ColorBack=0b00010000; //RRRGGGBB
	char *text_buf[3] = { "string 1 ", "AbcE23_asdfghjkl;qwerty", " TEXT _test1234567890-=+({//|\\})!qwertyuiop[]asdfghjkl;'zxcvbnm,./?" };
	printstr_lcd(text_buf[0], 1, 0, 1, 1);
	printstr_lcd(text_buf[1], 0, 1, 1, 0);
	printstr_lcd(text_buf[2], 1, 3, 2, 1); //размер 2, макс 4 строки, 8 столбц
	_delay_ms(2000);
	*/

	ColorInk=BLACK;
	ColorBack=WHITE;
	printstr_lcd(str, 0, 0, 1, 1);
}

//w = TRUE - отправляем данные, FALSE - команду. datt = данные на отравку
//передаём от старшего бита к младшему
void WriteToLCD(uint8_t w, uint8_t datt) {
	_delay_us(_WAIT);
	sbi(LCD_Port, PIN_CS); //LCD_PORT |= LCD_CS;		// "CS" <- "1"
	_delay_us(_WAIT);
	// Если в дисплей передаём команду, то RS->0
	//w ? (LCD_PORT |= LCD_RS) : (LCD_PORT &= ~LCD_RS);
	//HAL_GPIO_WritePin(LCD_Port, PIN_RS, w ? GPIO_PIN_SET : GPIO_PIN_RESET);
	if (w != 0) {
		sbi(LCD_Port, PIN_RS);
	} else {
		cbi(LCD_Port, PIN_RS);
	}
	_delay_us(_WAIT);
	cbi(LCD_Port, PIN_CS); // LCD_PORT &= ~LCD_CS;
	_delay_us(_WAIT);
#ifdef SOFTWARE_SPI
	for (uint8_t i = 0; i < 8; i++) {
		cbi(LCD_Port, PIN_CLK);
		//HAL_GPIO_WritePin(LCD_Port, PIN_RS, ( datt & 0x80 ) ? GPIO_PIN_SET : GPIO_PIN_RESET);
		if ((datt & 0x80) != 0) {
			sbi(LCD_Port, PIN_DAT);
		} else {
			cbi(LCD_Port, PIN_DAT);
		}
		_delay_us(_WAIT);
		sbi(LCD_Port, PIN_CLK);
		_delay_us(_WAIT);
		datt = datt << 1;
		/*LCD_PORT &= ~LCD_SCLK;
		 ( datt & 0x80 ) ? (LCD_PORT |= LCD_MOSI) : (LCD_PORT&= ~LCD_MOSI);
		 LCD_PORT |= LCD_SCLK;
		 datt = datt << 1;*/
	}
#else
	//SPDR = datt;
	//while(!(SPSR & (1<<SPIF)));
#endif
	sbi(LCD_Port, PIN_CS); //LCD_PORT |= LCD_CS;
}

// Инициализация LCD после подачи питания
// Можно указать разрядность для цвета
// контрастность, внутренние питания, инверсию и тд
void InitLCD(void)	//siemens m55 lph8731-3C (contr 104x82 -> m55 101x80)
{
	//LCD_DDR = ~LCD_MISO;	//MISO INput, other OUTput
	//LCD_PORT =  LCD_MOSI|LCD_SCLK|LCD_CS|LCD_MISO;	// MISO pull-up
#ifndef SOFTWARE_SPI
/*
	// for AVR
	//Init_SPI
	//SPIE - interupt en/ DORD - 1 первый младший бит/MSTR - master mode/SPR1:SPR0 0:0=SCKt/4 1:1=SCK/128
	//SPE - Enable Hardware SPI / SPSR:SPI2X - ускоряем в 2 раза
	SPCR = (0<<SPIE)|(1<<SPE)|(0<<DORD)|(1<<MSTR)|(1<<CPOL)|(1<<CPHA)|(0<<SPR1)|(0<<SPR0);
	//SPSR |= (1<<SPI2X);
	//SPDR = data;
	//while(!(SPSR & (1<<SPIF)));
	//SPCR = 0;
*/
#endif

	// время для надёжного разряда С по питанию LCD, возможно важно при ресете устройсва
	_delay_ms(1000);

	//LCD_PORT |= LCD_POWER;	//LCD POWER ON "_--"
	//_delay_ms(5);

	//LCD_PORT |= LCD_RESET;	//LCD RESET OFF "_--"
	sbi(LCD_Port, PIN_RES);
	_delay_ms(5);

   WriteToLCD(CMD,0x01);	//Software Reset
   _delay_ms(5);

   WriteToLCD(CMD,0xc6);	//INIESC - Initialize the Settings inside the IC
   _delay_ms(40);			//в примере даташита вообще без задержки!

   WriteToLCD(CMD,0xb9);	//Refresh set
   WriteToLCD(DAT,0x00);
   WriteToLCD(CMD,0xb6);	//Display control
   WriteToLCD(DAT,0x90); //80
   WriteToLCD(DAT,0x80); //04
   WriteToLCD(DAT,0x8a); //
   WriteToLCD(DAT,0x54); //
   WriteToLCD(DAT,0x45); //
   WriteToLCD(DAT,0x52); //
   WriteToLCD(DAT,0x43); //

   WriteToLCD(CMD,0xb3); //Gray scale position set 0
   WriteToLCD(DAT,0x02); //
   WriteToLCD(DAT,0x0a); //
   WriteToLCD(DAT,0x15); //
   WriteToLCD(DAT,0x1f); //
   WriteToLCD(DAT,0x28); //
   WriteToLCD(DAT,0x30); //
   WriteToLCD(DAT,0x37); //
   WriteToLCD(DAT,0x3f); //
   WriteToLCD(DAT,0x47); //
   WriteToLCD(DAT,0x4c); //
   WriteToLCD(DAT,0x54); //
   WriteToLCD(DAT,0x65); //
   WriteToLCD(DAT,0x75); //
   WriteToLCD(DAT,0x80); //
   WriteToLCD(DAT,0x85); //
   WriteToLCD(CMD,0xb5); //Gamma curve
   WriteToLCD(DAT,0x01); //
   WriteToLCD(CMD,0xb7); //Temperature gradient
   WriteToLCD(DAT,0x03); //
   WriteToLCD(DAT,0x00); //
   WriteToLCD(DAT,0x00); //
   WriteToLCD(DAT,0x00); //
   WriteToLCD(DAT,0x00); //
   WriteToLCD(DAT,0x00); //
   WriteToLCD(DAT,0x00); //
   WriteToLCD(DAT,0x00); //
   WriteToLCD(DAT,0x00); //
   WriteToLCD(DAT,0x00); //
   WriteToLCD(DAT,0x00); //
   WriteToLCD(DAT,0x00); //
   WriteToLCD(DAT,0x00); //
   WriteToLCD(DAT,0x00); //
   WriteToLCD(CMD,0xbd); //Common driver output select
   WriteToLCD(DAT,0x00); //
   WriteToLCD(CMD,0x36); //Memory access control
   WriteToLCD(DAT,0x48); //
   WriteToLCD(CMD,0x2d); //Colour set
   WriteToLCD(DAT,0x00); //
   WriteToLCD(DAT,0x03); //
   WriteToLCD(DAT,0x05); //
   WriteToLCD(DAT,0x07); //
   WriteToLCD(DAT,0x09); //
   WriteToLCD(DAT,0x0b); //
   WriteToLCD(DAT,0x0d); //
   WriteToLCD(DAT,0x0f); //
   WriteToLCD(DAT,0x00); //
   WriteToLCD(DAT,0x03); //
   WriteToLCD(DAT,0x05); //
   WriteToLCD(DAT,0x07); //
   WriteToLCD(DAT,0x09); //
   WriteToLCD(DAT,0x0b); //
   WriteToLCD(DAT,0x0d); //
   WriteToLCD(DAT,0x0f); //
   WriteToLCD(DAT,0x00); //
   WriteToLCD(DAT,0x05); //
   WriteToLCD(DAT,0x0b); //
   WriteToLCD(DAT,0x0f); //
   WriteToLCD(CMD,0xba); //Voltage control
   WriteToLCD(DAT,0x2f); //
   WriteToLCD(DAT,0x03); //
   WriteToLCD(CMD,0x25); //Write contrast
   WriteToLCD(DAT,0x64); // в моём изо есть при (0x57...0x6d). перебирать индивидуально по всей шкале!
   WriteToLCD(CMD,0xbe); //Power control
   WriteToLCD(DAT,0x59); //
   WriteToLCD(CMD,0x3a); //interfase pixel format /Установка глубины цвета 0bXXXXXABC
#ifdef COLOR_8
	WriteToLCD(DAT,0x02); // 2 = 8bit/pixel, 3 = 12 bit/pixel, остальные варианты игнор. В реж 8бит цвет конвертируется через таблицу Colour set
#else
	WriteToLCD(DAT,0x03);
#endif
   //должно быть в конце
   WriteToLCD(CMD,0x03); //Booster voltage ON
   _delay_ms(40);
   WriteToLCD(CMD,0x11); //sleep out
   _delay_ms(40);


	ColorBack = BLACK;
	ColorInk = WHITE;
	clear_lcd(WHITE); //заполним буфер что бы не было мусора на экране

   WriteToLCD(CMD,0x29); //Display ON
}

void contrast_lcd_set(unsigned char contrast)
{
	WriteToLCD(CMD,0x25); //Write contrast
	WriteToLCD(DAT,contrast); // в моём изо есть при (0x57...0x6d). перебирать индивидуально по всей шкале!
}

void voltcont_lcd(unsigned char vol)
{
   WriteToLCD(CMD,0xba); //Voltage control
   WriteToLCD(DAT,vol & 0x7f); //def 0x2f
   WriteToLCD(DAT,0x03); //
}
//нарисовать пиксель в позиции XPos, YPos. Цвет в глобальной переменной ColorInk
void DrawPixel(uint8_t XPos, uint8_t YPos)
{
	//104x82 -> m55 101 x 80
	WriteToLCD( CMD, 0x2A );	//x-координата
	WriteToLCD( DAT, XPos );
	WriteToLCD( DAT, 100 );	//=0x64  len=101 /max=103

	WriteToLCD( CMD, 0x2B );	//Y-координата
	WriteToLCD( DAT, YPos+1 );
	WriteToLCD( DAT, 80 );	//=0x50  len=80 /max=83

	WriteToLCD( CMD, 0x2C );	// WR_MEM
#ifdef COLOR_8
	WriteToLCD(DAT, (uint8_t)ColorInk);
#else
	WriteToLCD(DAT, ColorInk >> 4);   // RRRRGGGGBBBBxxxx / red=0x0f00 -> f0.00
	WriteToLCD(DAT, (uint8_t)ColorInk << 4);
	WriteToLCD( CMD, 0x2C );	// сбрасываем позицию на начало
#endif
}
//нарисовать пиксель в позиции XPos, YPos цвета Color.
void DrawPixel_col(uint8_t XPos, uint8_t YPos, uint16_t Color)
{
	//104x82 -> m55 101 x 80
	WriteToLCD( CMD, 0x2A );	//x-координата
	WriteToLCD( DAT, XPos );
	WriteToLCD( DAT, 100 );	//=0x64  len=101 /max=103

	WriteToLCD( CMD, 0x2B );	//Y-координата
	WriteToLCD( DAT, YPos+1 );
	WriteToLCD( DAT, 80 );	//=0x50  len=80 /max=83

	WriteToLCD( CMD, 0x2C );	// WR_MEM
	#ifdef COLOR_8
	WriteToLCD(DAT, (uint8_t)Color);
	#else
	WriteToLCD(DAT, ColorInk >> 4);   // RRRRGGGGBBBBxxxx / red=0x0f00 -> f0.00
	WriteToLCD(DAT, (uint8_t)Color << 4);
	WriteToLCD( CMD, 0x2C );	// сбрасываем позицию на начало
	#endif
}

void DrawLineX(uint8_t y)
{
	uint8_t i = 100;
	do
	{
		DrawPixel(i, y);
	} while (i--);
}


void clear_lcd( uint16_t Color )
{
	window_lcd(0, 0, 100, 79);
	WriteToLCD( CMD, 0x2C );	// WR_MEM

#ifdef COLOR_8
	for(uint16_t i = 0; i < 8080; i++)	// 101x80
	{
		WriteToLCD(DAT, (uint8_t)Color);
	}
#else
	uint8_t c;
	for(uint16_t i = 0; i < 4040; i++)	// 101x80/2
	{
		//RRRRGGGG BBBBrrrr ggggbbbb  - передача 3я байтами 2пикселя! в 4096цветрежиме
		WriteToLCD(DAT, Color >> 4);
		c = (uint8_t)Color << 4;
		WriteToLCD(DAT, c|(uint8_t)(Color >> 8));
		WriteToLCD(DAT, (uint8_t)Color);
	}
#endif
}


void window_lcd(unsigned char xstart, unsigned char ystart, unsigned char xend, unsigned char yend)
{
	WriteToLCD( CMD, 0x2A );	//x-координата
	WriteToLCD( DAT, xstart );
	WriteToLCD( DAT, (xend>100)?100:xend );		//max=103

	WriteToLCD( CMD, 0x2B );	//Y-координата
	WriteToLCD( DAT, ystart+1 );
	WriteToLCD( DAT, (yend+1>80)?80:yend+1);		//max=83
	WriteToLCD( CMD, 0x2C );	// WR_MEM
}

void cursor_lcd(unsigned char row, unsigned char col)
{
	window_lcd(row,col, 100, 79);
}

void line_lcd(unsigned char x1,unsigned char y1, unsigned char x2, unsigned char y2)
{
	int dx, dy, stepx, stepy, fraction;
	dy = y2 - y1;
	dx = x2 - x1;

	if ( dy < 0 )
	{
		dy    = -dy;
		stepy = -1;
	}
	else
	{
		stepy = 1;
	}

	if ( dx < 0 )
	{
		dx    = -dx;
		stepx = -1;
	}
	else
	{
		stepx = 1;
	}

	dx <<= 1;
	dy <<= 1;

	DrawPixel( x1, y1);

	if ( dx > dy )
	{
		fraction = dy - ( dx >> 1);
		while ( x1 != x2 )
		{
			if ( fraction >= 0 )
			{
				y1 += stepy;
				fraction -= dx;
			}
			x1 += stepx;
			fraction += dy;

			DrawPixel( x1, y1);
		}
	}
	else
	{
		fraction = dx - ( dy >> 1);
		while ( y1 != y2 )
		{
			if ( fraction >= 0 )
			{
				x1 += stepx;
				fraction -= dy;
			}
			y1 += stepy;
			fraction += dx;

			DrawPixel( x1, y1);
		}
	}
}

//прямоугольник. если size=0 закрасим
void Rectangle_lcd( unsigned char x,unsigned char y, unsigned char x2, unsigned char y2, unsigned char size )
{
	if (size==0)
	{
		for (; y<y2; y++)
		{
			line_lcd(x, y, x2, y);
		}
	}
	else
	{
		for(unsigned int i=1; i<=size; i++)
		{
			line_lcd(x, y, x, y2);
			line_lcd(x2, y, x2, y2);
			line_lcd(x, y, x2, y);
			line_lcd(x, y2, x2, y2);
			x++;
			y++;
			x2--;
			y2--;
		}
	}
}

/****************************************************************************/
/*  Функция вывода круга                                                    */
/*  Function : LcdGraphCircle                                               */
/*      Parameters                                                          */
/*          Input   :  координаты центра, радиус круга                      */
/*          Output  :  Nothing                                              */
/****************************************************************************/
void Circle_lcd(unsigned char xc,unsigned char yc,unsigned char r)
{
	int  x,y,d;
	y = r;
	d = 3-2*r;
	x = 0;

	while(x <= y)
	{
		DrawPixel(x+xc,y+yc);
		DrawPixel(x+xc,-y+yc);
		DrawPixel(-x+xc,-y+yc);
		DrawPixel(-x+xc,y+yc);
		DrawPixel(y+xc,x+yc);
		DrawPixel(y+xc,-x+yc);
		DrawPixel(-y+xc,-x+yc);
		DrawPixel(-y+xc,x+yc);
		if (d<0) {
			d = d+4*x+6;
		} else {
			d = d+4*(x-y)+10;
			y--;
		}
		x++;
	};
}

//символы вверх ногами! вывод вертикальными байтами
const unsigned char font5x7 [][5] = {

{ 0x00, 0x00, 0x00, 0x00, 0x00 },   // sp
{ 0x00, 0x00, 0x2f, 0x00, 0x00 },   // !
{ 0x00, 0x07, 0x00, 0x07, 0x00 },   // "
{ 0x14, 0x7f, 0x14, 0x7f, 0x14 },   // #
{ 0x24, 0x2a, 0x7f, 0x2a, 0x12 },   // $
{ 0x32, 0x34, 0x08, 0x16, 0x26 },   // %
{ 0x36, 0x49, 0x55, 0x22, 0x50 },   // &
{ 0x00, 0x05, 0x03, 0x00, 0x00 },   // '
{ 0x00, 0x1c, 0x22, 0x41, 0x00 },   // (
{ 0x00, 0x41, 0x22, 0x1c, 0x00 },   // )
{ 0x14, 0x08, 0x3E, 0x08, 0x14 },   // *
{ 0x08, 0x08, 0x3E, 0x08, 0x08 },   // +
{ 0x00, 0x00, 0x50, 0x30, 0x00 },   // ,
{ 0x10, 0x10, 0x10, 0x10, 0x10 },   // -
{ 0x00, 0x60, 0x60, 0x00, 0x00 },   // .
{ 0x20, 0x10, 0x08, 0x04, 0x02 },   // /
{ 0x3E, 0x51, 0x49, 0x45, 0x3E },   // 0
{ 0x00, 0x42, 0x7F, 0x40, 0x00 },   // 1
{ 0x42, 0x61, 0x51, 0x49, 0x46 },   // 2
{ 0x21, 0x41, 0x45, 0x4B, 0x31 },   // 3
{ 0x18, 0x14, 0x12, 0x7F, 0x10 },   // 4
{ 0x27, 0x45, 0x45, 0x45, 0x39 },   // 5
{ 0x3C, 0x4A, 0x49, 0x49, 0x30 },   // 6
{ 0x01, 0x71, 0x09, 0x05, 0x03 },   // 7
{ 0x36, 0x49, 0x49, 0x49, 0x36 },   // 8
{ 0x06, 0x49, 0x49, 0x29, 0x1E },   // 9
{ 0x00, 0x36, 0x36, 0x00, 0x00 },   // :
{ 0x00, 0x56, 0x36, 0x00, 0x00 },   // ;
{ 0x08, 0x14, 0x22, 0x41, 0x00 },   // <
{ 0x14, 0x14, 0x14, 0x14, 0x14 },   // =
{ 0x00, 0x41, 0x22, 0x14, 0x08 },   // >
{ 0x02, 0x01, 0x51, 0x09, 0x06 },   // ?
{ 0x32, 0x49, 0x59, 0x51, 0x3E },   // @
{ 0x7E, 0x11, 0x11, 0x11, 0x7E },   // A
{ 0x7F, 0x49, 0x49, 0x49, 0x36 },   // B
{ 0x3E, 0x41, 0x41, 0x41, 0x22 },   // C
{ 0x7F, 0x41, 0x41, 0x22, 0x1C },   // D
{ 0x7F, 0x49, 0x49, 0x49, 0x41 },   // E
{ 0x7F, 0x09, 0x09, 0x09, 0x01 },   // F
{ 0x3E, 0x41, 0x49, 0x49, 0x7A },   // G
{ 0x7F, 0x08, 0x08, 0x08, 0x7F },   // H
{ 0x00, 0x41, 0x7F, 0x41, 0x00 },   // I
{ 0x20, 0x40, 0x41, 0x3F, 0x01 },   // J
{ 0x7F, 0x08, 0x14, 0x22, 0x41 },   // K
{ 0x7F, 0x40, 0x40, 0x40, 0x40 },   // L
{ 0x7F, 0x02, 0x0C, 0x02, 0x7F },   // M
{ 0x7F, 0x04, 0x08, 0x10, 0x7F },   // N
{ 0x3E, 0x41, 0x41, 0x41, 0x3E },   // O
{ 0x7F, 0x09, 0x09, 0x09, 0x06 },   // P
{ 0x3E, 0x41, 0x51, 0x21, 0x5E },   // Q
{ 0x7F, 0x09, 0x19, 0x29, 0x46 },   // R
{ 0x46, 0x49, 0x49, 0x49, 0x31 },   // S
{ 0x01, 0x01, 0x7F, 0x01, 0x01 },   // T
{ 0x3F, 0x40, 0x40, 0x40, 0x3F },   // U
{ 0x1F, 0x20, 0x40, 0x20, 0x1F },   // V
{ 0x3F, 0x40, 0x38, 0x40, 0x3F },   // W
{ 0x63, 0x14, 0x08, 0x14, 0x63 },   // X
{ 0x07, 0x08, 0x70, 0x08, 0x07 },   // Y
{ 0x61, 0x51, 0x49, 0x45, 0x43 },   // Z
{ 0x00, 0x7F, 0x41, 0x41, 0x00 },   // [
{ 0x55, 0x2A, 0x55, 0x2A, 0x55 },   // 55
{ 0x00, 0x41, 0x41, 0x7F, 0x00 },   // ]
{ 0x04, 0x02, 0x01, 0x02, 0x04 },   // ^
{ 0x40, 0x40, 0x40, 0x40, 0x40 },   // _
{ 0x00, 0x01, 0x02, 0x04, 0x00 },   // '
{ 0x20, 0x54, 0x54, 0x54, 0x78 },   // a
{ 0x7F, 0x48, 0x44, 0x44, 0x38 },   // b
{ 0x38, 0x44, 0x44, 0x44, 0x20 },   // c
{ 0x38, 0x44, 0x44, 0x48, 0x7F },   // d
{ 0x38, 0x54, 0x54, 0x54, 0x18 },   // e
{ 0x08, 0x7E, 0x09, 0x01, 0x02 },   // f
{ 0x0C, 0x52, 0x52, 0x52, 0x3E },   // g
{ 0x7F, 0x08, 0x04, 0x04, 0x78 },   // h
{ 0x00, 0x44, 0x7D, 0x40, 0x00 },   // i
{ 0x20, 0x40, 0x44, 0x3D, 0x00 },   // j
{ 0x7F, 0x10, 0x28, 0x44, 0x00 },   // k
{ 0x00, 0x41, 0x7F, 0x40, 0x00 },   // l
{ 0x7C, 0x04, 0x18, 0x04, 0x78 },   // m
{ 0x7C, 0x08, 0x04, 0x04, 0x78 },   // n
{ 0x38, 0x44, 0x44, 0x44, 0x38 },   // o
{ 0x7C, 0x14, 0x14, 0x14, 0x08 },   // p
{ 0x08, 0x14, 0x14, 0x18, 0x7C },   // q
{ 0x7C, 0x08, 0x04, 0x04, 0x08 },   // r
{ 0x48, 0x54, 0x54, 0x54, 0x20 },   // s
{ 0x04, 0x3F, 0x44, 0x40, 0x20 },   // t
{ 0x3C, 0x40, 0x40, 0x20, 0x7C },   // u
{ 0x1C, 0x20, 0x40, 0x20, 0x1C },   // v
{ 0x3C, 0x40, 0x30, 0x40, 0x3C },   // w
{ 0x44, 0x28, 0x10, 0x28, 0x44 },   // x
{ 0x0C, 0x50, 0x50, 0x50, 0x3C },   // y
{ 0x44, 0x64, 0x54, 0x4C, 0x44 },   // z

{ 0x08,0x36,0x41,0x00,0x00}, // Symbol 7B
{ 0x00,0x00,0x7F,0x00,0x00}, // Symbol 7C
{ 0x00,0x41,0x36,0x08,0x00}, // Symbol 7D
{ 0x01,0x01,0x02,0x02,0x01}, // Symbol 7E
{ 0x7F,0x7F,0x7F,0x7F,0x7F}  // Symbol 7F
};

//Argumenty: Znak, pozycja x, y, rozmiar 1 lub 2, kolor tekstu, koolor tіa tekstu (jeњli "-1" to pisze sam№ czcionkк bez tіa tekstu)
// шрифт 5х7 (снизу разделитель встроем в шрифт) + 1 символьный разделитель справа вывести программно
void printchr_lcd( char chr, uint8_t column, uint8_t row, size_print size, uint8_t fill )
{
	char buf;
	//uint8_t l, k;
	unsigned int xpoz, ypoz;
	xpoz = column * 6;
	ypoz = row * 8;
	if (size == BIG)
	{
		xpoz *= 2;
		ypoz *= 2;
	}
	const uint8_t* p_chr = &(font5x7[chr - 32][0]);
	if (size == SMALL)
	{
		for(uint8_t i = 0; i < 6; i++) //dla 5 elementуw tablicy znakowej
		{
			//buf=font5x7[chr - 32][i];
			//buf=pgm_read_byte(&(font5x7[chr - 32][i]));//przypisania buf jednego z 5 elementуw
			buf = (i==5) ? 0 : pgm_read_byte((const char*)p_chr++); //6-ым столбцом выводим разделительное пространство, иначе байт памяти
			for (uint8_t j=0; j < 8; j++)
			{
				if (buf & 1)
				{
					DrawPixel_col(xpoz+i,ypoz+j, ColorInk);
				}
				else if (fill)
				{
					DrawPixel_col(xpoz+i,ypoz+j, ColorBack);
				}
				buf >>= 1;
			}
		}
	}
	else if (size == BIG)
	{
		for (int i = 0; i < 6; i++ )
		{
			buf = (i==5) ? 0 : pgm_read_byte((const char*)p_chr++);//przypisania buf jednego z 5 elementуw
			for (uint8_t j=0; j < 8; j++, buf >>= 1)
			{
				if (buf & 1)
				{
					DrawPixel_col(xpoz + i*2,		ypoz + j*2,		ColorInk);
					DrawPixel_col(xpoz + i*2,		ypoz + j*2 + 1,	ColorInk);
					DrawPixel_col(xpoz + i*2 + 1,	ypoz + j*2,		ColorInk);
					DrawPixel_col(xpoz + i*2 + 1,	ypoz + j*2 + 1,	ColorInk);
				}
				else if (fill)
				{
					DrawPixel_col(xpoz + i*2,		ypoz + j*2,		ColorBack);
					DrawPixel_col(xpoz + i*2,		ypoz + j*2 + 1,	ColorBack);
					DrawPixel_col(xpoz + i*2 + 1,	ypoz + j*2,		ColorBack);
					DrawPixel_col(xpoz + i*2 + 1,	ypoz + j*2 + 1,	ColorBack);
				}
			}
		}
		//l = 0;
		//for (int i = 0; i < 6; i++ )
		//{
			//k = 0;
			//buf = (i==5)? 0: pgm_read_byte(p_chr++);//przypisania buf jednego z 5 elementуw
			//for (uint8_t j=0; j<8; j++, buf >>= 1, k+=2)
			//{
				//if(buf & 1)
				//{
					//DrawPixel_col(xpoz+l,ypoz+k, ColorInk);
					//DrawPixel_col(xpoz+l,ypoz+k+1, ColorInk);
					//DrawPixel_col(xpoz+l+1,ypoz+k, ColorInk);
					//DrawPixel_col(xpoz+l+1,ypoz+k+1, ColorInk);
				//}
				//else if(fill)
				//{
					//DrawPixel_col(xpoz+l,ypoz+k, ColorBack);
					//DrawPixel_col(xpoz+l,ypoz+k+1, ColorBack);
					//DrawPixel_col(xpoz+l+1,ypoz+k, ColorBack);
					//DrawPixel_col(xpoz+l+1,ypoz+k+1, ColorBack);
				//}
			//}
			//l += 2;
		//}
	}
}

void printstr_lcd(char *str, uint8_t column, uint8_t row, size_print size, uint8_t fill) {
	// Argumenty: tekst w "", pozycja x, y, rozmiar 1 lub 2, kolor tekstu, koolor tіa tekstu (jeњli "-1" to pisze sam№ czcionkк bez tіa tekstu)
	// здесь x y позиция не в пикселях, а столбец и строка от нуля!!!
	// while((*str != 0) && (*str != 10)) - переделал в упрощённый вар. / 10-"line feed"
	while (*str) {
		if (*str == '\n') {
			row++;
			column = 0;
			str++;
		} else if (*str >= ' ') {
			printchr_lcd(*str++, column++, row, size, fill);
			if (size == SMALL && column > 16) {
				row++;
				column = 0;
			} else if (size == BIG && column > 7) {
				row++;
				column = 0;
			}
		}
	}
}

void printstr_lcd_p(const char* str_p, uint8_t column, uint8_t row, size_print size, uint8_t fill )
{
	//Same as above, but the string is located in program memory,
	//so "lpm" instructions are needed to fetch it.

  char c;
  for (c = pgm_read_byte(str_p); c; ++str_p, c = pgm_read_byte(str_p))
    {
		printchr_lcd(c, column, row, size, fill);
		column++;
		if(size == SMALL && column>16)////Jeњli maіe litery i wyjdzie poza ekran
		{
			row++;
			column=0;
		}
		else if(size == BIG && column>7)//Jeњli duїe litery i wyjdzie poza ekran
		{
			row++;
			column=0;
		}
    }
}

void print_bin_lcd(char b, uint8_t column, uint8_t row)
{
	for (uint8_t j=0; j < 8; j++)
	{
		if (b & (1<<7))
		{
			printchr_lcd('1', column++, row, SMALL, 1);
		}
		else
		{
			printchr_lcd('0', column++, row, SMALL, 1);
		}
		b <<= 1;
		if (column > COLUMN_SMALL_MAX)
		{
			column = 0;
			row++;
			if (row > ROW_SMALL_MAX)
				row = 0;
		}
	}
}
/*
//----------------------------------------------------------------------------------------------------
//вывод символа в позицию
//----------------------------------------------------------------------------------------------------
inline void LCD_PutSymbol(unsigned char x,unsigned char y,char symbol)
{
	if (symbol<32 || symbol>127) return;
	window_lcd(x,y,x+5,y+7);
	int offset=6*(symbol-0x20);
	for(y=0;y<8;y++)
	{
		unsigned char mask=1<<y;
		for(x=0;x<6;x++)
		{
			unsigned char byte=pgm_read_byte(&(font6x8[offset+x]));
			if (byte&mask)
			{
				#ifdef COLOR_8
					WriteToLCD(DAT, (uint8_t)ColorInk);
				#else
					#error "LCD_PutSymbol ошибка в указании глубины цвета"
				#endif
			}
			else
			{
				#ifdef COLOR_8
					WriteToLCD(DAT, (uint8_t)ColorBack);
				#else
					#error "LCD_PutSymbol ошибка в указании глубины цвета"
				#endif
			}
		}
	}
}
//----------------------------------------------------------------------------------------------------
//вывод строчки в позицию
//----------------------------------------------------------------------------------------------------
inline void LCD_PutString(unsigned char x,unsigned char y, const char *string)
{
	unsigned char l=strlen(string);
	for(unsigned char n=0;n<l;n++,x+=6)
	{
		LCD_PutSymbol(x,y,string[n]);
	}
}
//Все массивы такого рода надо хранить на FLASH, а не в ОЗУ. Если вы не укажете компилятору, он всё будет хранить в ОЗУ.
//Вот вам функция печати символа для WinAVR (правда, для другого дисплея, так что могут быть перепутаны x и y):
//#include <avr/pgmspace.h>

// таблица символов
const char font6x8[576] =
{
	0x00,0x00,0x00,0x00,0x00,0x00, // Symbol 20
	0x00,0x00,0x00,0x5F,0x00,0x00, // Symbol 21
	0x00,0x00,0x03,0x00,0x03,0x00, // Symbol 22
	0x22,0x7F,0x22,0x22,0x7F,0x22, // Symbol 23
	0x00,0x24,0x2A,0x6B,0x2A,0x12, // Symbol 24
	0x00,0x23,0x13,0x08,0x64,0x62, // Symbol 25
	0x00,0x3A,0x45,0x45,0x3A,0x28, // Symbol 26
	0x00,0x00,0x00,0x02,0x01,0x00, // Symbol 27
	0x00,0x00,0x3E,0x41,0x00,0x00, // Symbol 28
	0x00,0x00,0x41,0x3E,0x00,0x00, // Symbol 29
	0x00,0x2A,0x1C,0x1C,0x2A,0x00, // Symbol 2A
	0x00,0x08,0x08,0x3E,0x08,0x08, // Symbol 2B
	0x00,0x00,0x80,0x40,0x00,0x00, // Symbol 2C
	0x00,0x08,0x08,0x08,0x08,0x00, // Symbol 2D
	0x00,0x00,0x00,0x40,0x00,0x00, // Symbol 2E
	0x00,0x20,0x10,0x08,0x04,0x02, // Symbol 2F
	0x00,0x3E,0x51,0x49,0x45,0x3E, // Symbol 30
	0x00,0x00,0x42,0x7F,0x40,0x00, // Symbol 31
	0x00,0x62,0x51,0x51,0x51,0x4E, // Symbol 32
	0x00,0x21,0x41,0x45,0x45,0x3B, // Symbol 33
	0x00,0x18,0x16,0x11,0x7F,0x10, // Symbol 34
	0x00,0x27,0x45,0x45,0x45,0x39, // Symbol 35
	0x00,0x3E,0x49,0x49,0x49,0x32, // Symbol 36
	0x00,0x01,0x61,0x11,0x09,0x07, // Symbol 37
	0x00,0x36,0x49,0x49,0x49,0x36, // Symbol 38
	0x00,0x26,0x49,0x49,0x49,0x3E, // Symbol 39
	0x00,0x00,0x00,0x12,0x00,0x00, // Symbol 3A
	0x00,0x00,0x20,0x12,0x00,0x00, // Symbol 3B
	0x00,0x08,0x14,0x22,0x41,0x00, // Symbol 3C
	0x00,0x14,0x14,0x14,0x14,0x14, // Symbol 3D
	0x00,0x41,0x22,0x14,0x08,0x00, // Symbol 3E
	0x00,0x06,0x01,0x51,0x09,0x06, // Symbol 3F
	0x00,0x3E,0x41,0x4D,0x4D,0x2E, // Symbol 40
	0x00,0x78,0x16,0x11,0x16,0x78, // Symbol 41
	0x00,0x7F,0x49,0x49,0x49,0x36, // Symbol 42
	0x00,0x3E,0x41,0x41,0x41,0x22, // Symbol 43
	0x00,0x7F,0x41,0x41,0x41,0x3E, // Symbol 44
	0x00,0x7F,0x49,0x49,0x49,0x41, // Symbol 45
	0x00,0x7F,0x09,0x09,0x09,0x01, // Symbol 46
	0x00,0x3E,0x41,0x41,0x51,0x32, // Symbol 47
	0x00,0x7F,0x08,0x08,0x08,0x7F, // Symbol 48
	0x00,0x00,0x41,0x7F,0x41,0x00, // Symbol 49
	0x00,0x30,0x40,0x41,0x41,0x3F, // Symbol 4A
	0x00,0x7F,0x08,0x08,0x14,0x63, // Symbol 4B
	0x00,0x7F,0x40,0x40,0x40,0x60, // Symbol 4C
	0x00,0x7F,0x04,0x18,0x04,0x7F, // Symbol 4D
	0x00,0x7F,0x04,0x08,0x10,0x7F, // Symbol 4E
	0x00,0x3E,0x41,0x41,0x41,0x3E, // Symbol 4F
	0x00,0x7F,0x09,0x09,0x09,0x06, // Symbol 50
	0x00,0x3E,0x41,0x61,0x21,0x5E, // Symbol 51
	0x00,0x7F,0x09,0x09,0x19,0x66, // Symbol 52
	0x00,0x26,0x49,0x49,0x49,0x32, // Symbol 53
	0x00,0x01,0x01,0x7F,0x01,0x01, // Symbol 54
	0x00,0x3F,0x40,0x40,0x40,0x3F, // Symbol 55
	0x00,0x07,0x18,0x60,0x18,0x07, // Symbol 56
	0x00,0x1F,0x60,0x18,0x60,0x1F, // Symbol 57
	0x00,0x63,0x14,0x08,0x14,0x63, // Symbol 58
	0x00,0x03,0x04,0x78,0x04,0x03, // Symbol 59
	0x00,0x61,0x51,0x49,0x45,0x43, // Symbol 5A
	0x00,0x00,0x7F,0x41,0x00,0x00, // Symbol 5B
	0x00,0x02,0x04,0x08,0x10,0x20, // Symbol 5C
	0x00,0x00,0x41,0x7F,0x00,0x00, // Symbol 5D
	0x00,0x00,0x00,0x00,0x00,0x00, // Symbol 5E
	0x40,0x40,0x40,0x40,0x40,0x40, // Symbol 5F
	0x00,0x00,0x00,0x01,0x02,0x00, // Symbol 60
	0x00,0x20,0x54,0x54,0x54,0x78, // Symbol 61
	0x00,0x7E,0x48,0x48,0x48,0x30, // Symbol 62
	0x00,0x38,0x44,0x44,0x44,0x28, // Symbol 63
	0x00,0x30,0x48,0x48,0x48,0x7E, // Symbol 64
	0x00,0x38,0x54,0x54,0x54,0x18, // Symbol 65
	0x00,0x10,0x7C,0x12,0x02,0x04, // Symbol 66
	0x00,0x0C,0x52,0x52,0x3C,0x02, // Symbol 67
	0x00,0x7E,0x08,0x08,0x08,0x70, // Symbol 68
	0x00,0x00,0x00,0x74,0x00,0x00, // Symbol 69
	0x00,0x40,0x80,0x80,0x74,0x00, // Symbol 6A
	0x00,0x7E,0x10,0x10,0x10,0x6C, // Symbol 6B
	0x00,0x00,0x02,0x7E,0x00,0x00, // Symbol 6C
	0x00,0x7C,0x04,0x78,0x04,0x78, // Symbol 6D
	0x00,0x7C,0x04,0x04,0x04,0x78, // Symbol 6E
	0x00,0x38,0x44,0x44,0x44,0x38, // Symbol 6F
	0x00,0xFC,0x24,0x24,0x24,0x18, // Symbol 70
	0x00,0x18,0x24,0x24,0x24,0xFC, // Symbol 71
	0x00,0x7C,0x08,0x04,0x04,0x08, // Symbol 72
	0x00,0x48,0x54,0x54,0x54,0x20, // Symbol 73
	0x00,0x08,0x3E,0x48,0x40,0x00, // Symbol 74
	0x00,0x3C,0x40,0x40,0x40,0x3C, // Symbol 75
	0x00,0x1C,0x20,0x40,0x20,0x1C, // Symbol 76
	0x00,0x3C,0x40,0x30,0x40,0x3C, // Symbol 77
	0x00,0x44,0x28,0x10,0x28,0x44, // Symbol 78
	0x00,0x1C,0x20,0xA0,0xA0,0x7C, // Symbol 79
	0x00,0x44,0x64,0x54,0x4C,0x44, // Symbol 7A
	0x00,0x08,0x36,0x41,0x00,0x00, // Symbol 7B
	0x00,0x00,0x00,0x7F,0x00,0x00, // Symbol 7C
	0x00,0x00,0x41,0x36,0x08,0x00, // Symbol 7D
	0x02,0x01,0x01,0x02,0x02,0x01, // Symbol 7E
	0x00,0x7F,0x7F,0x7F,0x7F,0x7F  // Symbol 7F
};
*/
