"""Класс обёртка над модулем serial"""
r''' пример работы с serial портом
import serial
try:
  ser = serial.Serial('\\\\.\\COM3', baudrate=115200, timeout=1)
  ser.open()
  ser.reset_output_buffer()
  ser.write(b"_led_clear\n")
  ser.write(f'{mess}\n'.encode())
  ser.flushOutput()
  ser.reset_input_buffer()
  if ser.in_waiting > 0:
    print(ser.read_all().decode()) # байттстроку нужжно конвертировать в строку
  ser.close()
except:
  raise ConnectionError(f'Can\'t open serial port')

help Serial
ser = serial.Serial('\\.\COM3', 9600,timeout=None, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, bytesize=serial.EIGHTBITS)
ser = serial.Serial('COM196', 115200,timeout=None, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, bytesize=serial.EIGHTBITS)
ser = serial.Serial(port="COM4", baudrate=9600, bytesize=8, timeout=2, stopbits=serial.STOPBITS_ONE)

без указания скорости создаётся на 9600, но для ардуино stm32 CDC скорость не важна.

ser.write(b"_led_clear\n")
ser.write(b"_led_show\n")
ser.write(b"_h11030303\n")
ser.write(b"_H11030303\n") //без show

readline() reads up to one line, including the \n at the end.
Be careful when using readline().
Do specify a timeout when opening the serial port otherwise
it could block forever if no newline character is received.
If the \n is missing in the return value, it returned on timeout.

COM ports higher than COM9 need a prefix of \\.\, 
  which is written as "\\\\.\\"
Please also note that using "\\\\.\\" is always possible for devices 
  (also for COM1-COM9): CreateFile("\\\\.\\COM1", ...) 
  
  pianotutor в musescore принимает порт \\.\COM3  или COM3, и не работает в формате \\\\.\\COM3
    \\.\COM3     
    \\\\.\\COM3
    \\\\.\\COM10
    'loop://'
    'socket://localhost:777'
    'rfc2217://localhost:7000'

# pip install pyserial keyboard
'''

import serial
import time
from contextlib import contextmanager
import logging

log = logging.getLogger(__name__)
log.setLevel(logging.DEBUG)


class Serial:
    _handl: serial.Serial | None = None  # дескриптор порта
    port = 'loop://'  # по умолчанию будет loopback
    # port = '\\\\.\\COM3'
    timeout = 1
    baudrate = 115200

    @classmethod
    @contextmanager
    def connect(cls, port: str = 'loop://', baudrate: int = 115200, timeout=1):
        cls.port = port
        cls.baudrate = baudrate
        cls.timeout = timeout
        cls.init_serial()
        cls.reset_input_buffer()
        try:
            yield cls._handl
        finally:
            cls.close()

    @classmethod
    def init_serial(cls):  # первичная инициализация порта
        # assert cls.ser == None
        if cls._handl == None:  # первичная инициализация порта
            try:
                if cls.port == 'loop://':
                    cls._handl = serial.serial_for_url('loop://', timeout=cls.timeout)
                else:
                    cls._handl = serial.Serial(cls.port, baudrate=cls.baudrate, timeout=cls.timeout)
                # print(f'Open serial port: {cls.ser.name}')
            except:
                raise ConnectionError(f'Can\'t open serial port: {cls.port}, class: \'{cls.__name__}\'')
        if not cls._handl.is_open:  # может быть закрытым из-за autoclose
            try:
                cls._handl.open()
            except:
                raise ConnectionError(f'Can\'t reopen serial port: {cls.port}, class: \'{cls.__name__}\'')

    @classmethod
    def send(cls, mess: bytes, autoclose=False):
        if mess == None or mess == '': return
        # тут вставить блок try:  except:
        cls.init_serial()
        cls._handl.write(mess)
        # чтобы не зависал от переполнения буферов при тесте через 'loop://'
        if cls.port == 'loop://':
            cls._handl.flushOutput()
            # cls.ser.reset_output_buffer()
            # cls.ser.reset_input_buffer()
        if autoclose:
            cls._handl.close()

    @classmethod
    def read(cls) -> bytes | None:
        try:
            cls.init_serial()
            if Serial._handl.in_waiting > 0:
                return Serial._handl.read_all()
        except:
            raise ConnectionError(f'Can\'t read serial port: {cls.port}, class: \'{cls.__name__}\'')

    @classmethod
    def read_line(cls) -> bytes | None:
        try:
            cls.init_serial()
            if Serial._handl.in_waiting > 0:
                return Serial._handl.readline()
        except:
            raise ConnectionError(f'Can\'t read serial port: {cls.port}, class: \'{cls.__name__}\'')

    @classmethod
    def reset_input_buffer(cls):
        if cls._handl != None:
            cls._handl.reset_input_buffer()

    @classmethod
    def reset(cls):  # from stm32loader.py
        cls._handl.setDTR(0)
        time.sleep(0.1)
        cls._handl.setDTR(1)
        time.sleep(0.5)

    @classmethod
    def is_open(cls) -> bool:
        return cls._handl != None and cls._handl.is_open

    @classmethod
    def close(cls):
        if cls._handl != None and cls._handl.is_open:
            try:
                cls._handl.close()
            except:
                raise ConnectionError(f'Can\'t close serial port: {cls.port}')


if __name__ == "__main__":
    pass
