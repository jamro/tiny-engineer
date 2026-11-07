#!/usr/bin/env bash
# Refresh web/vendor/ from pinned npm packages (three + esp-web-tools).
# Usage (repo root): ./scripts/web/vendor-libs.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

THREE_VER="${THREE_VER:-0.186.0}"
EWT_VER="${EWT_VER:-10.4.0}"

VENDOR="$ROOT/web/vendor"
THREE_DST="$VENDOR/three"
EWT_DST="$VENDOR/esp-web-tools"

die() { echo "error: $*" >&2; exit 1; }
command -v npm >/dev/null || die "npm required"
command -v tar >/dev/null || die "tar required"

TMP="$(mktemp -d)"
cleanup() { rm -rf "$TMP"; }
trap cleanup EXIT

echo "Packing three@${THREE_VER}…"
(
  cd "$TMP"
  npm pack "three@${THREE_VER}" --silent
  tar -xzf "three-${THREE_VER}.tgz"
)

THREE_SRC="$TMP/package"
[[ -f "$THREE_SRC/build/three.module.js" ]] || die "three pack missing build/three.module.js"

echo "Packing esp-web-tools@${EWT_VER}…"
(
  cd "$TMP"
  # Fresh extract dir so three's package/ is not mixed in.
  mkdir -p ewt && cd ewt
  npm pack "esp-web-tools@${EWT_VER}" --silent
  tar -xzf "esp-web-tools-${EWT_VER}.tgz"
)

EWT_SRC="$TMP/ewt/package"
[[ -f "$EWT_SRC/dist/web/install-button.js" ]] || die "esp-web-tools pack missing dist/web/install-button.js"

# Minimal three.js addon closure used by Print / Assembly / Flash.
THREE_ADDONS=(
  examples/jsm/loaders/STLLoader.js
  examples/jsm/loaders/GLTFLoader.js
  examples/jsm/controls/OrbitControls.js
  examples/jsm/utils/BufferGeometryUtils.js
  examples/jsm/utils/SkeletonUtils.js
)

echo "Writing ${THREE_DST}…"
rm -rf "$THREE_DST"
mkdir -p "$THREE_DST/build"
cp "$THREE_SRC/build/three.module.js" "$THREE_DST/build/three.module.js"
for rel in "${THREE_ADDONS[@]}"; do
  src="$THREE_SRC/$rel"
  [[ -f "$src" ]] || die "missing three addon $rel"
  mkdir -p "$THREE_DST/$(dirname "$rel")"
  cp "$src" "$THREE_DST/$rel"
done
if [[ -f "$THREE_SRC/LICENSE" ]]; then
  cp "$THREE_SRC/LICENSE" "$THREE_DST/LICENSE"
elif [[ -f "$THREE_SRC/LICENSE.md" ]]; then
  cp "$THREE_SRC/LICENSE.md" "$THREE_DST/LICENSE"
else
  die "three pack missing LICENSE"
fi

echo "Writing ${EWT_DST}…"
rm -rf "$EWT_DST"
mkdir -p "$EWT_DST"
cp -R "$EWT_SRC/dist/web/." "$EWT_DST/"
if [[ -f "$EWT_SRC/LICENSE" ]]; then
  cp "$EWT_SRC/LICENSE" "$EWT_DST/LICENSE"
elif [[ -f "$EWT_SRC/LICENSE.md" ]]; then
  cp "$EWT_SRC/LICENSE.md" "$EWT_DST/LICENSE"
else
  # Package may only declare license in package.json
  {
    echo "esp-web-tools ${EWT_VER}"
    echo "License: see package.json (typically Apache-2.0)"
    python3 -c "import json; print(json.load(open('$EWT_SRC/package.json')).get('license',''))"
  } >"$EWT_DST/LICENSE"
fi

mkdir -p "$VENDOR"
{
  echo "three=${THREE_VER}"
  echo "esp-web-tools=${EWT_VER}"
} >"$VENDOR/VERSIONS.txt"

cat >"$VENDOR/README.md" <<EOF
# Vendored browser libraries

Pinned copies served by GitHub Pages (no CDN).

| Package | Version | Path |
| --- | --- | --- |
| three | ${THREE_VER} | \`three/\` |
| esp-web-tools | ${EWT_VER} | \`esp-web-tools/\` |

Regenerate from the repo root:

\`\`\`bash
./scripts/web/vendor-libs.sh
\`\`\`

Override pins with \`THREE_VER\` / \`EWT_VER\` if needed, then update HTML import maps only when the three major/API changes.
EOF

echo "Done:"
cat "$VENDOR/VERSIONS.txt"
du -sh "$THREE_DST" "$EWT_DST"
