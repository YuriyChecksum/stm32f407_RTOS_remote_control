"""Методы чтения и записи в файл"""
import time


def write_log(s, filename='data.log'):
    """сохраняет в файл добавлением строки и указания времени"""
    sTime = time.strftime('%Y.%m.%d %X', time.localtime())
    with open(filename, 'a+') as f:
        f.write(f'{sTime}, {s}\n')


def save_dump(dataInt, filename='data.bin'):
    with open(filename, 'wb+') as f:
        f.write(bytes(dataInt))


def read_dump(filename='data.bin'):
    with open(filename, 'rb') as f:
        return f.read()
