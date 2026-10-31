#!/usr/bin/env python3
"""Host checks for scripts/pcba_test.py. Does not flash."""

import io
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from pcba_test import (  # noqa: E402
    BOARD_REV,
    FIRMWARE_DOC,
    FIRMWARE_LINE,
    PortLost,
    STEPS,
    WEB_TESTER,
    display,
    port_open_message,
    encode_key,
    windows_key,
    feed_line,
    new_state,
    next_action_text,
    serve,
    prep_text,
    startup_text,
)

ROOT = Path(__file__).resolve().parents[1]
CARD = ROOT / "docs" / "hardware" / "pcba-test.md"


class FakePort:
    def __init__(self, lines, drop_when_empty=False):
        self.lines = list(lines)
        self.drop_when_empty = drop_when_empty
        self.writes = []

    def readline(self):
        if not self.lines:
            if self.drop_when_empty:
                raise PortLost("cable")
            return b""
        return (self.lines.pop(0) + "\n").encode()

    def write(self, data):
        self.writes.append(data)


def play(script):
    state = new_state()
    shown = []
    writes = []
    for kind, value in script:
        if kind == "line":
            feed_line(state, value)
            shown.append(display(state))
        else:
            writes.append(encode_key(value))
    return shown, writes, state


class HostTests(unittest.TestCase):
    def test_windows_key_ignores_unmapped_and_extended(self):
        extra = []

        def read_extended():
            extra.append("scan")

        for ch in ("x", " ", "\x03"):
            extra.clear()
            self.assertIsNone(windows_key(ch, read_extended))
            self.assertEqual(extra, [])

        for prefix in ("\x00", "\xe0"):
            extra.clear()
            self.assertIsNone(windows_key(prefix, read_extended))
            self.assertEqual(extra, ["scan"])

        self.assertEqual(windows_key("\r", read_extended), "enter")
        self.assertEqual(windows_key("n", read_extended), "n")
        self.assertEqual(windows_key("+", read_extended), "+")

    def test_busy_port_waits_without_a_traceback(self):
        self.assertIn("Port is busy", port_open_message(OSError("Resource busy")))
        self.assertIn("keep waiting", port_open_message(OSError("[Errno 16] Resource busy")))

        def open_port():
            raise PortLost("could not open port /dev/cu.usbmodem: Resource busy")

        out = io.StringIO()
        serve(open_port, out, max_lines=1)
        self.assertIn("PREPARE", out.getvalue())
        self.assertNotIn("Traceback", out.getvalue())

    def test_startup_names_web_tester(self):
        self.assertIn(WEB_TESTER, startup_text())
        self.assertIn(BOARD_REV, startup_text())
        self.assertIn(FIRMWARE_LINE, startup_text())
        self.assertIn(FIRMWARE_DOC, startup_text())
        prep = prep_text()
        self.assertIn("Take the board.", prep)
        self.assertIn("Match the USB mark on the board.", prep)
        self.assertIn("Plug the speaker into the JST speaker port.", prep)
        self.assertIn("Use the board socket, not the ESP32.", prep)

    def test_connected_board_waits_for_confirmation(self):
        port = FakePort(["READY"])

        def open_port():
            return port

        out = io.StringIO()
        serve(open_port, out, max_lines=1)
        log = out.getvalue()
        self.assertEqual(port.writes, [])
        self.assertIn("The board is connected.", log)
        self.assertIn("Enter = continue", log)
        self.assertNotIn("1/3", log)

    def test_ready_waits_for_the_operator(self):
        shown, _writes, _state = play([("line", "READY")])
        self.assertIn("CONNECTED", shown[0])
        self.assertIn("The board is connected.", shown[0])
        self.assertIn("Enter = continue", shown[0])
        self.assertNotIn("speaker", shown[0].lower())
        self.assertNotIn("1/3", shown[0])

    def test_accept_path_and_fail_menu(self):
        shown, writes, state = play(
            [
                ("line", "PCBA-TEST v1"),
                ("line", "STEP tone"),
                ("key", "enter"),
                ("line", "STEP servos"),
                ("line", "STEP oe"),
                ("line", "RESULT PASS"),
            ]
        )
        self.assertIn("1/3", shown[1])
        self.assertIn("SPEAKER", shown[1])
        self.assertIn("Speaker on the JST speaker port.", shown[1])
        self.assertNotIn("Lamps do not count.", shown[1])
        self.assertIn("Expect a repeating tone.", shown[1])
        self.assertNotIn("STEP tone", shown[1])
        self.assertIn("Volume 6/10.", shown[1])
        self.assertIn("+ = louder", shown[1])
        self.assertIn("- = quieter", shown[1])
        self.assertIn("Enter = good", shown[1])
        self.assertIn("n = bad", shown[1])
        self.assertIn("├", shown[1])
        self.assertIn("2/3", shown[2])
        self.assertIn("SERVOS", shown[2])
        self.assertNotIn("+ = louder", shown[2])
        self.assertNotIn("Volume", shown[2])
        self.assertIn("Signal is on all five servo outputs.", shown[2])
        self.assertIn("Plug it into one column at a time.", shown[2])
        self.assertIn("Columns are HEAD, NECK, ARM L, ARM R, and BODY.", shown[2])
        self.assertIn("Check the LEDs on every column.", shown[2])
        self.assertIn("Red stays on and yellow pulses.", shown[2])
        self.assertNotIn("No key between columns.", shown[2])
        self.assertNotIn("STEP servos", shown[2])
        self.assertIn("3/3", shown[3])
        self.assertIn("OE OUTPUT ENABLE", shown[3])
        self.assertIn("The red LED stays on.", shown[3])
        self.assertIn("Only the yellow LED goes out.", shown[3])
        self.assertIn("No need to test each servo column.", shown[3])
        self.assertNotIn("PWM", shown[3])
        self.assertNotIn("5 V", shown[3])
        self.assertNotIn("STEP oe", shown[3])
        self.assertIn("├", shown[2])
        self.assertEqual(writes, [b"\n"])
        self.assertEqual(encode_key("+"), b"+")
        self.assertEqual(encode_key("="), b"+")
        self.assertEqual(encode_key("-"), b"-")
        louder, _writes, louder_state = play([("line", "STEP tone"), ("line", "VOL 8")])
        self.assertIn("Volume 8/10.", louder[1])
        self.assertEqual(louder_state["volume"], 8)
        self.assertIn("RESULT PASS", shown[-1])

        shown, writes, _state = play(
            [
                ("line", "STEP servos"),
                ("key", "n"),
                ("line", "FAIL servos"),
                ("key", "c"),
            ]
        )
        self.assertIn("2/3", shown[0])
        self.assertNotIn("STEP servos", shown[0])
        self.assertIn("Red stays on and yellow pulses.", shown[1])
        self.assertIn("r = try this step again", shown[1])
        self.assertIn("c = keep the fail, go on", shown[1])
        self.assertIn("a = start this board over", shown[1])
        self.assertIn("FAIL servos", shown[1])
        self.assertEqual(writes, [b"n", b"c"])

        banner, _writes, _state = play([("line", "PCBA-TEST v1")])
        self.assertNotIn("module LED stays steady white", banner[0])
        self.assertIn("PCBA-TEST v1", banner[0])

        pca, _writes, _state = play([("line", "FAIL pca")])
        self.assertIn("PCA9685 probe failed at I2C 0x40.", pca[0])
        self.assertIn("this board stops before the speaker test.", pca[0])
        self.assertIn("3.3 V, not 5 V.", pca[0])
        self.assertNotIn("r = try this step again", pca[0])
        self.assertNotIn("c = keep the fail, go on", pca[0])
        self.assertNotIn("a = start this board over", pca[0])

        verdict, _writes, _state = play(
            [("line", "FAIL pca"), ("line", "RESULT FAIL pca")]
        )
        self.assertIn("PCA9685 probe failed at I2C 0x40.", verdict[-1])
        self.assertIn("this board stops before the speaker test.", verdict[-1])
        self.assertIn("3.3 V, not 5 V.", verdict[-1])
        self.assertIn("RESULT FAIL pca", verdict[-1])
        self.assertIn("unplug USB for the next board, or q to stop", verdict[-1])
        self.assertNotIn("r = try this step again", verdict[-1])

        page = (ROOT / "web" / "js" / "pcba-test.mjs").read_text()
        script = (ROOT / "scripts" / "pcba_test.py").read_text()
        for _name, _title, sentence in STEPS:
            self.assertIn(sentence, page)
            self.assertIn(sentence, script)

    def test_reconnect_shows_the_next_session(self):
        ports = [
            FakePort(["RESULT FAIL tone body"], drop_when_empty=True),
            FakePort(["PCBA-TEST v1"]),
        ]

        def open_port():
            if not ports:
                return None
            return ports.pop(0)

        out = io.StringIO()
        state = serve(open_port, out, max_lines=2)
        log = out.getvalue()

        self.assertIn("RESULT FAIL tone body", log)
        self.assertIn("PCBA-TEST v1", state["shown"])

    def test_write_oserror_opens_the_next_port(self):
        class WriteFails(FakePort):
            def write(self, data):
                raise OSError("device disconnected")

        ports = [
            WriteFails(["READY"]),
            FakePort(["PCBA-TEST v1"]),
        ]
        keys = iter(["q"])

        def open_port():
            if not ports:
                return None
            return ports.pop(0)

        def key_source():
            return next(keys, None)

        out = io.StringIO()
        state = serve(open_port, out, key_source=key_source, max_lines=2)

        self.assertNotIn("Traceback", out.getvalue())
        self.assertIn("PCBA-TEST v1", state["shown"])

    def test_result_says_what_to_do_and_stop_returns(self):
        port = FakePort(["RESULT PASS", "STOP"])

        def open_port():
            return port

        out = io.StringIO()
        serve(open_port, out)
        log = out.getvalue()
        self.assertIn("RESULT PASS", log)
        self.assertIn(next_action_text().strip(), log)
        self.assertIn("STOP", log)

    def test_scripted_sessions_match_the_card(self):
        pass_shown, _writes, _state = play(
            [
                ("line", "STEP tone"),
                ("key", "enter"),
                ("line", "STEP servos"),
                ("key", "enter"),
                ("line", "RESULT PASS"),
            ]
        )
        fail_shown, fail_writes, _state = play(
            [
                ("line", "STEP tone"),
                ("key", "n"),
                ("line", "FAIL tone"),
                ("key", "c"),
                ("line", "STEP servos"),
                ("key", "n"),
                ("line", "FAIL servos"),
                ("key", "c"),
                ("line", "RESULT FAIL tone servos"),
            ]
        )
        self.assertIn("RESULT PASS", pass_shown[-1])
        self.assertIn("RESULT FAIL tone servos", fail_shown[-1])
        self.assertIn(b"n", fail_writes)
        self.assertIn(b"c", fail_writes)

        card = CARD.read_text()
        for phrase in (
            "tone",
            "servos",
            "HEAD",
            "NECK",
            "ARM L",
            "ARM R",
            "BODY",
            "testing board",
            "oe",
            "+ = louder",
            "- = quieter",
            "Only the yellow LED goes out",
            "No need to test each servo column",
            "RESULT PASS",
            "RESULT FAIL tone servos",
            "python3 scripts/pcba_test.py",
            WEB_TESTER,
            "main control board rev 3",
            "Red stays on and yellow pulses",
            "square pad as GND",
            "pin labels on both boards",
            "PWM to PWM",
            "J1",
            "golden module stays with the tester",
            "The ESP32 must already run the test firmware",
            "pio run -e pcba-test -t upload",
            "The product firmware cannot run this test",
            "J2",
            "OLED",
            "Wi-Fi",
            "2 A load",
        ):
            self.assertIn(phrase, card)
        for key in ("r", "c", "a", "n", "q"):
            self.assertIn("`%s`" % key, card)


if __name__ == "__main__":
    unittest.main()
