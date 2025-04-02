
import logging


log = logging.getLogger(__name__)

PRESSURE_CONVERSION_MMHG = 101325 / 760 # ≈ 133,322 Па. Константа для перевода Паскалей в мм рт. ст.

class SensorDataFilter:
    """
    Класс для обработки и фильтрации данных с датчика.
    Реализует IIR-фильтр для сглаживания значений давления.
    """
    def __init__(self, k_iir=0.02):
        """
        Инициализация фильтра.
        :param k_iir: Коэффициент фильтрации для IIR-фильтра.
        """
        self.k_iir = k_iir
        self.last_data_iir = 0  # Последнее отфильтрованное значение давления
        
    def apply_iir_filter(self, data):
        """
        Применяет IIR-фильтр к значению давления.
        :param pressure: Текущее значение давления (в Паскалях).
        :return: Отфильтрованное значение давления.
        """
        # IIR ФНЧ val = val*(1-K) + res*K; K = 0.02
        data_iir = data * (1 - self.k_iir) + self.last_data_iir * self.k_iir
        self.last_data_iir = data_iir
        return data_iir

    def parse_sensor_data(self, data):
        """
        Парсит строку данных с датчика и применяет фильтрацию.
        :param data: Строка данных, полученная с датчика.
        :return: Словарь с обработанными значениями.
        """
        try:
            # Проверка валидности строки
            if not data or not data.startswith("T: "):
                return None

            # Пример строки: "T: 31.20; P1: 102119.70; P2: 765.9608; H: 30.60; T2: 31.44"
            parsed_data = {}
            parameters = data.split(";")
            for param in parameters:
                if param.strip():  # Проверка, что строка не пустая
                    key, value = map(str.strip, param.split(":"))
                    parsed_data[key] = float(value)

            # Извлечение значений
            temperature = parsed_data.get('T')
            pressure = parsed_data.get('P1')
            pressure_mm = parsed_data.get('P2') # == pressure / PRESSURE_CONVERSION_MMHG
            humidity_ath25 = parsed_data.get('H')
            temperature_ath25 = parsed_data.get('T2')

            # Применение IIR-фильтра к давлению
            pressure_iir = self.apply_iir_filter(pressure)

            # Возвращаем обработанные данные
            return {
                "temperature": temperature,
                "pressure": pressure,
                "pressure_iir": pressure_iir,
                "pressure_mm": pressure / PRESSURE_CONVERSION_MMHG,
                "humidity_ath25": humidity_ath25,
                "temperature_ath25": temperature_ath25
            }
        except ValueError as e:
            # Логируем ошибку парсинга
            log.exception("Ошибка обработки данных с датчика: %s", e)
            return None
        