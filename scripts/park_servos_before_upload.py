"""Pre-upload: shell tiny-engineer park, then daemon stop --yield (fail-open)."""

import os
import shutil
import subprocess
from pathlib import Path


def _warn(msg):
    print("park_servos_before_upload: %s" % msg)


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


def command_argvs(project_dir, upload_port, which=None):
    """Park argv, then `daemon stop --yield`. None when the CLI cannot be resolved."""
    base = cli_base(project_dir, which=which)
    if base is None:
        return None
    park = base + ["park", "--transport", "serial"]
    if upload_port:
        park = park + ["--port", upload_port]
    stop = base + ["daemon", "stop", "--yield"]
    return [park, stop]


def _invoke(runner, argv, project_dir, timeout, label):
    print("park_servos_before_upload: %s" % " ".join(argv))
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
        _warn("%s timed out; upload continues" % label)
        return
    except OSError as exc:
        _warn("%s failed to start (%s); upload continues" % (label, exc))
        return

    if completed.returncode == 0:
        if label == "park":
            print("park_servos_before_upload: parked")
        return

    detail = (completed.stderr or completed.stdout or "").strip()
    if detail:
        _warn("%s failed (%s); upload continues" % (label, detail.splitlines()[-1]))
    else:
        _warn("%s exit %s; upload continues" % (label, completed.returncode))


def park_servos(source, target, env, runner=None):
    project_dir = Path(env.subst("$PROJECT_DIR"))
    upload_port = env.subst("$UPLOAD_PORT").strip()
    argvs = command_argvs(project_dir, upload_port)
    if argvs is None:
        _warn("CLI not found; skip park (upload continues)")
        return

    run = runner or subprocess.run
    _invoke(run, argvs[0], project_dir, 45, "park")
    _invoke(run, argvs[1], project_dir, 15, "daemon stop --yield")


try:
    Import("env")  # noqa: F821 — SCons injects this when PlatformIO loads the script
except Exception:
    env = None

if env is not None:
    env.AddPreAction("upload", park_servos)
