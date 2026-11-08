#!/usr/bin/env bash
# Copy individual printable meshes into web/print/files/{servo}/ for GitHub Pages.
# Excludes PartsSetA / PartsSetB aggregates (service-order path, not the home queue).
# Usage: from repo root — ./scripts/web/sync-print-files.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PARTS_JSON="${ROOT}/web/print/parts.json"
OUT_ROOT="${ROOT}/web/print/files"
SRC_ROOT="${ROOT}/3d_models/parts"
SERVOS=(sg90 fs0307 es9051)
EXCLUDE_RE='^(PartsSetA|PartsSetB)\.(3mf|stl)$'

if [[ ! -f "${PARTS_JSON}" ]]; then
  echo "missing ${PARTS_JSON}" >&2
  exit 1
fi

rm -rf "${OUT_ROOT}"
mkdir -p "${OUT_ROOT}"

for servo in "${SERVOS[@]}"; do
  dest="${OUT_ROOT}/${servo}"
  mkdir -p "${dest}"
  for fmt in 3mf stl; do
    src="${SRC_ROOT}/${servo}/${fmt}"
    if [[ ! -d "${src}" ]]; then
      echo "missing source dir ${src}" >&2
      exit 1
    fi
    while IFS= read -r -d '' file; do
      base="$(basename "${file}")"
      if [[ "${base}" =~ ${EXCLUDE_RE} ]]; then
        continue
      fi
      cp -f "${file}" "${dest}/${base}"
    done < <(find "${src}" -maxdepth 1 -type f -name "*.${fmt}" -print0)
  done
  echo "synced ${servo}: $(find "${dest}" -type f | wc -l | tr -d ' ') files"
done

# Fail if any file referenced by parts.json is missing for each servo.
python3 - "${PARTS_JSON}" "${OUT_ROOT}" <<'PY'
import json, sys
from pathlib import Path

parts_path = Path(sys.argv[1])
out_root = Path(sys.argv[2])
data = json.loads(parts_path.read_text())
servos = data.get("servos") or {}
items = list(data.get("testers") or []) + list(data.get("parts") or [])
missing = []
for sid, meta in servos.items():
    base = out_root / sid
    for item in items:
        files = item.get("files") or {}
        for key in ("3mf", "stl"):
            name = files.get(key)
            if not name:
                continue
            path = base / name
            if not path.is_file() or path.stat().st_size <= 0:
                missing.append(f"{sid}/{name}")
if missing:
    print("missing print files:", file=sys.stderr)
    for m in missing:
        print(f"  {m}", file=sys.stderr)
    sys.exit(1)
print(f"parts.json OK — {len(items)} items × {len(servos)} servos")
PY

echo "print files ready under ${OUT_ROOT}"
