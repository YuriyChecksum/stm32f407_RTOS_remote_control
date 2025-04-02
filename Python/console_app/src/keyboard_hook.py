"""Вешает хуки на клавиатурные нажатия, например завершение программы по ESC"""

import keyboard  # pip install keyboard
import time
import logging

log = logging.getLogger(__name__)
log.setLevel(logging.DEBUG)

_interrupt = False  # флаг для прерывания потока
_last_state_key = False


# if keyboard.is_pressed('space') or interrupt: break
# import msvcrt  # https://www.oreilly.com/library/view/python-standard-library/0596000960/ch12s11.html
# if msvcrt.kbhit(): break # работает только в консоли

def sleep_break(t: float = 0, t2: float = 0.1):
    """
    Замена функции time.sleep,
    позволяет обрабатывать клавиатурные хуки во время ожидания.
    """
    t_start = time.perf_counter()
    if t < t2:
        t2 = t
    while time.perf_counter() - t_start < t:
        time.sleep(t2)
        if is_break():
            return True
    return False


def is_break():
    global _last_state_key, _interrupt
    if _interrupt:
        raise KeyboardInterrupt("Exit by hook 'Esc'")

    if _last_state_key != keyboard.is_pressed('space'):
        _last_state_key = keyboard.is_pressed('space')
        return False
    elif keyboard.is_pressed('esc'):
        return True
    else:
        return False


def kb_hook_default(key):
    global _interrupt
    if key.event_type == "down":
        if key.name == "esc":
            log.info("Brake by 'esc' 'keyboard.hook'")
            _interrupt = True  # для прерывания потока


def loop(func: callable, n: int, sleep: float = 1) -> None:
    """example: loop(lambda: print('loop'), 5, 0.1)"""
    while n > 0:
        n -= 1
        func()
        if sleep_break(sleep):
            return


def timeloop(func: callable, timer: float = 1, sleep: float = 0.5):
    """Вызывает func через каждые sleep сек в течении timer сек.
    В паузах опрашивает клавиатурные хуки"""

    t_start = time.perf_counter()
    while time.perf_counter() - t_start < timer:
        func()
        if sleep_break(sleep):
            return


def hook(fun: callable) -> None:
    """передать свой хук"""
    keyboard.hook(fun)


def hook_default() -> None:
    """хук по умолчанию"""
    keyboard.hook(kb_hook_default)


def unhook() -> None:
    """отвязать"""
    keyboard.unhook_all()


# пример хука на клавиатуру
def kb_hook_example(key):
    "пример хука на клавиатуру"
    global bright, _interrupt
    min_br = 0.0
    max_br = 0.4

    if key.event_type == "down":
        if key.name == "esc":
            # keyboard.unhook_all()
            print(f"exit by 'esc'")
            _interrupt = True  # для прерывания потока
            # raise RuntimeError('exit исключение')
            # exit()
        elif key.name == "up":
            tmp = bright + 0.01
            if tmp > max_br:
                tmp = max_br
            bright = tmp
            print(f'set bright: {bright:.2}')
        elif key.name == "down":
            tmp = bright - 0.01
            if tmp < min_br:
                tmp = min_br
            bright = tmp
            print(f'set bright: {bright:.2}')


if __name__ == "__main__":
    # if sleep_break(1): return
    # if is_break(): break

    hook(kb_hook_default)
    state = False
    while True:
        try:
            b = is_break()
            if state != b:
                state = b
                if b:
                    log.info("%s, %s", b, _interrupt)
                time.sleep(0.05)
        except BaseException as err:  # Exception('Exit by hook Esc')
            log.info(err)
            _interrupt = False  # для прерывания потока
    unhook()
