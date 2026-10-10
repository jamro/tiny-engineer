# Bake git-describe into FW_VERSION for the robot firmware env.
#
# CI and release workflows use fetch-depth: 0 so tags are visible
# (shallow checkout yields a bare SHA). Release/CI assets:
# bootloader.bin (0x0), partitions.bin (0x8000), firmware.bin
# (0x10000), and littlefs.bin from `pio run -t buildfs` (0x220000).
Import("env")

import re
import subprocess
from pathlib import Path

_SAFE = re.compile(r"[^A-Za-z0-9._+-]")


def _git_describe(project_dir: Path) -> str:
    try:
        out = subprocess.check_output(
            ["git", "describe", "--tags", "--always", "--dirty"],
            cwd=project_dir,
            stderr=subprocess.DEVNULL,
            text=True,
        )
        return out.strip()
    except (subprocess.CalledProcessError, FileNotFoundError, OSError):
        return "unknown"


def _sanitize(raw: str) -> str:
    cleaned = _SAFE.sub("", raw.strip())
    return cleaned if cleaned else "unknown"


project_dir = Path(env.subst("$PROJECT_DIR"))
version = _sanitize(_git_describe(project_dir))
print(f"Firmware version: {version}")
env.Append(CPPDEFINES=[("FW_VERSION", '\\"%s\\"' % version)])
