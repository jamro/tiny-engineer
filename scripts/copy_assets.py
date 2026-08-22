Import("env")

import shutil
from pathlib import Path

project_dir = Path(env.subst("$PROJECT_DIR"))
assets_dir = project_dir / "assets"
dest_dir = project_dir / "data"

dest_dir.mkdir(exist_ok=True)

for name in ("bell.wav", "welcome.wav", "attention.wav"):
    source = assets_dir / name
    if not source.is_file():
        raise FileNotFoundError(f"Missing audio asset: {source}")
    dest = dest_dir / name
    shutil.copy2(source, dest)
    print(f"Copied {source.name} -> {dest}")
