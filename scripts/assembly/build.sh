#!/usr/bin/env bash
# Build the 3D viewer's stock SG90 model. Adds M2 screws and nuts
# (scripts/assembly/fasteners.py), checks steps and clipping, then packs the
# GLB (see docs/3d/assembly-viewer.md).
#   scripts/assembly/build.sh [out-dir]   # default web/assembly/models
# Python 3.13 with scripts/assembly/requirements.txt, Node 18+.
set -euo pipefail
cd "$(dirname "$0")/../.."
out="${1:-web/assembly/models}"
work=artifacts/assembly
preset=sg90
python="${PYTHON:-python3}"

"$python" scripts/assembly/extract.py --preset "$preset" --out "$work/$preset"
"$python" scripts/assembly/fasteners.py "$work/$preset/assembly.json"
node scripts/assembly/check_steps.mjs "$work/$preset/assembly.json" web/assembly/steps.json
node scripts/assembly/check_clipping.mjs "$work/$preset/assembly.json" web/assembly/steps.json
node scripts/assembly/build_glb.mjs "$work/$preset/assembly.json" "$out/$preset.glb"
