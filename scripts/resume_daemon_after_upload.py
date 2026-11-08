"""Post-upload helper: clear yield and re-arm serial reactions (fail-open).

Called from `upload_fs_after_upload.py` after the LittleFS step (success or
fail) so a raised FS error cannot skip the resume.
"""

import os
import shutil
import subprocess
from pathlib import Path


def _warn(msg):
    print("resume_daemon_after_upload: %s" % msg)


def cli_base(project_dir, which=None):
    """Prefer PATH tiny-engineer; else node plus the repo CLI entry."""
    which = which or shutil.which
    project_dir = Path(project_dir)
    on_path = which("tiny-engineer")
    if on_path:
        return [on_path]
    entry = project_dir / "packages" / "tiny-engineer-cli" / "bin" / "tiny-engineer.js"
    if not entry.is_file():
        return None
    node = which("node")
    if not node:
        return None
    return [node, str(entry)]


def command_argv(project_dir, which=None):
    """`daemon start` argv. None when the CLI cannot be resolved."""
    base = cli_base(project_dir, which=which)
    if base is None:
        return None
    return base + ["daemon", "start"]


def _invoke(runner, argv, project_dir, timeout):
    print("resume_daemon_after_upload: %s" % " ".join(argv))
    try:
        completed = runner(
            argv,
            cwd=str(project_dir),
            capture_output=True,
            text=True,
            timeout=timeout,
            env=os.environ.copy(),
        )
    except subprocess.TimeoutExpired:
        _warn("daemon start timed out; clear yield manually with tiny-engineer daemon start")
        return
    except OSError as exc:
        _warn("daemon start failed to start (%s)" % exc)
        return

    if completed.returncode == 0:
        print("resume_daemon_after_upload: daemon started")
        return

    detail = (completed.stderr or completed.stdout or "").strip()
    if detail:
        _warn("daemon start failed (%s)" % detail.splitlines()[-1])
    else:
        _warn("daemon start exit %s" % completed.returncode)


def resume_daemon(source, target, env, runner=None):
    project_dir = Path(env.subst("$PROJECT_DIR"))
    argv = command_argv(project_dir)
    if argv is None:
        _warn("CLI not found; skip resume (run tiny-engineer daemon start)")
        return

    _invoke(runner or subprocess.run, argv, project_dir, 30)
