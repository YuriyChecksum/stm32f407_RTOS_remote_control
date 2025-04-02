"""Строит графики по данным из 'BMP280_pressure.csv'"""

import logging
from pathlib import Path
import time
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.dates import DayLocator, HourLocator, MinuteLocator, DateFormatter, drange
import datetime as DT  # DT.datetime.now()
from collections import namedtuple
from abc import ABC, abstractmethod

logging.basicConfig(
    level=logging.WARNING,
    datefmt='%H:%M:%S',  # '%Y-%m-%d %H:%M:%S'
    format='%(asctime)s.%(msecs)03d %(name)-12s %(levelname)-8s: %(message)s',
)
log = logging.getLogger(__name__)
log.setLevel(logging.DEBUG)
log.info("Starting app...")


class AbcDataLoader(ABC):
    "Получение данных из источника"

    @abstractmethod
    def load_data(self):
        pass

    @abstractmethod
    def get_data(self):
        pass

    @abstractmethod
    def get_updated_data(self):
        pass

    @abstractmethod
    def get_last(self):
        pass


class DataCSV(AbcDataLoader):
    SensorData = namedtuple('SensorData', ["datetimenow", "T", "P", "Piir", "Pmm", "Hum", "T_ath25"])

    def __init__(self, filename: str, n=0):
        self.filename = filename
        self.n = n
        self.data: pd.DataFrame | None = None
        self.load_data()  # загрузить данные из файла

    def load_data(self) -> None:
        self.data = pd.DataFrame(self.load_data_from_csv(self.filename, self.n))

    def get_data(self) -> pd.DataFrame | None:
        return self.data

    def get_updated_data(self) -> pd.DataFrame | None:
        self.load_data()
        return self.data

    def get_last(self):
        return self.data.iloc[-1]

    def get_last_T(self):
        return self.data[-1:]['T'].values[0]

    def load_data_from_csv(self, filename: str, n=0) -> list[SensorData] | None:
        """Загружает историю из файла в DataFrame"""
        # формат строки в файле: f'{temperature: 2.2f} C, {pressure: 10.2f} Pa, {pressureIIR: 10.2f} Pa IIR, \
        # {pressure/cls.mmHg:3.6f} mmhg, {humidity_ATH25: 2.2f} %, {temperature_ATH25: 2.2f} C')

        try:
            df = pd.read_csv(filename, names=None, sep=";")
            df.columns = ['datetimenow', 'T', 'P', 'Piir', 'Pmm', "Hum", "T_ath25", 'empty']

            if n == 0:
                n = len(df)

            # Преобразование типов данных
            # df['datetimenow'] = pd.to_datetime(df['datetimenow'])
            # df['T'] = df['T'].str.replace(',', '.').astype(float)

            data = []
            for _d in df[-n:].itertuples(index=False):
                # добавляем именованный кортеж
                data.append(self.SensorData(
                    # time.strptime(_d.time.strip(), '%H:%M:%S'), # '%H:%M:%S'
                    # _d.Index,
                    _d.datetimenow.strip(),
                    # float(_d.V.replace(',', '.')),
                    float(_d.T.replace(',', '.')),
                    float(_d.P.replace(',', '.')),
                    float(_d.Piir.replace(',', '.')),
                    float(_d.Pmm.replace(',', '.')),
                    float(_d.Hum.replace(',', '.')),
                    float(_d.T_ath25.replace(',', '.')),
                ))
            return data  # pd.DataFrame(data)
        except FileNotFoundError as err:
            log.error(err)
        except KeyboardInterrupt as err:
            log.info(err)


class DrawChart:
    def __init__(self, data_source: AbcDataLoader):
        self.data_source = data_source
        self.isBreak = False  # флаг для прерывания цикла
        self._sleep = 1
        self.init_chart()

    @abstractmethod
    def init_chart(self):
        raise NotImplementedError

    @abstractmethod
    def update(self):
        raise NotImplementedError

    def update_full_redraw(self):
        """Обновление данных на графике через перерисовку всего графика, но будет мерцание"""
        # Код только для примера!
        raise NotImplementedError
        _data = self.data_source.get_updated_data()
        plt.clf()  # Clear
        # plt.fill_between(xx, 0, yy, color='lightgrey')
        plt.plot(_data['T'])
        self.ax1 = plt.pyplot.axes()
        self._text(self.format_time(DT.datetime.now()))
        plt.draw()
        plt.gcf().canvas.flush_events()

    def loop(self, _sleep: float = 1) -> None:
        self._sleep = _sleep
        try:
            while True:
                self.update()
                self.sleep(_sleep)
        except KeyboardInterrupt as err:  # Exit by Esc or ctrl+C
            log.info(err)
        finally:
            plt.close('all')  # закрыть все активные окна
        plt.ioff()  # Отключить интерактивный режим по завершению анимации
        plt.show()  # Нужно, чтобы график не закрывался после завершения анимации

    def sleep(self, t: float = 1):
        "позволяет быстро прерывать процесс по ctrl+C, не блокируя поток на полное время паузы"
        t1 = time.time()
        t2 = 0.1
        while time.time() - t1 < t:
            plt.pause(t2)  # не блокирующая график пауза в отличии от time.sleep()
            if self.isBreak:
                log.info("Break in sleep() function")
                break

    def set_window_title(self, title):
        self.fig.canvas.manager.set_window_title(title)

    def set_hooks(self):
        """Установка хуков на события графика"""

        def on_close(event):
            """callback на закрытие графика"""
            log.info('Closed Figure!')
            self.isBreak = True
            # raise KeyboardInterrupt("Exit on close figure")
            exit(0)

        def on_press(event):
            """callback на график"""
            log.info('you pressed', event.button, event.xdata, event.ydata)

        # про закрытие окон в matplotlib: 
        # https://ru.stackoverflow.com/questions/1302494/Закрыть-интерактивное-окно-matplotlib-в-jupyter-по-кнопке-прерывания

        # self.cid = self.fig.canvas.mpl_connect('button_press_event', self.on_press)
        # self.fig.canvas.mpl_connect('close_event', on_close)

        # Повесим хук на закрытие окна графика
        plt.get_current_fig_manager().canvas.mpl_connect('close_event', on_close)


class PressureChart(DrawChart):
    def __init__(self, data_source: AbcDataLoader):
        super().__init__(data_source)

    def init_chart(self):
        # Создание окна и осей для графика
        # self.fig, self.axs = plt.subplots(nrows = 3)  # попроще конструктор
        # sharex=True - скрывает подписи на Х
        self.fig, self.axs = plt.subplots(3, 1, figsize=(10, 5), sharex=True)
        self.ax1 = self.axs[0]
        self.ax2 = self.axs[1]
        self.ax3 = self.axs[2]

        # fig.set_figheight(2)

        # Горизонтальные линии:
        # ax.vlines(2, y.min(), y.max(), color = 'r')
        # ax.hlines(5, -10, 10, color = 'b', linewidth = 3, linestyle = '--')

        # plt.axhline(y=1.2, color = 'b', linewidth = 1, linestyle='--', label='alert levels')
        # ax.hlines(y=1.2, xmin=0, xmax=n,      color = 'b', linewidth = 1, linestyle='--', label='б\\у)

        # Установка отображаемых интервалов по осям
        # ax.set_xlim(0, 4)
        # ax.set_ylim(0, 1500)

        # plt.annotate('General direction', xy = (3.4, 17)) #add annotation
        # ax.grid()  # сетка
        # Отобразить график функции в начальный момент времени

        # df_time = pd.DataFrame(data)['datetime'].apply(lambda x: DT.datetime.strptime(x, '%Y.%m.%d %X').astimezone()) # from timestamp

        self.line1, = self.ax1.plot([], linewidth=1, linestyle='-', label='T')
        self.line2, = self.ax1.plot([], color='orange', linewidth=1, linestyle='-', label='T_ath25')
        self.line3, = self.ax2.plot([], color='r', linewidth=1, linestyle='-', marker='', label='')
        self.line4, = self.ax3.plot([], color='g', linewidth=1, linestyle='-', marker='', label='')

        self._text = self.ax1.text(0.001, 1.1, '', transform=self.ax1.transAxes).set_text

        self.ax1.set_title('Temperature (T)')
        self.ax1.set_ylabel('T (°C)')

        # self.ax2.set_title('humidity, %')
        self.ax2.set_ylabel('Hum (%)')

        # self.ax3.set_title('Pressure, mmHg')
        self.ax3.set_ylabel('P (mm)')

        # сетка
        self.ax1.grid()
        self.ax2.grid()
        self.ax3.grid()

        # убрать горизонтальную ось
        self.ax1.set_xticks([])
        self.ax2.set_xticks([])

        self.ax3.fmt_xdata = DateFormatter('%Y-%m-%d %H:%M:%S')
        self.fig.autofmt_xdate()

        self.format_time = '{:%H:%M:%S}'.format  # {:%Y-%m-%d %H:%M:%S}
        # plt.legend(loc='best')

        self.set_hooks()

        plt.tight_layout()  # для оптимального размещения элементов

    def update(self):
        """Обновление данных на графике"""
        time_start = time.perf_counter()
        _data = self.data_source.get_updated_data()
        load_file_time = time.perf_counter() - time_start
        last = self.data_source.get_last()

        self._text(
            self.format_time(DT.datetime.now())
            # f', load {load_file_time:1.3f} с, count {len(_data)}, pause {self._sleep}c\n'
            # f'{last["T"]:5.2f} C, {last["T_ath25"]:5.2f} C, '
            # f'{last["P"]:9.2f} Pa, {last["Pmm"]} mmHg, {last["Hum"]}%'
        )
        # line.set_ydata(_data['T']) # ошибка, потому что оси не настроенны

        x = range(len(_data)) # линейный ряд для оси Х

        # ["datetimenow", "T", "P", "Piir", "Pmm", "Hum", "T_ath25"]
        self.line1.set_data(x, _data['T'])
        self.line2.set_data(x, _data['T_ath25'])
        self.line3.set_data(x, _data['Hum'])
        self.line4.set_data(x, _data['Pmm'])

        self.set_window_title(
            f'{last["T"]:5.2f} {last["Pmm"]:5.1f} [{self.format_time(DT.datetime.now())}]')

        # Установка диапазонов по осям (подбирать по данным)
        # self.ax.set_ylim(min(val) * 0.9, int(max(val) * 1.1))
        # self.ax.set_xlim(0, 4)

        # update axes limits
        self.ax1.relim()
        self.ax1.autoscale_view(scaley=True)
        self.ax2.relim()
        self.ax2.autoscale_view(scaley=True)
        self.ax3.relim()
        self.ax3.autoscale_view(scaley=True)

        self.fig.canvas.draw()
        self.fig.canvas.flush_events()


BASE_PATH = Path(__file__).parent


def main():
    N_last = 5000  # n последних элементов для отображения. = 0 - использовать все
    period = 2  # период опроса файла в секундах, либо вешать хук в файловой системе на изменение файла

    filename = BASE_PATH.joinpath('BMP280_pressure.csv')
    filename_demo = BASE_PATH.joinpath('BMP280_pressure_demo.csv')

    if not filename.exists():
        log.warning(f"{filename} not found. Using '{filename_demo}'")
        if not filename_demo.exists():
            log.error(f"{filename_demo} not found")
            exit(0)
        filename = filename_demo

    data_source = DataCSV(filename, N_last)

    if (data_source.get_data() is None):
        log.error("No data")
        exit(0)

    chart = PressureChart(data_source)
    chart.loop(period)


if __name__ == "__main__":
    main()
