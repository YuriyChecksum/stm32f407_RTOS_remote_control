/*
 * CRC.h
 *
 *  Created on: Oct 3, 2024
 *  Author: Maltsev Yuriy
 */
// функции вычисления CRC для разных случаев.
// писал на основе документации из вики, а так же из даташитов на разные датчики требующие подсчёта CRC
// в большинстве случаев указывается начальное инициализирующее число, полином, инверсия и разрядность.

#ifndef INC_CRC_H_
#define INC_CRC_H_

#ifndef _CRC_8_16_32_H
#define _CRC_8_16_32_H
#endif

//https://ru.wikibooks.org/wiki/Реализации_алгоритмов/Циклический_избыточный_код

// crc16 для передачи по modbus rtu,
// вставляем в конец посылки, первый LowByte, затем Hight
uint16_t crc_modbus_rtu(unsigned char *data, unsigned char length) {
	register uint16_t j;
	register uint16_t reg_crc = 0xFFFF;
	while (length--) {
		reg_crc ^= *data++;
		for (j = 0; j < 8; j++) {
			if (reg_crc & 0x01) {
				reg_crc = (reg_crc >> 1) ^ 0xA001;
			} else {
				reg_crc = reg_crc >> 1;
			}
		}
	}
	return reg_crc;
}

// Функция вычисления CRC EEPROM
// CRC константы
const unsigned long crc_table[16] = {
  0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
  0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
  0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
  0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

//unsigned long eeprom_crc() {
//	unsigned long crc = ~0L;
//	for (int index = 0; index < EEPROM.length(); ++index) {
//		crc = crc_table[(crc ^ EEPROM[index]) & 0x0f] ^ (crc >> 4);
//		crc = crc_table[(crc ^ (EEPROM[index] >> 4)) & 0x0f] ^ (crc >> 4);
//		crc = ~crc;
//	}
//	return crc;
//}

//unsigned long crc = ~0L; // crc Init  : 0xFFFFFFFF
//crc32stream(data, &crc);
void crc32stream(uint8_t data, unsigned long *_crc) {
	//unsigned long crc = ~0L;
	unsigned long crc = *_crc;
	crc = crc_table[(crc ^ (unsigned long) data) & 0x0f] ^ (crc >> 4);
	crc = crc_table[(crc ^ (unsigned long) (data >> 4)) & 0x0f] ^ (crc >> 4);
	crc = ~crc;
	*_crc = crc;
}

//void crc32stream(uint8_t data, unsigned long *crc) {
//	//unsigned long crc = ~0L;
//	*crc = crc_table[(*crc ^ (unsigned long) data) & 0x0f] ^ (*crc >> 4);
//	*crc = crc_table[(*crc ^ (unsigned long) (data >> 4)) & 0x0f] ^ (*crc >> 4);
//	*crc = ~(*crc);
//}

unsigned long crc32buf(uint8_t *pData, size_t len) {
	unsigned long crc = ~0L;
	for (int index = 0; index < len; ++index) {
		crc = crc_table[(crc ^ pData[index]) & 0x0f] ^ (crc >> 4);
		crc = crc_table[(crc ^ (pData[index] >> 4)) & 0x0f] ^ (crc >> 4);
		crc = ~crc;
	}
	return crc;
}

/*
  Name  : CRC-8 CRC8-ATM
  Poly  : x^8+𝑥x^2+𝑥x^1+𝑥x^0

https://www.trinamic.com/fileadmin/assets/Products/ICs_Documents/TMC220x_TMC2224_datasheet_Rev1.09.pdf  стр.19
An 8 bitCRC polynomial is used for checking both read and write access. It allows detection of up to eight single bit errors.
The CRC8-ATM polynomial with an initial value of zero is appliedLSB to MSB, including  the  sync-and  addressing  byte.  The  sync  nibble  is assumed  to  always  be  correct.  The TMC22xxresponds  only  to  correctly  transmitted  datagrams  containing  its  own  slave  address.  It increases its datagram counter for each correctly received write access datagram.𝐶𝑅CRC𝐶=𝑥x^8+𝑥x^2+𝑥x^1+𝑥x^0
SERIAL CALCULATION EXAMPLE
	CRC = (CRC << 1) OR (CRC.7 XOR CRC.1 XOR CRC.0 XOR [new incoming bit])
*/
void TMC220x_uart_CRC(unsigned char *data, unsigned char len) {
	int i, j;
	unsigned char *crc = data + (len - 1); // CRC located in last byte of messageUCHAR currentByte;
	unsigned char currentByte;

	*crc = 0;
	for (i = 0; i < (len - 1); i++) { // Execute for all bytes of a message
		currentByte = data[i];  // Retrieve a byte to be sent from Array
		for (j = 0; j < 8; j++) {
			if ((*crc >> 7) ^ (currentByte & 0x01)) { // update CRC based result of XOR operation
				*crc = (*crc << 1) ^ 0x07;
			} else {
				*crc = (*crc << 1);
			}
			currentByte = currentByte >> 1;
		}  // for CRC bit
	}  // for message byte
}

/*
  Name  : CRC-8
  Poly  : 0x31    x^8 + x^5 + x^4 + 1
  Init  : 0xFF
  Revert: false
  XorOut: 0x00
  Check : 0xF7 ("123456789")
  MaxLen: 15 байт(127 бит) - обнаружение
    одинарных, двойных, тройных и всех нечетных ошибок
*/
unsigned char Crc8(unsigned char *pcBlock, unsigned int len) {
    unsigned char crc = 0xFF;
    unsigned int i;

    while (len--)
    {
      crc ^= *pcBlock++;
      for (i = 0; i < 8; i++)
        crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;
    }
    return crc;
}

/* GPT-4o-mini
 * напиши функцию на C для микроконтроллера stm32,
 * которая получает на вход адрес буфера данных и
 * считает crc8 с полиномом x^8 + x^5 + x^4 + 1 и начальным значением 0xff
 */
//#include <stdint.h>
uint8_t crc8_2(const uint8_t *data, size_t length) {
    uint8_t crc = 0xFF; // Начальное значение

    for (size_t i = 0; i < length; i++) {
        crc ^= data[i]; // XOR с текущим байтом

        for (uint8_t j = 0; j < 8; j++) { // Для каждого бита
            if (crc & 0x80) { // Если старший бит равен 1
                crc = (crc << 1) ^ 0x31; // Сдвиг влево и XOR с полином
            } else {
                crc <<= 1; // Просто сдвиг влево
            }
        }
    }

    return crc; // Возвращаем результат
}

/*
  Name  : CRC-16 CCITT
  Poly  : 0x1021    x^16 + x^12 + x^5 + 1
  Init  : 0xFFFF
  Revert: false
  XorOut: 0x0000
  Check : 0x29B1 ("123456789")
  MaxLen: 4095 байт (32767 бит) - обнаружение
    одинарных, двойных, тройных и всех нечетных ошибок
*/

unsigned short Crc16(unsigned char *pcBlock, unsigned short len)
{
    unsigned short crc = 0xFFFF;
    unsigned char i;

    while (len--)
    {
        crc ^= *pcBlock++ << 8;

        for (i = 0; i < 8; i++)
            crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc;
}

/*
  Name  : CRC-32
  Poly  : 0x04C11DB7    x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11
                       + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1
  Init  : 0xFFFFFFFF
  Revert: true
  XorOut: 0xFFFFFFFF
  Check : 0xCBF43926 ("123456789")
  MaxLen: 268 435 455 байт (2 147 483 647 бит) - обнаружение
   одинарных, двойных, пакетных и всех нечетных ошибок
*/
/*
const uint_least32_t Crc32Table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

uint_least32_t Crc32(const unsigned char *buf, size_t len) {
	uint_least32_t crc = 0xFFFFFFFF;
	while (len--)
		crc = (crc >> 8) ^ Crc32Table[(crc ^ *buf++) & 0xFF];
	return crc ^ 0xFFFFFFFF;
}
*/

/*
  Name  : CRC-32
  Poly  : 0x04C11DB7    x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11
                       + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1
  Init  : 0xFFFFFFFF
  Revert: true
  XorOut: 0xFFFFFFFF
  Check : 0xCBF43926 ("123456789")
  MaxLen: 268 435 455 байт (2 147 483 647 бит) - обнаружение
   одинарных, двойных, пакетных и всех нечетных ошибок
*/
/*
// Flag: has the table been computed? Initially false.
int crc_table_computed = 0;

// Table of CRCs of all 8-bit messages.
unsigned long crc_table[256];

void make_crc_table(void)
{
	unsigned long c;
	int n, k;
	for (n = 0; n < 256; n++) {
	  c = (unsigned long) n;
	  for (k = 0; k < 8; k++) {
		if (c & 1) {
		  c = 0xedb88320L ^ (c >> 1);
		} else {
		  c = c >> 1;
		}
	  }
	  crc_table[n] = c;
	}
	crc_table_computed = 1;
}

uint_least32_t Crc32(unsigned char *buf, size_t len)
{
    uint_least32_t crc_table[256];
    uint_least32_t crc; int i, j;

	if (!crc_table_computed)
          make_crc_table();

    for (i = 0; i < 256; i++)
    {
        crc = i;
        for (j = 0; j < 8; j++)
            crc = crc & 1 ? (crc >> 1) ^ 0xEDB88320UL : crc >> 1;

        crc_table[i] = crc;
    };

    crc = 0xFFFFFFFFUL;

    while (len--)
        crc = crc_table[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);

    return crc ^ 0xFFFFFFFFUL;
}
*/

#endif /* INC_CRC_H_ */
