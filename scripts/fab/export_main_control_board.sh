#!/usr/bin/env bash
# Export main-control-board JLCPCB package into web/shop/fab/main-control-board/.
#
# Primary (required for Pages): Fabrication Toolkit via KiCad Python (AUTO TRANSLATE).
# Output matches hardware/boards/main-control-board/production/ from the Toolkit GUI/CLI.
#
# Optional local fallback: FAB_FORCE_KICAD_CLI=1 (NOT JLCPCB-equivalent for connector CPL).
#
# Usage (repo root):
#   ./scripts/fab/export_main_control_board.sh
#
# Env (optional):
#   KICAD_PYTHON / KICAD_CLI / KICAD_APPDIR
#   FAB_TOOLKIT_ROOT / FAB_TOOLKIT_VERSION (default 5.3.1)
#   FAB_OUT_DIR (default web/shop/fab/main-control-board)
#   FAB_REQUIRE_TOOLKIT=1  fail if Toolkit path unavailable (Pages sets this)
#   FAB_FORCE_KICAD_CLI=1  skip Toolkit (dev only)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

BOARD_DIR="$ROOT/hardware/boards/main-control-board"
PCB="$BOARD_DIR/main-control-board.kicad_pcb"
OUT_DIR="${FAB_OUT_DIR:-$ROOT/web/shop/fab/main-control-board}"
TOOLKIT_VERSION="${FAB_TOOLKIT_VERSION:-5.3.1}"
CACHE_DIR="${FAB_CACHE_DIR:-$ROOT/artifacts/fab-tools}"

die() { echo "error: $*" >&2; exit 1; }

[[ -f "$PCB" ]] || die "missing $PCB"

# When CI extracted an AppImage, wire pcbnew/wx into KICAD_PYTHON.
if [[ -n "${KICAD_APPDIR:-}" && -d "${KICAD_APPDIR}" ]]; then
  # shellcheck source=/dev/null
  source "$ROOT/scripts/fab/prepare_kicad_appdir_env.sh"
fi

find_kicad_python() {
  if [[ -n "${KICAD_PYTHON:-}" && -x "$KICAD_PYTHON" ]]; then
    if "$KICAD_PYTHON" -c 'import pcbnew' >/dev/null 2>&1; then
      echo "$KICAD_PYTHON"
      return
    fi
  fi
  local candidates=(
    "/Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python3"
    "/Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/3.9/bin/python3"
  )
  if [[ -n "${KICAD_APPDIR:-}" ]]; then
    local p
    while IFS= read -r p; do
      candidates+=("$p")
    done < <(find "$KICAD_APPDIR" \( -type f -o -type l \) -name 'python3*' 2>/dev/null | head -n 20 || true)
  fi
  local c
  for c in "${candidates[@]}"; do
    [[ -x "$c" ]] || continue
    if "$c" -c 'import pcbnew' >/dev/null 2>&1; then
      echo "$c"
      return
    fi
  done
  return 1
}

find_kicad_cli() {
  if [[ -n "${KICAD_CLI:-}" && -x "$KICAD_CLI" ]]; then
    echo "$KICAD_CLI"
    return
  fi
  if command -v kicad-cli >/dev/null 2>&1; then
    command -v kicad-cli
    return
  fi
  local c
  for c in \
    "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli" \
    "${KICAD_APPDIR:-}/usr/bin/kicad-cli" \
    "${KICAD_APPDIR:-}/bin/kicad-cli"
  do
    [[ -x "$c" ]] && { echo "$c"; return; }
  done
  return 1
}

shim_flat_toolkit() {
  local flat="$1"
  local shim="$CACHE_DIR/toolkit-shim"
  mkdir -p "$CACHE_DIR"
  rm -rf "$shim"
  mkdir -p "$shim"
  ln -s "$flat" "$shim/plugins"
  echo "$shim"
}

ensure_toolkit_root() {
  local candidate="${FAB_TOOLKIT_ROOT:-}"
  if [[ -z "$candidate" ]]; then
    local pcm
    for pcm in \
      "$HOME/Documents/KiCad/10.0/3rdparty/plugins/com_github_bennymeg_JLC-Plugin-for-KiCad" \
      "$HOME/Documents/KiCad/9.0/3rdparty/plugins/com_github_bennymeg_JLC-Plugin-for-KiCad"
    do
      if [[ -f "$pcm/cli.py" ]]; then
        candidate="$pcm"
        break
      fi
    done
  fi

  if [[ -n "$candidate" ]]; then
    if [[ -f "$candidate/plugins/cli.py" ]]; then
      echo "$candidate"
      return
    fi
    if [[ -f "$candidate/cli.py" && -f "$candidate/thread.py" ]]; then
      shim_flat_toolkit "$candidate"
      return
    fi
    die "FAB_TOOLKIT_ROOT=$candidate missing plugins/cli.py (or flat cli.py)"
  fi

  local extract="$CACHE_DIR/Fabrication-Toolkit-${TOOLKIT_VERSION}"
  if [[ ! -f "$extract/plugins/cli.py" ]]; then
    mkdir -p "$CACHE_DIR"
    local tgz="$CACHE_DIR/Fabrication-Toolkit-${TOOLKIT_VERSION}.tar.gz"
    local url="https://github.com/bennymeg/Fabrication-Toolkit/archive/refs/tags/${TOOLKIT_VERSION}.tar.gz"
    echo "Downloading Fabrication Toolkit ${TOOLKIT_VERSION}…" >&2
    curl -fL --retry 5 --retry-delay 5 -o "$tgz" "$url"
    rm -rf "$extract"
    tar -xzf "$tgz" -C "$CACHE_DIR"
    [[ -f "$extract/plugins/cli.py" ]] || die "Toolkit extract missing plugins/cli.py"
  fi
  echo "$extract"
}

write_source_txt() {
  local method="$1"
  local out="$OUT_DIR/SOURCE.txt"
  {
    echo "board: main-control-board"
    echo "method: $method"
    echo "generated_utc: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    if command -v git >/dev/null 2>&1 && git -C "$ROOT" rev-parse HEAD >/dev/null 2>&1; then
      echo "git_sha: $(git -C "$ROOT" rev-parse HEAD)"
    fi
    echo "toolkit_version: ${TOOLKIT_VERSION}"
  } >"$out"
}

publish_from_production() {
  local prod="$BOARD_DIR/production"
  mkdir -p "$OUT_DIR"
  # Explicit status checks: callers may invoke us under `if`/`elif`, where set -e
  # is disabled for the function body.
  cp "$prod/bom.csv" "$OUT_DIR/bom.csv" || return 1
  cp "$prod/positions.csv" "$OUT_DIR/positions.csv" || return 1
  local zip
  # maxdepth 1 already skips subdirs; do not filter on full path (cwd may contain "backup").
  zip="$(find "$prod" -maxdepth 1 -type f -name '*.zip' ! -iname '*backup*' | head -n 1)"
  [[ -n "$zip" ]] || die "no Gerber zip in $prod"
  cp "$zip" "$OUT_DIR/gerbers.zip" || return 1
}

verify_out() {
  local f
  for f in gerbers.zip bom.csv positions.csv; do
    [[ -s "$OUT_DIR/$f" ]] || die "missing or empty $OUT_DIR/$f"
  done
  head -n 1 "$OUT_DIR/bom.csv" | grep -qi 'Designator' || die "bom.csv missing Designator header"
  head -n 1 "$OUT_DIR/bom.csv" | grep -qi 'LCSC' || die "bom.csv missing LCSC column"
  head -n 1 "$OUT_DIR/positions.csv" | grep -qi 'Designator' || die "positions.csv missing Designator header"
  echo "Published $OUT_DIR:"
  ls -la "$OUT_DIR"
}

export_via_toolkit() {
  local py toolkit
  py="$(find_kicad_python)" || return 1
  # wx optional: run_fabrication_toolkit.py stubs it if missing.
  # Explicit || return 1: this function is called from `elif export_via_toolkit`,
  # which disables set -e for the function body.
  toolkit="$(ensure_toolkit_root)" || return 1
  echo "Using Toolkit via $py"
  echo "Toolkit root: $toolkit"
  rm -rf "$BOARD_DIR/production"
  FAB_TOOLKIT_ROOT="$toolkit" "$py" "$ROOT/scripts/fab/run_fabrication_toolkit.py" --board "$PCB" --toolkit-root "$toolkit" || return 1
  publish_from_production || return 1
  write_source_txt "fabrication-toolkit" || return 1
}

export_via_cli() {
  local cli
  cli="$(find_kicad_cli)" || die "kicad-cli not found (set KICAD_CLI)"
  echo "Using kicad-cli fallback: $cli" >&2
  echo "WARNING: CPL will NOT match Fabrication Toolkit (connector pad-center / AUTO TRANSLATE)." >&2
  mkdir -p "$OUT_DIR"
  python3 "$ROOT/scripts/fab/export_kicad_cli_fallback.py" \
    --board-dir "$BOARD_DIR" \
    --out-dir "$OUT_DIR" \
    --kicad-cli "$cli"
  write_source_txt "kicad-cli-fallback"
}

mkdir -p "$OUT_DIR"

if [[ "${FAB_FORCE_KICAD_CLI:-}" == "1" ]]; then
  export_via_cli
elif export_via_toolkit; then
  :
elif [[ "${FAB_REQUIRE_TOOLKIT:-}" == "1" ]]; then
  die "Fabrication Toolkit export required (FAB_REQUIRE_TOOLKIT=1) but unavailable — fix KICAD_PYTHON/pcbnew/wx"
else
  echo "Toolkit path unavailable — falling back to kicad-cli (dev only)…" >&2
  export_via_cli
fi

verify_out
