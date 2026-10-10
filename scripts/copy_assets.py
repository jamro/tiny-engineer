Import("env")

import sys
from pathlib import Path

project_dir = Path(env.subst("$PROJECT_DIR"))
scripts_dir = project_dir / "scripts"
if str(scripts_dir) not in sys.path:
    sys.path.insert(0, str(scripts_dir))

from audio_pack import pack_audio, spiffs_size_bytes, validate_mod_name

mod_name = env.GetProjectOption("custom_audio_mod", "")
mod_name = "" if mod_name is None else str(mod_name).strip()
mod_dir = None
if mod_name:
    validate_mod_name(mod_name)
    mod_dir = project_dir / "mods" / mod_name / "assets"
    print(f"Audio mod overlay: {mod_name}")

pack_audio(
    project_dir / "assets",
    project_dir / "data",
    spiffs_size_bytes(project_dir / env.GetProjectOption("board_build.partitions")),
    mod_dir,
)
