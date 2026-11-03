#!/usr/bin/env bash
# Build the 3D viewer's models: stock SG90 and every mod in web/assembly/mods/.
# Checks every placement and every step on the way, clipping included, and the
# stock robot's joint poses and animations (see docs/3d/assembly-viewer.md).
#   scripts/assembly/build.sh [out-dir]   # default web/assembly/models
# Python 3.13 with scripts/assembly/requirements.txt, Node 18+.
set -euo pipefail
cd "$(dirname "$0")/../.."
out="${1:-web/assembly/models}"
work=artifacts/assembly
preset=sg90
python="${PYTHON:-python3}"

"$python" scripts/assembly/extract.py --preset "$preset" --out "$work/$preset"
node scripts/assembly/check_steps.mjs "$work/$preset/assembly.json" web/assembly/steps.json
node scripts/assembly/check_clipping.mjs "$work/$preset/assembly.json" web/assembly/steps.json
node scripts/assembly/check_poses.mjs "$work/$preset/assembly.json" web/assembly/steps.json
node scripts/assembly/build_glb.mjs "$work/$preset/assembly.json" "$out/$preset.glb"

for config in web/assembly/mods/*.json; do
  mod="$(basename "$config" .json)"
  f3d="$(node -e 'console.log(JSON.parse(require("fs").readFileSync(process.argv[1])).f3d)' "$config")"
  parts="$(node -e 'console.log(JSON.parse(require("fs").readFileSync(process.argv[1])).parts)' "$config")"
  "$python" scripts/assembly/extract.py --mod --f3d "$f3d" --parts "$parts" --preset "$preset" --out "$work/$preset-$mod-raw"
  node scripts/assembly/merge_mod.mjs "$work/$preset/assembly.json" "$work/$preset-$mod-raw/assembly.json" "$config" "$work/$preset-$mod/assembly.json"
  node scripts/assembly/check_steps.mjs "$work/$preset-$mod/assembly.json" web/assembly/steps.json "$config"
  node scripts/assembly/check_clipping.mjs "$work/$preset-$mod/assembly.json" web/assembly/steps.json "$config"
  node scripts/assembly/build_glb.mjs "$work/$preset-$mod/assembly.json" "$out/$preset-$mod.glb"
done
