#!/usr/bin/env python3
"""Argv / invoke for scripts/resume_daemon_after_upload.py. Does not flash."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import resume_daemon_after_upload as resume  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
ENTRY = str(ROOT / "packages" / "tiny-engineer-cli" / "bin" / "tiny-engineer.js")


class _Env:
    def subst(self, key):
        if key == "$PROJECT_DIR":
            return str(ROOT)
        return ""


class _Result:
    def __init__(self, code, stderr="", stdout=""):
        self.returncode = code
        self.stderr = stderr
        self.stdout = stdout


class ResumeArgvTest(unittest.TestCase):
    def test_path_cli_daemon_start(self):
        def which(name):
            if name == "tiny-engineer":
                return "/usr/bin/tiny-engineer"
            return None

        argv = resume.command_argv(ROOT, which=which)
        self.assertEqual(argv, ["/usr/bin/tiny-engineer", "daemon", "start"])

    def test_repo_cli_when_not_on_path(self):
        def which(name):
            if name == "node":
                return "/usr/bin/node"
            return None

        argv = resume.command_argv(ROOT, which=which)
        self.assertEqual(argv, ["/usr/bin/node", ENTRY, "daemon", "start"])

    def test_missing_cli(self):
        self.assertIsNone(resume.command_argv(ROOT, which=lambda _name: None))

    def test_resume_invokes_daemon_start(self):
        calls = []

        def runner(argv, **_kwargs):
            calls.append(argv)
            return _Result(0)

        saved = resume.command_argv
        resume.command_argv = lambda _project_dir, which=None: [
            "/usr/bin/tiny-engineer",
            "daemon",
            "start",
        ]
        try:
            resume.resume_daemon(None, None, _Env(), runner=runner)
        finally:
            resume.command_argv = saved

        self.assertEqual(calls, [["/usr/bin/tiny-engineer", "daemon", "start"]])

    def test_resume_skips_when_cli_missing(self):
        calls = []

        def runner(argv, **_kwargs):
            calls.append(argv)
            return _Result(0)

        saved = resume.command_argv
        resume.command_argv = lambda *_a, **_k: None
        try:
            resume.resume_daemon(None, None, _Env(), runner=runner)
        finally:
            resume.command_argv = saved

        self.assertEqual(calls, [])


if __name__ == "__main__":
    unittest.main()
