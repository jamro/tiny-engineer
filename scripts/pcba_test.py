#!/usr/bin/env python3
"""PCBA host for the golden-module line protocol. Does not flash."""

import argparse
import os
import sys
import time

WEB_TESTER = "https://tinyengineer.dev/pcba_test"
BOARD_REV = "Compatible with main control board rev 3."
FIRMWARE_LINE = "The ESP32 must already run the test firmware."
FIRMWARE_DOC = (
    "https://github.com/jamro/tiny-engineer/blob/main/docs/hardware/"
    "pcba-test.md#get-an-esp32-with-the-test-firmware"
)
ESPRESSIF_VID = 0x303A


class PortLost(OSError):
    pass


HEADER = """\
   .---.
  | o o |  TINY ENGINEER
  |  ^  |  PCBA test
   '---'
"""


def startup_text():
    return "%sWeb tester: %s\n%s\n%s\nHow to get one: %s\n" % (
        HEADER,
        WEB_TESTER,
        BOARD_REV,
        FIRMWARE_LINE,
        FIRMWARE_DOC,
    )


def prep_text():
    return card(
        [
            "PREPARE",
            "",
            "1  Take the board.",
            "2  Plug in the ESP32.",
            "   Match the USB mark on the board.",
            "3  Plug the speaker into the JST speaker port.",
            "4  Connect USB to the board.",
            "   Use the board socket, not the ESP32.",
        ]
    )


READY_COACH = "The board is connected."
BANNER_COACH = (
    "The module LED stays steady white while the test image has power. "
    "That lamp is not the board verdict."
)
PCA_COACH = "PCA9685 did not answer. This board fails before the speaker test."
WAIT_KEYS = ("Enter = good", "n = bad")
TONE_KEYS = ("+ = louder", "- = quieter")
TONE_LEVELS = 10
TONE_LEVEL_DEFAULT = 6
FAIL_KEYS = (
    "r = try this step again",
    "c = keep the fail, go on",
    "a = start this board over",
)
STEPS = (
    ("tone", "SPEAKER", "Speaker on the JST speaker port. Expect a repeating tone."),
    (
        "servos",
        "SERVOS",
        "Signal is on all five servo outputs. Take the testing board. Plug it into one column at a time. Columns are HEAD, NECK, ARM L, ARM R, and BODY. Check the LEDs on every column. Red stays on and yellow pulses.",
    ),
    (
        "oe",
        "OE OUTPUT ENABLE",
        "The red LED stays on. Only the yellow LED goes out. No need to test each servo column.",
    ),
)
STEP_TEXT = {name: sentence for name, _title, sentence in STEPS}
STEP_TITLE = {name: title for name, title, _sentence in STEPS}


def new_state():
    return {"shown": "", "menu": False, "volume": TONE_LEVEL_DEFAULT, "step": ""}


def card(lines, footer=None):
    body = list(lines)
    foot = list(footer or [])
    width = max(len(line) for line in body + foot)
    rule = "─" * (width + 2)
    framed = ["┌" + rule + "┐"]
    for line in body:
        framed.append("│ " + line.ljust(width) + " │")
    if foot:
        framed.append("├" + rule + "┤")
        for line in foot:
            framed.append("│ " + line.ljust(width) + " │")
    framed.append("└" + rule + "┘")
    return "\n".join(framed)


def prose(text):
    return [part if part.endswith(".") else part + "." for part in text.split(". ") if part]


def coach_step(name, volume=None):
    index = next(i for i, (step, _title, _sentence) in enumerate(STEPS) if step == name)
    lines = ["%d/%d  %s" % (index + 1, len(STEPS), STEP_TITLE[name]), ""]
    lines.extend(prose(STEP_TEXT[name]))
    footer = WAIT_KEYS
    if name == "tone":
        level = TONE_LEVEL_DEFAULT if volume is None else volume
        lines.append("Volume %d/%d." % (level, TONE_LEVELS))
        footer = TONE_KEYS + WAIT_KEYS
    return card(lines, footer=footer)


def coach_fail(name, token):
    if name == "pca":
        return card(["PCA", ""] + prose(PCA_COACH) + ["", token])
    if name not in STEP_TEXT:
        return token
    lines = [STEP_TITLE[name], ""]
    lines.extend(prose(STEP_TEXT[name]))
    lines.append("")
    lines.extend(FAIL_KEYS)
    lines.extend(["", token])
    return card(lines)


def coach_ready():
    return card(["CONNECTED", ""] + prose(READY_COACH), footer=("Enter = continue",))


def feed_line(state, line):
    text = line.strip()
    if not text:
        return False
    if text.startswith("STEP "):
        name = text[5:]
        state["step"] = name
        state["shown"] = coach_step(name, state.get("volume")) if name in STEP_TEXT else text
        state["menu"] = False
    elif text.startswith("VOL "):
        try:
            level = int(text[4:])
        except ValueError:
            return False
        if level < 1 or level > TONE_LEVELS:
            return False
        state["volume"] = level
        if state.get("step") == "tone":
            state["shown"] = coach_step("tone", level)
        state["menu"] = False
    elif text.startswith("FAIL "):
        name = text[5:]
        state["shown"] = coach_fail(name, text)
        state["menu"] = name in STEP_TEXT
    elif text.startswith("RESULT "):
        state["shown"] = card(["BOARD", "", text, "", next_action_text().strip()])
        state["menu"] = False
    elif text == "READY":
        state["shown"] = coach_ready()
        state["menu"] = False
    elif text.startswith("PCBA-TEST"):
        state["shown"] = "%s\n%s" % (BANNER_COACH, text)
        state["menu"] = False
    elif text == "STOP":
        state["shown"] = text
        state["menu"] = False
    else:
        return False
    state["echo"] = not text.startswith("PCBA-TEST")
    return True


def display(state):
    return state["shown"]


KEY_CHARS = frozenset("nrcaq+-=")


def token_for_char(ch):
    if ch in ("\n", "\r"):
        return "enter"
    if ch in KEY_CHARS:
        return ch
    return None


def windows_key(ch, read_extended):
    if ch in ("\x00", "\xe0"):
        read_extended()
        return None
    return token_for_char(ch)


def encode_key(key):
    if key == "enter":
        return b"\n"
    if key in ("+", "="):
        return b"+"
    if key in ("n", "r", "c", "a", "q", "-"):
        return key.encode("ascii")
    raise ValueError(key)


def next_action_text():
    return "unplug USB for the next board, or q to stop\n"


def read_chunk(port):
    """Bytes already buffered, or one fake-port line. Does not block the keyboard.

    macOS CDC often leaves in_waiting at 0 until read() polls the kernel, so a
    select-then-in_waiting loop never sees the banner.
    """
    if not hasattr(port, "fileno"):
        return port.readline()
    try:
        waiting = getattr(port, "in_waiting", 0) or 0
        if waiting:
            return port.read(waiting)
        timeout = port.timeout
        port.timeout = 0.05
        try:
            return port.read(4096) or b""
        finally:
            port.timeout = timeout
    except OSError as exc:
        # macOS pyserial raises when a CDC read is signaled but returns nothing.
        if "no data" in str(exc).lower():
            return b""
        raise PortLost(str(exc)) from exc


def serve(open_port, out, key_source=None, max_lines=None):
    """Read device lines until STOP, max_lines protocol lines, or the opener is done.

    open_port() returns a port or None. A port raises PortLost when the cable drops.
    """
    if key_source is None:
        key_source = lambda: None
    state = new_state()
    out.write(startup_text())
    out.write("\n")
    out.write(prep_text())
    out.write("\n")
    out.flush()
    port = None
    pending = b""
    seen = 0
    painted = {"screen": None}

    def emit(screen):
        if screen == painted["screen"]:
            return
        painted["screen"] = screen
        out.write("\n")
        out.write(screen + "\n")
        out.flush()

    while max_lines is None or seen < max_lines:
        if port is None:
            try:
                port = open_port()
            except PortLost:
                port = None
            if port is None:
                if max_lines is not None:
                    break
                time.sleep(0.3)
                continue
        try:
            raw = read_chunk(port)
        except PortLost:
            port = None
            pending = b""
            continue
        if raw:
            pending += raw
        changed = False
        while b"\n" in pending and (max_lines is None or seen < max_lines):
            chunk, pending = pending.split(b"\n", 1)
            line = chunk.decode("utf-8", "replace")
            if not feed_line(state, line):
                continue
            text = line.strip()
            changed = True
            seen += 1
            if text == "STOP":
                if changed and state.get("echo", True):
                    emit(display(state))
                return state
        if changed and state.get("echo", True):
            emit(display(state))
        key = key_source()
        if key and port is not None:
            try:
                port.write(encode_key(key))
            except OSError:
                port = None
                pending = b""
        if max_lines is None and not raw and key is None and not hasattr(port, "fileno"):
            time.sleep(0.02)
    return state


def cdc_devices(list_ports):
    found = []
    for port in list_ports:
        if getattr(port, "vid", None) == ESPRESSIF_VID and getattr(port, "device", None):
            found.append(port.device)
    return found


def port_open_message(exc):
    text = str(exc).lower()
    if "busy" in text or "permission" in text or "access is denied" in text:
        return (
            "Port is busy. Close the other program using the board. "
            "This script will keep waiting."
        )
    return "Port did not open. Unplug the board and plug it back in."


def open_cdc(explicit, list_ports, serial_module):
    if explicit:
        return open_serial(serial_module, explicit)
    devices = cdc_devices(list_ports())
    if len(devices) != 1:
        return None, devices
    return open_serial(serial_module, devices[0]), devices


def open_serial(serial_module, device):
    port = serial_module.Serial()
    port.port = device
    port.baudrate = 115200
    port.timeout = 0
    port.write_timeout = 1
    # Leave DTR and RTS low so opening the port does not reset the module.
    port.dtr = False
    port.rts = False
    port.open()
    # One byte marks the USB link up. It is not Enter.
    try:
        port.write(b"?")
    except OSError:
        pass
    return port


def read_key_nonblocking():
    if os.name == "nt":
        import msvcrt

        if not msvcrt.kbhit():
            return None
        return windows_key(msvcrt.getwch(), msvcrt.getwch)
    import select

    if not select.select([sys.stdin], [], [], 0)[0]:
        return None
    return token_for_char(sys.stdin.read(1))


def configure_stdin():
    if os.name == "nt" or not sys.stdin.isatty():
        return None
    import termios
    import tty

    fd = sys.stdin.fileno()
    previous = termios.tcgetattr(fd)
    tty.setcbreak(fd)
    return previous


def restore_stdin(previous):
    if previous is None:
        return
    import termios

    termios.tcsetattr(sys.stdin.fileno(), termios.TCSADRAIN, previous)


def main(argv=None):
    parser = argparse.ArgumentParser(description="PCBA test host")
    parser.add_argument("--port", help="CDC serial device")
    args = parser.parse_args(argv)
    try:
        import serial
        from serial.tools import list_ports
    except ImportError:
        sys.stderr.write("Install pyserial: python3 -m pip install pyserial\n")
        return 1

    announced = {"waiting": False}

    def open_port():
        try:
            opened, devices = open_cdc(args.port, list_ports.comports, serial)
        except serial.SerialException as exc:
            text = port_open_message(exc)
            if announced.get("error") != text:
                sys.stdout.write("\n%s\n" % text)
                sys.stdout.flush()
                announced["error"] = text
            return None
        if opened is not None:
            announced["waiting"] = False
            announced["error"] = None
            return opened
        if not announced["waiting"]:
            if devices:
                sys.stdout.write("CDC ports: %s\n" % ", ".join(devices))
            sys.stdout.flush()
            announced["waiting"] = True
        return None

    previous = configure_stdin()
    try:
        serve(open_port, sys.stdout, key_source=read_key_nonblocking)
    except KeyboardInterrupt:
        return 0
    finally:
        restore_stdin(previous)
    return 0


if __name__ == "__main__":
    sys.exit(main())
