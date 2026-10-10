Import("env")

import os
import subprocess
from pathlib import Path
from urllib.parse import urlparse

DEFAULT_HOST = "tiny-engineer.local"


def ota_host():
    url = os.environ.get("TINY_ENGINEER_URL", "")
    # urlparse only finds a hostname after "//"; accept a bare host or IP too.
    host = urlparse(url if "//" in url else f"//{url}").hostname
    return host or DEFAULT_HOST


def espota(image, filesystem):
    command = [
        env.subst("$PYTHONEXE"),
        str(Path(env.PioPlatform().get_package_dir("framework-arduinoespressif32")) / "tools" / "espota.py"),
        "--ip", ota_host(),
        "--file", str(image),
        "--progress",
    ]

    token = os.environ.get("TINY_ENGINEER_TOKEN")

    if token:
        command += ["--auth", token]

    if filesystem:
        command.append("--spiffs")

    print(f"OTA {'filesystem' if filesystem else 'firmware'} -> {ota_host()}")
    subprocess.run(command, check=True)


def upload_firmware(source, target, env):
    espota(env.subst("$BUILD_DIR/${PROGNAME}.bin"), filesystem=False)


def upload_filesystem(source, target, env):
    project_dir = env.subst("$PROJECT_DIR")
    # The platform only wires up the LittleFS image builder when buildfs is a command-line target.
    subprocess.run(
        [env.subst("$PYTHONEXE"), "-m", "platformio", "run", "-t", "buildfs", "-e", env.subst("$PIOENV"), "-d", project_dir],
        check=True,
    )
    espota(env.subst("$BUILD_DIR/${ESP32_FS_IMAGE_NAME}.bin"), filesystem=True)


env.AddCustomTarget(
    name="ota",
    dependencies="$BUILD_DIR/${PROGNAME}.bin",
    actions=upload_firmware,
    title="Upload OTA",
    description="Upload firmware over Wi-Fi",
)

env.AddCustomTarget(
    name="otafs",
    dependencies=None,
    actions=upload_filesystem,
    title="Upload Filesystem OTA",
    description="Upload LittleFS image over Wi-Fi",
)
