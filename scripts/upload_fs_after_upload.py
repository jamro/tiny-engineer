Import("env")

import subprocess
import sys
from pathlib import Path

# SCons exec() does not set __file__; resolve via project dir.
_SCRIPTS = Path(env.subst("$PROJECT_DIR")) / "scripts"
if str(_SCRIPTS) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS))

from resume_daemon_after_upload import resume_daemon  # noqa: E402


def upload_filesystem(source, target, env):
    project_dir = Path(env.subst("$PROJECT_DIR"))
    fs_image = project_dir / ".pio" / "build" / env.subst("${PIOENV}") / "littlefs.bin"
    error = None

    # Build the argument vector ourselves instead of handing SCons a command
    # string: SCons splits on whitespace, so a $PROJECT_DIR containing a space
    # would arrive at PlatformIO as two separate arguments.
    # Upload LittleFS to the same board the firmware just went to. The child
    # PlatformIO run does not inherit --upload-port, and auto-detection picks
    # the first serial port it finds when several devices are plugged in.
    upload_port = env.subst("$UPLOAD_PORT")

    def run_pio(pio_target):
        args = [
            env.subst("$PYTHONEXE"),
            "-m", "platformio", "run",
            "-t", pio_target,
            "-e", env.subst("$PIOENV"),
            "-d", str(project_dir),
        ]
        if upload_port and pio_target == "uploadfs":
            args += ["--upload-port", upload_port]
        subprocess.run(args, check=True)

    try:
        if not fs_image.is_file():
            print("LittleFS image missing; building filesystem first")
            run_pio("buildfs")

        print("Uploading LittleFS (bell.wav)...")
        run_pio("uploadfs")
    except Exception as exc:
        error = exc
    finally:
        # Always clear upload yield / re-arm reactions, even when LittleFS fails.
        resume_daemon(source, target, env)

    if error is not None:
        raise error


env.AddPostAction("upload", upload_filesystem)
