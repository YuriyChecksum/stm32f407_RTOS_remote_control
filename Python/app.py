import datetime
from pathlib import Path
import sys
import logging
import csv
import serial
import serial.tools.list_ports
from PySide6.QtWidgets import QFileDialog
from PySide6.QtWidgets import QApplication, QMainWindow, QMessageBox, QWidget
from PySide6.QtCore import QTimer, Signal, QThread
from PySide6.QtGui import QTextCharFormat, QColor, QTextCursor
from src.sensor_data import SensorDataFilter
from src.mainwindow import Ui_MainWindow
from src.plotwidget import PlotWidget, PlotDataPoint
from src.logger import setup_logger, format_log_record

logging.basicConfig(
    datefmt='%H:%M:%S',
    format='%(asctime)s.%(msecs)03d %(name)-12s %(levelname)-8s: %(message)s',
)
log = logging.getLogger(__name__)
log.setLevel(logging.DEBUG)
log.info("Starting app...")

# Константы
TARGET_VID_PID = "1A86:7523" # VID и PID искомого устройства
BAUDRATE = 1000000  # Скорость по умолчанию

BASE_PATH = Path(__file__).parent

class SerialReaderThread(QThread):
    """Поток для чтения данных с последовательного порта."""
    data_received = Signal(str)  # Сигнал для передачи данных в основной поток
    error_occurred = Signal(str)  # Сигнал для передачи ошибок

    def __init__(self, serial_port):
        super().__init__()
        self.serial_port = serial_port
        self.running = True  # Флаг для управления потоком

    def run(self):
        """Основной метод потока."""
        try:
            while self.running:
                if self.serial_port and self.serial_port.is_open:
                    try:
                        # Читаем строку данных
                        line = self.serial_port.readline().decode('utf-8').strip()
                        if line:
                            self.data_received.emit(line)  # Отправляем данные через сигнал
                    except serial.SerialException as e:
                        self.error_occurred.emit(f"Ошибка чтения: {e}")
                        # log.exception("Ошибка чтения данных с последовательного порта", exc_info=True)
                        break
                    except UnicodeDecodeError as e:
                        self.error_occurred.emit(f"Ошибка декодирования данных: {e}")
                        # log.exception("Ошибка декодирования данных", exc_info=True)
                    except Exception as e:
                        self.error_occurred.emit(f"Неизвестная ошибка: {e}")
                        # log.exception("Неизвестная ошибка в потоке чтения данных", exc_info=True)
                        break
        except Exception as e:
            self.error_occurred.emit(f"Ошибка в потоке: {e}")
            # log.exception("Ошибка в потоке SerialReaderThread", exc_info=True)

    def stop(self):
        """Останавливает поток."""
        self.running = False
        self.wait()  # Ждем завершения потока

class MainWindow(QMainWindow):
    log_signal = Signal(object) # Сигнал для передачи объекта лога

    def __init__(self):
        super().__init__()
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)

        # Подключаем логгер
        log_handler = setup_logger(self.log_signal, level=logging.INFO, module_name=__name__)
        log.addHandler(log_handler)

        self.log_signal.connect(self.update_text_edit) # В textEdit отправляем форматированный текст лога
        # self.log_signal.connect(lambda rec: self.ui.textEdit.append(rec.getMessage())) # В textEdit отправляем только текст лога

        # Инициализация переменных
        self.serial_port = None  # экземпляр порта
        self.serial_thread = None  # Поток для чтения данных
        self.sensor_filter = SensorDataFilter(k_iir=0.02)

        self.plot_widget = PlotWidget() # PlotWidget(self.ui.plotWidget)
        
        # Удаляем старый layout если он есть
        if self.ui.plotWidget.layout():
            old_layout = self.ui.plotWidget.layout()
            while old_layout.count():
                item = old_layout.takeAt(0)
                if item.widget():
                    item.widget().deleteLater()
            # Передаем layout другому виджету для удаления
            QWidget().setLayout(old_layout)
        
        # Заменяем plotWidget на новый PlotWidget, потому что при 
        # наследовании от plotWidget не удаётся добиться ресайза.
        layout = self.ui.horizontalLayout_2
        layout.replaceWidget(self.ui.plotWidget, self.plot_widget)
        self.ui.plotWidget.hide()  # Скрываем старый виджет
        self.plot_widget.show()    # Показываем новый

        # Настройка кнопок
        self.ui.pushButton_connect.clicked.connect(self.toggle_connection)
        self.ui.pushButton_saveCSV.clicked.connect(self.save_to_csv)
        self.ui.pushButton_send.clicked.connect(self.send_command_from_ui)

        self.ui.comboBox_baudrate.setCurrentText(str(BAUDRATE)) # Установка baudrate
        self.list_com_ports() # выведет в логи список портов
        self.auto_select_port() # Автоматический выбор порта

    def update_text_edit(self, record: logging.LogRecord):
        """Обновляет textEdit с форматированием из logger.py.

        Форматирование:
        - Время отображается серым цветом.
        - Уровень лога:
            - DEBUG: синий.
            - INFO: зеленый.
            - WARNING: оранжевый.
            - ERROR: красный.
            - CRITICAL: темно-красный.
        - Сообщение отображается черным цветом.

        Автоматически прокручивает textEdit к последнему сообщению.
        """
        cursor = self.ui.textEdit.textCursor()
        cursor.movePosition(QTextCursor.End)

        # Получаем форматированные части записи лога
        formats = format_log_record(record)

        # Вставляем форматированные части в textEdit
        for part, fmt in formats.values():
            cursor.insertText(part, fmt)

        # Прокрутка textEdit к последнему сообщению
        self.ui.textEdit.setTextCursor(cursor)

    def list_com_ports(self):
        """Выведет список портов с подробной информацией"""
        try:
            ports = serial.tools.list_ports.comports()
            for port in ports:
                log.info(f"{port.device:7}{port.description}")
                # log.info(str(port.__dict__))
        except Exception as e:
            log.error("Ошибка при получении списка COM портов", exc_info=True)
            QMessageBox.critical(self, "Ошибка", f"Не удалось получить список портов: {e}")

    def auto_select_port(self):
        """Автоматический выбор порта с заданным VID и PID, но отображение всех портов."""
        try:
            ports = serial.tools.list_ports.comports()
            target_port = None  # Переменная для хранения порта с искомым VID и PID

            # Очистка списка портов в comboBox_ports
            self.ui.comboBox_ports.clear()

            ports_names = sorted([p.device for p in ports])  # p.name или p.device
            self.ui.comboBox_ports.addItems(ports_names)

            # ищем порт с нужным VID и PID
            for port in ports:
                if TARGET_VID_PID in port.hwid.upper():
                    target_port = port.device  # Сохраняем порт

            # Если найден порт с искомым VID и PID, выбираем его
            if target_port:
                self.ui.comboBox_ports.setCurrentText(target_port)
                log.info("Устройство найдено %s", target_port)
                if not self.serial_port:
                    self.connect_device()  # Автоподключение к устройству
            else:
                log.info("Устройство с заданным VID_PID не найдено.")
        except Exception as e:
            log.error("Ошибка при автоматическом выборе порта", exc_info=True)
            QMessageBox.critical(self, "Ошибка", f"Ошибка при автоматическом выборе порта: {e}")

    def save_to_csv(self):
        """Сохранение данных в CSV с разделителем ';'."""
        default_file_name = str(BASE_PATH / "data.csv")
        file_path, _ = QFileDialog.getSaveFileName(self, "Сохранить файл", default_file_name, "CSV Files (*.csv)")
        if not file_path:
            log.info("Сохранение отменено пользователем.")
            return

        try:
            with open(file_path, mode="w", newline="", encoding="utf-8") as file:
                writer = csv.writer(file, delimiter=';')
                # Заголовки CSV
                writer.writerow(["Время", "Температура (°C)", "Температура ath25 (°C)", "Влажность (%)", "Давление (mmHg)"])
                # Запись данных из PlotWidget
                for data_point in self.plot_widget.data_points:
                    writer.writerow([
                        data_point.time,
                        data_point.temperature,
                        data_point.temperature_ath25,
                        data_point.humidity,
                        f"{data_point.pressure:.3f}",
                    ])
            log.info("Данные успешно сохранены в %s", file_path)
        except PermissionError as e:
            log.error("Ошибка доступа к файлу при сохранении", exc_info=True)
            QMessageBox.critical(self, "Ошибка", f"Нет доступа к файлу: {e}")
        except Exception as e:
            log.exception("Ошибка сохранения файла", exc_info=True)
            QMessageBox.critical(self, "Ошибка", f"Ошибка сохранения файла: {e}")

    def toggle_connection(self):
        """Подключение/отключение устройства."""
        if self.serial_port and self.serial_port.is_open:
            self.disconnect_device()
        else:
            self.connect_device()

    def connect_device(self):
        """Подключение к выбранному порту."""
        port_name = self.ui.comboBox_ports.currentText()
        baudrate = int(self.ui.comboBox_baudrate.currentText())
        if not port_name:
            log.warning("Порт не выбран.")
            QMessageBox.warning(self, "Warning", "Please select a COM port.")
            return

        try:
            self.serial_port = serial.Serial(port_name, baudrate=baudrate, timeout=1)
            self.serial_port.reset_input_buffer()
            # self.init_sensor()
            # self.timer.start(100)  # Чтение данных каждые 100 мс

            # Запускаем поток для чтения данных
            self.serial_thread = SerialReaderThread(self.serial_port)
            self.serial_thread.data_received.connect(self.process_serial_data)
            self.serial_thread.error_occurred.connect(self.handle_serial_error)
            self.serial_thread.start()

            self.ui.pushButton_connect.setText("Отключить")
            log.info("Подключено к %s со скоростью %s бод", self.serial_port.port, self.serial_port.baudrate)
        except serial.SerialException as e:
            log.error("Ошибка подключения к последовательному порту", exc_info=True)
            QMessageBox.critical(self, "Ошибка", f"Не удалось подключиться к {port_name}: {e}")
        except ValueError as e:
            log.error("Некорректное значение скорости передачи данных", exc_info=True)
            QMessageBox.critical(self, "Ошибка", f"Некорректное значение скорости: {e}")
        except Exception as e:
            log.exception("Неизвестная ошибка при подключении устройства", exc_info=True)
            QMessageBox.critical(self, "Ошибка", f"Неизвестная ошибка: {e}")

    def disconnect_device(self):
        """Отключение от устройства."""
        if self.serial_thread:
            self.serial_thread.stop()
            self.serial_thread = None

        if self.serial_port:
            # self.timer.stop()
            self.serial_port.close()
            self.serial_port = None

        self.ui.pushButton_connect.setText("Подключить")
        log.info("Устройство отключено.")

    def init_sensor(self) -> None:
        """Настраиваем датчик bmp280 через его внутренние регистры в соответствии с даташитом."""
        # отключает вывод данных АЦП
        self.send_serial_data('q')
        time.sleep(0.05)

        # период опроса в мс в десятичной системе
        # self.send(f'task bmp280 t 2000 ')
        # time.sleep(0.05)

        # старт/стоп
        # self.send(f'task bmp280 p ')
        # time.sleep(0.05)

        # Send to bmp280: 0xF4 <- 0x53, 0xF5 <- 0x30
        log.info('Send init parameters to registers in bmp280')
        self.send_serial_data('i2c 76 w 1 00f4 0001 53')
        time.sleep(0.1)
        self.send_serial_data('i2c 76 w 1 00f5 0001 30')
        time.sleep(0.5)
        # ещё вариант настроек для bmp280
        # self.send_serial_data('i2c 76 w 1 00f4 0001 27')

    def read_serial_data(self):
        """Чтение данных с устройства по таймеру"""
        if not self.serial_port or not self.serial_port.is_open:
            return

        try:
            # TODO блокирует основной поток, надо выносить в отдельный.
            # Читает строку данных типа bytes и декодирует.
            line = self.serial_port.readline().decode('utf-8').strip()
            if line:
                self.process_serial_data(line)
        except serial.SerialException as err:  # Can\'t open serial port
            log.exception(err)
        except Exception as e:
            log.exception("Ошибка чтения данных")
    
    def process_serial_data(self, line):
        """Обработка данных, полученных из потока."""
        log.debug("Получено: %s", line)

        if not line.startswith("T: "):
            return

        data = self.sensor_filter.parse_sensor_data(line)
        if not data:
            log.info("Некорректные данные для обновления графика.")
            return

        # Обновляем график
        self.plot_widget.update_plot(PlotDataPoint(
            time=datetime.datetime.now().strftime("%H:%M:%S"),
            temperature=data['temperature'],
            temperature_ath25=data['temperature_ath25'],
            humidity=data['humidity_ath25'],
            pressure=data['pressure_mm']
        ))

        # Логируем обработанные данные
        log.info(
            f"{data['temperature']: 2.2f} C, "
            f"{data['pressure']: 10.2f} Pa, "
            f"{data['pressure_iir']: 10.2f} Pa IIR, "
            f"{data['pressure_mm']: 3.6f} mmHg, "
            f"{data['humidity_ath25']: 2.2f} %, "
            f"{data['temperature_ath25']: 2.2f} C"
        )

    def handle_serial_error(self, error_message):
        """Обработка ошибок из потока."""
        log.error(error_message)
        QMessageBox.critical(self, "Ошибка", error_message)
        self.disconnect_device()

    def send_command_from_ui(self) -> None:
        command = self.ui.lineEdit.text()
        if command:
            self.send_serial_data(command)
    
    def send_serial_data(self, command: str = '') -> None:
        """Отправляет строковую команду на подключенное устройство."""
        if not self.serial_port or not self.serial_port.is_open:
            log.warning('Попытка отправить данные без подключения к устройству.')
            QMessageBox.warning(self, "Ошибка", "Устройство не подключено.")
            return
        log.debug('Отправка команды: %s', command)
        try:
            self.serial_port.write(f'{command}\n'.encode())
        except serial.SerialException as e:
            log.error("Ошибка отправки данных на устройство", exc_info=True)
            QMessageBox.critical(self, "Ошибка", f"Ошибка отправки данных: {e}")
        except Exception as e:
            log.exception("Неизвестная ошибка при отправке данных", exc_info=True)
            QMessageBox.critical(self, "Ошибка", f"Неизвестная ошибка: {e}")

    def log_message(self, message):
        """Вывод сообщения в textEdit."""
        self.ui.textEdit.append(message)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())