Import("env")

from pathlib import Path


def upload_filesystem(source, target, env):
    project_dir = Path(env.subst("$PROJECT_DIR"))
    fs_image = project_dir / ".pio" / "build" / env.subst("${PIOENV}") / "littlefs.bin"

    if not fs_image.is_file():
        print("LittleFS image missing; building filesystem first")
        env.Execute("$PYTHONEXE -m platformio run -t buildfs -d $PROJECT_DIR")

    env.Execute(
        env.VerboseAction(
            "$PYTHONEXE -m platformio run -t uploadfs -d $PROJECT_DIR",
            "Uploading LittleFS (bell.wav)...",
        )
    )


env.AddPostAction("upload", upload_filesystem)
