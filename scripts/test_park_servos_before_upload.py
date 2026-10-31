#!/usr/bin/env python3
"""Argv order for scripts/park_servos_before_upload.py. Does not flash."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import park_servos_before_upload as park  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
ENTRY = str(ROOT / "packages" / "tiny-engineer-cli" / "bin" / "tiny-engineer.js")


class _Env:
    def __init__(self, port=""):
        self.port = port

    def subst(self, key):
        if key == "$PROJECT_DIR":
            return str(ROOT)
        if key == "$UPLOAD_PORT":
            return self.port
        return ""


class _Result:
    def __init__(self, code, stderr="", stdout=""):
        self.returncode = code
        self.stderr = stderr
        self.stdout = stdout


class ParkArgvTest(unittest.TestCase):
    def test_path_cli_parks_then_yields(self):
        def which(name):
            if name == "tiny-engineer":
                return "/usr/bin/tiny-engineer"
            return None

        argvs = park.command_argvs(ROOT, "/dev/cu.usbmodem", which=which)
        self.assertEqual(
            argvs[0],
            ["/usr/bin/tiny-engineer", "park", "--transport", "serial", "--port", "/dev/cu.usbmodem"],
        )
        self.assertEqual(argvs[1], ["/usr/bin/tiny-engineer", "daemon", "stop", "--yield"])

    def test_repo_cli_when_not_on_path(self):
        def which(name):
            if name == "node":
                return "/usr/bin/node"
            return None

        argvs = park.command_argvs(ROOT, "", which=which)
        self.assertEqual(argvs[0], ["/usr/bin/node", ENTRY, "park", "--transport", "serial"])
        self.assertEqual(argvs[1], ["/usr/bin/node", ENTRY, "daemon", "stop", "--yield"])

    def test_missing_cli(self):
        self.assertIsNone(park.command_argvs(ROOT, "", which=lambda _name: None))

    def test_yield_stop_runs_after_park_failure(self):
        calls = []

        def runner(argv, **_kwargs):
            calls.append(argv)
            if "park" in argv:
                return _Result(1, stderr="no port")
            return _Result(0)

        park.park_servos(None, None, _Env("/dev/cu.usbmodem"), runner=runner)
        self.assertEqual(len(calls), 2)
        self.assertIn("park", calls[0])
        self.assertEqual(calls[1][-3:], ["daemon", "stop", "--yield"])
        self.assertNotIn("--port", calls[1])


if __name__ == "__main__":
    unittest.main()
