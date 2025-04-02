from PySide6.QtCore import QObject, Signal
import serial
import serial.tools.list_ports
from serial.tools.list_ports_common import ListPortInfo
import logging

log = logging.getLogger(__name__)
log.setLevel(logging.DEBUG)

class SerialHandler(QObject):
    data_received = Signal(str)

    def __init__(self):
        super().__init__()
        self.serial_port = None

    def connect(self, port: str, baudrate: int = 115200):
        try:
            self.serial_port = serial.Serial(port, baudrate, timeout=1)
            log.info(f"Connected to {port} at {baudrate} baud.")
        except Exception as e:
            log.error(f"Failed to connect to {port}: {e}")

    def disconnect(self):
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
            log.info("Disconnected from serial port.")

    def read_data(self):
        if self.serial_port and self.serial_port.is_open:
            try:
                if self.serial_port.in_waiting > 0:
                    line = self.serial_port.readline().decode().strip()
                    log.info(f"Data received: {line}")
                    self.data_received.emit(line)
            except Exception as e:
                log.error(f"Error reading data: {e}")

    def search_port(self, vid_pid: str):
        ports = serial.tools.list_ports.comports()
        for port in ports:
            if vid_pid in port.hwid.upper():
                log.info(f"Found device on port: {port.device}")
                return port.device
        log.warning("Device not found.")
        return None
    
    def list_com_ports(cls) -> list[ListPortInfo]:
        ports = serial.tools.list_ports.comports()
        return ports
    
    def is_connected(self):
        return self.serial_port is not None and self.serial_port.is_open
    
    def send_command(self, command: str = '') -> None:
        log.debug('send command: %s', command)
        self.serial_port.write(f'{command}\n'.encode())

    def send(self, data: bytes):
        if data == None or data == '':
            return
        self.serial_port.write(data)

