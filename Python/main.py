"""Read from sensor BMP280 to console and file"""

import time
import logging
from src.serial import Serial
import src.keyboard_hook as kb
# import src.crc as CRC
from src.file_io import read_dump, save_dump, write_log

# https://docs.python.org/3/library/logging.html#logrecord-attributes
logging.basicConfig(
    # level=logging.DEBUG,
    # filename='test_logging.log', filemode='w',
    # datefmt='%Y-%m-%d %H:%M:%S',
    datefmt='%H:%M:%S',
    format='%(asctime)s.%(msecs)03d %(name)-12s %(levelname)-8s: %(message)s',
)
log = logging.getLogger(__name__)
log.setLevel(logging.DEBUG)
log.info("Starting app...")


class BMP280:
    debug = False  # показать отладочную информацию
    filehandler = None
    start_time = 0
    mmHg = 101325 / 760  # 101325/760 ≈ 133,322 Па. константа мм. рт. ст в паскали
    last_pressureIIR = 0
    K_IIR = 0.02  # coeff for IIR LF  сгладим кривую фильтром

    @classmethod
    def send(cls, str: str = '') -> None:
        if cls.debug:
            log.debug('send: %s', str)
        Serial.send(f'{str}\n'.encode())

    @classmethod
    def read_line_from_serial(cls) -> str | None:
        """Читает строку данных типа bytes и возвращает декодированную.
        для readline обязательно выставлять timeout"""
        line = Serial.read_line()
        if line is not None:
            return line.decode()

    @classmethod
    def init_sensor(cls) -> None:
        # отключает вывод данных АЦП
        cls.send(f'q')
        time.sleep(0.05)

        # период опроса в мс в десятичной системе
        cls.send(f'task bmp280 t 2000 ')
        time.sleep(0.05)

        # старт/стоп
        # cls.send(f'task bmp280 p ') # pause
        # time.sleep(0.05)

        log.info('Send init parameters: 0xF4 = 0x53, 0xF5 = 0x30')
        # ещё вариант: i2c 76 w 1 00f4 0001 27
        cls.send(f'i2c 76 w 1 00f4 0001 53')
        time.sleep(0.1)
        cls.send(f'i2c 76 w 1 00f5 0001 30')
        time.sleep(0.5)

    # example str: "T: 27.28; P1: 101355.92; P2: 760.2319;"
    @classmethod
    def read_data(cls) -> None:
        try:
            line = cls.read_line_from_serial()

            # валидация строки
            if line == None or len(line) == 0 or line.strip() == '' or not line.startswith("T: "):
                return

            parsed_data = {}

            parameters = line.split(";")
            try:
                # Парсим каждый параметр
                for param in parameters:
                    if param.strip():  # Проверка, что строка не пустая
                        key, value = param.split(":")
                        parsed_data[key.strip()] = float(value.strip())
            except KeyboardInterrupt as err:
                log.error(err, line)

            # pressure: 851393, 851433, min
            # lines = line.strip().removeprefix('pressure:').split(',')
            # pressureLF = int(lines[1]) # после ФНЧ  val = val*(1-K) + res*K; // K = 0.02
            # log.info(f'pressure: {pressure: 8d}, ФНЧ:{pressureLF: 8d}, {pressure: 024b}{mm}')
            temperature = parsed_data['T']
            pressure = parsed_data['P1']
            pressure_mm = parsed_data['P2']
            humidity_ATH25 = parsed_data['H']
            temperature_ATH25 = parsed_data['T2']

            # K_IIR = 0.02
            pressureIIR = pressure * (
                    1 - cls.K_IIR) + cls.last_pressureIIR * cls.K_IIR  # IIR ФНЧ val = val*(1-K) + res*K; K = 0.02
            cls.last_pressureIIR = pressureIIR

            # sTime = time.strftime('%X', time.localtime()) # '%Y.%m.%d %X'  f'{time.monotonic():11.3f}'
            sTime = time.strftime('%Y-%m-%d %X', time.localtime())
            sTimeLog = sTime  # sTimeLog = f'{time.monotonic() - cls.start_time:11.3f}'
            log.info(
                f'{temperature: 2.2f} C, {pressure: 10.2f} Pa, {pressureIIR: 10.2f} Pa IIR, {pressure / cls.mmHg:3.6f} mmhg, {humidity_ATH25: 2.2f} %, {temperature_ATH25: 2.2f} C')

            if cls.filehandler is not None:
                cls.filehandler.write(
                    f'{sTimeLog}; {temperature:2.2f}; {pressure:10.2f}; {pressureIIR:10.2f}; {pressure / cls.mmHg:3.6f}; {humidity_ATH25: 2.2f}; {temperature_ATH25: 2.2f};\n'.replace(
                        '.', ','))
                cls.filehandler.flush()

        except ConnectionError as err:  # Can\'t open serial port
            log.exception(err)
        except KeyboardInterrupt as err:  # Exception('Exit by hook Esc')
            log.error(err)

    @classmethod
    def start_loop_read_pressure_to_file(cls, filename: str | None = None) -> None:
        Serial.reset_input_buffer()
        cls.init_sensor()
        Serial.reset_input_buffer()
        if cls.filehandler:
            cls.filehandler.close()
            cls.filehandler = None

        cls.start_time = time.monotonic()

        if filename:
            with open(filename, 'a+') as f:
                cls.filehandler = f
                # loop чтения из буфера порта
                kb.timeloop(lambda: cls.read_data(), 60 * 60 * 24 * 10, 1)
                # освобождаем хэндл
                cls.filehandler = None
        else:
            kb.timeloop(lambda: cls.read_data(), 60 * 60 * 24 * 10, 1)


def main():
    try:
        # в linux форматы обозначения порта иные \\.\COM4  или COM4
        with Serial.connect('\\\\.\\COM4', 1000000) as conn:
            log.info('Serial port %s, baudrate %s', conn.port, conn.baudrate)
            # Serial.read_line()
            # хуки для прерывания кода по Esc, или для управления параметрами через клавиши
            kb.hook_default()
            BMP280.start_loop_read_pressure_to_file('BMP280_pressure.csv')
            kb.unhook()
    except ConnectionError as err:  # Can\'t open serial port
        log.error(err)
        input('press Enter for exit...')
    except KeyboardInterrupt as err:  # Exception('Exit by hook Esc')
        log.error(err)
    except Exception as err:
        log.exception(err)


if __name__ == "__main__":
    main()
    # input('press Enter for exit...')

""" output console
03:48:08.787 __main__     INFO    :  28.45 C,  100488.23 Pa,  100488.26 Pa IIR, 753.723709 mmhg,  34.59 %,  29.93 C
03:48:10.810 __main__     INFO    :  28.45 C,  100487.46 Pa,  100487.48 Pa IIR, 753.717933 mmhg,  34.60 %,  29.93 C
03:48:12.826 __main__     INFO    :  28.45 C,  100488.46 Pa,  100488.44 Pa IIR, 753.725434 mmhg,  34.60 %,  29.93 C
03:48:14.845 __main__     INFO    :  28.45 C,  100489.46 Pa,  100489.44 Pa IIR, 753.732935 mmhg,  34.58 %,  29.93 C
03:48:16.865 __main__     INFO    :  28.45 C,  100487.80 Pa,  100487.83 Pa IIR, 753.720484 mmhg,  34.56 %,  29.93 C
03:48:18.883 __main__     INFO    :  28.45 C,  100488.46 Pa,  100488.45 Pa IIR, 753.725434 mmhg,  34.57 %,  29.93 C
03:48:20.901 __main__     INFO    :  28.45 C,  100489.46 Pa,  100489.44 Pa IIR, 753.732935 mmhg,  34.57 %,  29.92 C
"""
