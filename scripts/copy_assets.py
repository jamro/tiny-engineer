Import("env")

import shutil
from pathlib import Path

project_dir = Path(env.subst("$PROJECT_DIR"))
source = project_dir / "assets" / "bell.wav"
dest_dir = project_dir / "data"
dest = dest_dir / "bell.wav"

if not source.is_file():
    raise FileNotFoundError(f"Missing audio asset: {source}")

dest_dir.mkdir(exist_ok=True)
shutil.copy2(source, dest)

print(f"Copied {source.name} -> {dest}")
