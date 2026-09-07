Import("env")

import subprocess
from pathlib import Path


def upload_filesystem(source, target, env):
    project_dir = Path(env.subst("$PROJECT_DIR"))
    fs_image = project_dir / ".pio" / "build" / env.subst("${PIOENV}") / "littlefs.bin"

    # Build the argument vector ourselves instead of handing SCons a command
    # string: SCons splits on whitespace, so a $PROJECT_DIR containing a space
    # would arrive at PlatformIO as two separate arguments.
    def run_pio(pio_target):
        subprocess.run(
            [
                env.subst("$PYTHONEXE"),
                "-m", "platformio", "run",
                "-t", pio_target,
                "-d", str(project_dir),
            ],
            check=True,
        )

    if not fs_image.is_file():
        print("LittleFS image missing; building filesystem first")
        run_pio("buildfs")

    print("Uploading LittleFS (bell.wav)...")
    run_pio("uploadfs")


env.AddPostAction("upload", upload_filesystem)
