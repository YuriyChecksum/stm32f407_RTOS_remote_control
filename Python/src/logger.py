import logging
from typing import Optional
from PySide6.QtCore import Signal
from PySide6.QtGui import QTextCharFormat, QColor
from typing import Dict, Tuple

def format_log_record(record: logging.LogRecord) -> Dict[str, Tuple[str, QTextCharFormat]]:
    """Форматирует запись лога для отображения в textEdit."""
    formats = {}

    # Форматирование времени
    time_format = QTextCharFormat()
    time_format.setForeground(QColor("green"))
    formats["time"] = (f"{record.asctime} ", time_format)

    # Форматирование уровня лога
    level_format = QTextCharFormat()
    if record.levelno == logging.DEBUG:
        level_format.setForeground(QColor("blue"))
    elif record.levelno == logging.INFO:
        level_format.setForeground(QColor("green"))
    elif record.levelno == logging.WARNING:
        level_format.setForeground(QColor("orange"))
    elif record.levelno == logging.ERROR:
        level_format.setForeground(QColor("red"))
    elif record.levelno == logging.CRITICAL:
        level_format.setForeground(QColor("darkred"))
    # formats["level"] = (f"{record.levelname}: ", level_format) # раскомментировать если нужно время лога

    # Форматирование текста сообщения
    message_format = QTextCharFormat()
    message_format.setForeground(QColor("black"))
    formats["message"] = (f"{record.getMessage()}\n", message_format)

    return formats

class TextEditLogger(logging.Handler):
    """Обработчик логов для вывода сообщений в textEdit через сигнал."""
    def __init__(self, log_signal: Signal) -> None:
        super().__init__()
        self.log_signal = log_signal

    def emit(self, record: logging.LogRecord) -> None:
        """Вывод сообщения в textEdit с форматированием."""
        try:
            # Форматируем сообщение
            msg = self.format(record)  # Вызов обязателен для корректной обработки
            # Отправляем объект записи лога
            self.log_signal.emit(record) # .emit(msg)
        except Exception:
            self.handleError(record)

class CustomLogFilter(logging.Filter):
    """Фильтр для логов, позволяющий фильтровать по уровню и модулю."""
    def __init__(
            self, 
            level: Optional[int] = None, 
            module_name: Optional[str] = None
        ) -> None:
        super().__init__()
        self.level = level
        self.module_name = module_name

    def filter(self, record: logging.LogRecord) -> bool:
        if self.level is not None and record.levelno < self.level:
            return False
        if self.module_name is not None and not record.name.startswith(self.module_name):
            return False
        return True

def setup_logger(
    log_signal: Signal, 
    level: int = logging.DEBUG, 
    module_name: Optional[str] = None
) -> TextEditLogger:
    """Настраивает логгер с обработчиком для textEdit."""

    # Создаем обработчик для textEdit
    log_handler = TextEditLogger(log_signal)
    log_handler.setLevel(level)
    log_handler.addFilter(CustomLogFilter(logging.INFO, module_name))  # Применяем фильтр
    formatter = logging.Formatter('%(asctime)s %(levelname)-8s: %(message)s', datefmt='%H:%M:%S')
    log_handler.setFormatter(formatter)

    # Если хотим вернуть непосредственно готовый логгер
    # log = logging.getLogger(__name__)
    # log.setLevel(level)
    # log.addHandler(log_handler)
    # return log

    return log_handler