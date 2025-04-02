from PySide6.QtWidgets import QWidget, QVBoxLayout, QSizePolicy
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
from matplotlib.dates import DateFormatter
import matplotlib.pyplot as plt
import datetime

MAX_POINTS = 1000 # Максимальное количество точек на графике

class PlotWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)

        # Установка политики размера для виджета
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        self.figure = Figure() # Figure(figsize=(10, 6))
        # self.figure.subplots(3, 1, gridspec_kw={'height_ratios': [1, 1, 1.2]}) # Для лучшего управления пропорциями графиков
        # self.figure.subplots_adjust(left=0.1, right=0.95, top=0.95, bottom=0.1, hspace=0.3) # уменьшить отступы

        self.canvas = FigureCanvas(self.figure)
        
        # Установка политики размера для canvas
        self.canvas.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.canvas.updateGeometry()

        self.ax1 = self.figure.add_subplot(311)  # График температуры
        self.ax2 = self.figure.add_subplot(312)  # График влажности
        self.ax3 = self.figure.add_subplot(313)  # График давления

        # другой вариант подключить canvas
        # self.figure, self.ax = plt.subplots()
        # self.layout.addWidget(self.figure.canvas)

        # Настройка интерфейса
        layout = QVBoxLayout(self)
        layout.addWidget(self.canvas)
        layout.setContentsMargins(0, 0, 0, 0)  # Убираем отступы
        self.setLayout(layout)

        # Инициализация данных для графиков
        self.time_data = []  # Временные метки
        self.temp_data = []  # Температура
        self.temp_data_ath25 = []  # Температура ath25
        self.humidity_data = []  # Влажность
        self.pressure_data = []  # Давление

        # Настройка графиков
        self.ax1.set_ylabel('T (°C)')
        self.ax2.set_ylabel('Hum (%)')
        self.ax3.set_ylabel('P (mm)')

        # self.ax1.set_title("Температура (°C)")
        # self.ax1.set_ylabel("Температура")
        # self.ax2.set_title("Влажность (%)")
        # self.ax2.set_ylabel("Влажность")
        # self.ax3.set_title("Давление (mmHg)")
        # self.ax3.set_ylabel("Давление")
        # self.ax3.set_xlabel("Время")

        # убрать горизонтальную ось
        self.ax1.set_xticks([])
        self.ax2.set_xticks([])
        self.ax3.set_xticks([])
        self.ax3.fmt_xdata = DateFormatter('%Y-%m-%d %H:%M:%S')
        self.figure.autofmt_xdate()

        self.ax1.grid(True)
        self.ax2.grid(True)
        self.ax3.grid(True)

        # Линии графиков
        self.temp_line,     = self.ax1.plot([], [], linewidth=1, linestyle='-', marker='', label="Температура (°C)", color="red") # Температура
        self.temp_line2,    = self.ax1.plot([], [], linewidth=1, linestyle='-', marker='', label="Температура ath25 (°C)", color="orange") # Температура ath25 (°C)
        self.humidity_line, = self.ax2.plot([], [], linewidth=1, linestyle='-', marker='', label="", color="green") # Влажность (%)
        self.pressure_line, = self.ax3.plot([], [], linewidth=1, linestyle='-', marker='', label="", color="blue") # Давление (mmHg)

        # Настройка легенд loc="upper right" loc='best'
        self.ax1.legend(loc="upper right")
        self.ax2.legend(loc="upper right")
        self.ax3.legend(loc="upper right")
        self.figure.tight_layout() # для оптимального размещения элементов

        # Подключение обработчика изменения размера
        self.figure.canvas.mpl_connect('resize_event', lambda e: self.figure.tight_layout())

    def update_plot(self, values):
        """
        Обновление графиков с новыми данными.
        :param values: Список из значений [температура, влажность, давление].
        """
        try:
            current_time = datetime.datetime.now().strftime("%H:%M:%S")

            # Добавляем данные
            self.time_data.append(current_time)
            self.temp_data.append(values[0])
            self.temp_data_ath25.append(values[1])
            self.humidity_data.append(values[2])
            self.pressure_data.append(values[3])

            # Обновляем данные линий и ограничиваем количество точек на графиках
            x_range = range(len(self.time_data[-MAX_POINTS:]))
            self.temp_line.set_data(x_range, self.temp_data[-MAX_POINTS:])
            self.temp_line2.set_data(x_range, self.temp_data_ath25[-MAX_POINTS:])
            self.humidity_line.set_data(x_range, self.humidity_data[-MAX_POINTS:])
            self.pressure_line.set_data(x_range, self.pressure_data[-MAX_POINTS:])

            # Настраиваем масштаб осей
            # _len = len(self.time_data) - 1
            # self.ax1.set_xlim(0, _len)
            # self.ax2.set_xlim(0, _len)
            # self.ax3.set_xlim(0, _len)
            # self.ax1.set_ylim(min(self.temp_data, default=0)     - 5, max(self.temp_data, default=0) + 5)
            # self.ax2.set_ylim(min(self.humidity_data, default=0) - 5, max(self.humidity_data, default=0) + 5)
            # self.ax3.set_ylim(min(self.pressure_data, default=0) - 5, max(self.pressure_data, default=0) + 5)

            # Автонастройка масштаба осей
            self.ax1.relim()
            self.ax1.autoscale_view(scaley=True)
            self.ax2.relim()
            self.ax2.autoscale_view(scaley=True)
            self.ax3.relim()
            self.ax3.autoscale_view(scaley=True)

            # Устанавливаем временные метки только на нижнем графике
            # self.ax3.set_xticks(x_range)
            # self.ax3.set_xticklabels(self.time_data, rotation=45, ha="right")

            # Перерисовываем графики
            self.canvas.draw()
            self.canvas.flush_events()
        except Exception as e:
            print(f"Ошибка обновления графиков: {e}")
