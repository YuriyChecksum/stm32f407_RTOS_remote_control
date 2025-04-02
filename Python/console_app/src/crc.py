"""
Реализации CRC алгоритмов/Циклический избыточный код.
Материал из Викиучебника https://ru.wikibooks.org/wiki/Реализации_алгоритмов/Циклический_избыточный_код
"""

''' ************************************************************ '''
''' CRC-8
  Name  : CRC-8
  Poly  : 0x31    x^8 + x^5 + x^4 + 1
  Init  : 0xFF
  Revert: false
  XorOut: 0x00
  Check : 0xF7 ("123456789")
  MaxLen: 15 байт(127 бит) - обнаружение
    одинарных, двойных, тройных и всех нечетных ошибок
'''


def crc8(datas):
    crc = 0xFF
    for data in datas:
        crc ^= data
        for j in range(8):
            if crc & 0x80:
                crc = (crc << 1) ^ 0x31
            else:
                crc = crc << 1
        crc = crc & 0xff  # для python нужно ограничить длину, т.к. int бесконечный
    return crc


''' ************************************************************ '''
''' CRC-16 CCITT
  Name  : CRC-16 CCITT
  Poly  : 0x1021    x^16 + x^12 + x^5 + 1
  Init  : 0xFFFF
  Revert: false
  XorOut: 0x0000
  Check : 0x29B1 ("123456789")
  MaxLen: 4095 байт (32767 бит) - обнаружение
    одинарных, двойных, тройных и всех нечетных ошибок
'''


def crc16(datas):
    crc = 0xFFFF  # (1<<16)-1
    for data in datas:
        crc ^= data << 8
        for j in range(8):
            if crc & 0x8000:  # 1<<15
                crc = (crc << 1) ^ 0x1021
            else:
                crc = crc << 1
        crc = crc & 0xffff  # для python нужно ограничить длину, т.к. int бесконечный
    return crc


''' ************************************************************ '''
""" CRC-32  Соответствует winhex crc32!
Алгоритм CRC32 на примитивном полиноме 0xEDB88320 (зеркальное отображение полинома 0x04C11DB7).
  Name  : CRC-32
  Poly  : 0x04C11DB7    x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11
                       + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1
  Init  : 0xFFFFFFFF
  Revert: true
  XorOut: 0xFFFFFFFF
  Check : 0xCBF43926 ("123456789")
  MaxLen: 268 435 455 байт (2 147 483 647 бит) - обнаружение одинарных, двойных, пакетных и всех нечетных ошибок
"""
crc_table32 = None  # таблица 256 * 32 бит для вычисления CRC32


def crc32(datas):
    # созданние таблицы для ускоренного вычисления
    _table = crc_table32
    if _table == None:
        _table = list([0] * 256)
        for i in range(256):
            crc = i
            for j in range(8):
                if crc & 1:
                    crc = (crc >> 1) ^ 0xEDB88320
                else:
                    crc = crc >> 1
            _table[i] = crc
    # ----------
    crc = 0xFFFFFFFF  # (1<<32)-1
    for data in datas:
        crc = _table[(crc ^ data) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFF


# bin((1<<8)-1).removeprefix('0b').rjust(8, '0') == f'0b{crc:32b}'
def toStr(datas):
    print(
        f'CRC32_4: 0x{crc32_4(datas):08x}, CRC32: 0x{crc32(datas):08x}, CRC16: 0x{crc16(datas):04x}, CRC8: 0x{crc8(datas):02x}')


''' ************************************************************ '''


# Упрощённая функция вычисления CRC, подходящая для avr микроконтроллеров.
# Таблица составляет не 256 значений, а всего 16
# и вычисление CRC происходит в два этапа над младшим получайтом данных и над старшим.
def crc32_4(datas):
    _table = crc_table32_4
    crc = (1 << 32) - 1  # 0xffffffff
    for data in datas:
        crc = _table[(crc ^ data) & 0x0f] ^ (crc >> 4);
        crc = _table[(crc ^ (data >> 4)) & 0x0f] ^ (crc >> 4);
        crc = crc ^ (1 << 32) - 1  # crc = ~crc;
    return crc


# CRC константы
crc_table32_4 = [
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
]

if __name__ == "__main__":
    pass
