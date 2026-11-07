#!/usr/bin/env bash
# Source after extracting KiCad AppImage into KICAD_APPDIR (Pages/CI).
#
# Lite AppImage ships pcbnew (+ wx) under bundled Python site-packages.
# Ubuntu 24.04 system python is 3.12 — prefer the AppImage interpreter (3.11).
#
# Usage:
#   source scripts/fab/prepare_kicad_appdir_env.sh
# Exports: KICAD_CLI, KICAD_PYTHON, LD_LIBRARY_PATH, PYTHONPATH
#
# shellcheck disable=SC1091

if [[ -z "${KICAD_APPDIR:-}" || ! -d "${KICAD_APPDIR}" ]]; then
  echo "error: KICAD_APPDIR must point at extracted AppImage root" >&2
  return 1 2>/dev/null || exit 1
fi

_lib_candidates=(
  "$KICAD_APPDIR/usr/lib"
  "$KICAD_APPDIR/usr/lib/x86_64-linux-gnu"
  "$KICAD_APPDIR/shared/lib"
  "$KICAD_APPDIR/shared/lib/tmp/AppDir/usr/lib/x86_64-linux-gnu"
)
_ld_parts=()
for _d in "${_lib_candidates[@]}"; do
  [[ -d "$_d" ]] && _ld_parts+=("$_d")
done
# Extra shared libs next to pcbnew
while IFS= read -r _d; do
  [[ -n "$_d" ]] && _ld_parts+=("$_d")
done < <(find "$KICAD_APPDIR" -type d \( -name 'x86_64-linux-gnu' -o -name 'lib' \) 2>/dev/null | head -n 20 || true)

if [[ ${#_ld_parts[@]} -gt 0 ]]; then
  _joined="$(IFS=:; echo "${_ld_parts[*]}")"
  export LD_LIBRARY_PATH="${_joined}${LD_LIBRARY_PATH:+:}${LD_LIBRARY_PATH:-}"
fi

_py_path_parts=()
while IFS= read -r _d; do
  [[ -n "$_d" ]] && _py_path_parts+=("$_d")
done < <(find "$KICAD_APPDIR" -type d \( -name 'dist-packages' -o -name 'site-packages' \) 2>/dev/null | head -n 20 || true)

if [[ ${#_py_path_parts[@]} -gt 0 ]]; then
  _joined="$(IFS=:; echo "${_py_path_parts[*]}")"
  export PYTHONPATH="${_joined}${PYTHONPATH:+:}${PYTHONPATH:-}"
fi

_cli=""
for _c in "$KICAD_APPDIR/usr/bin/kicad-cli" "$KICAD_APPDIR/bin/kicad-cli"; do
  if [[ -x "$_c" ]]; then
    _cli="$_c"
    break
  fi
done
if [[ -z "$_cli" ]]; then
  echo "error: kicad-cli not under $KICAD_APPDIR" >&2
  return 1 2>/dev/null || exit 1
fi
export KICAD_CLI="$_cli"

_py_candidates=()
[[ -n "${KICAD_PYTHON:-}" ]] && _py_candidates+=("$KICAD_PYTHON")
while IFS= read -r _p; do
  [[ -n "$_p" ]] && _py_candidates+=("$_p")
done < <(find "$KICAD_APPDIR" \( -type f -o -type l \) -name 'python3*' 2>/dev/null | head -n 30 || true)

_py=""
for _c in "${_py_candidates[@]}"; do
  [[ -x "$_c" ]] || continue
  if "$_c" -c 'import pcbnew' >/dev/null 2>&1; then
    _py="$_c"
    break
  fi
done

if [[ -z "$_py" ]]; then
  echo "error: no Python with importable pcbnew under $KICAD_APPDIR" >&2
  echo "PYTHONPATH=${PYTHONPATH:-}" >&2
  echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-}" >&2
  find "$KICAD_APPDIR" -name 'pcbnew*' 2>/dev/null | head -n 20 >&2 || true
  return 1 2>/dev/null || exit 1
fi
export KICAD_PYTHON="$_py"

echo "KICAD_CLI=$KICAD_CLI"
echo "KICAD_PYTHON=$KICAD_PYTHON"
"$KICAD_CLI" version
"$KICAD_PYTHON" -c 'import pcbnew; print("pcbnew", pcbnew.Version())'
if "$KICAD_PYTHON" -c 'import wx; print("wx", wx.version())' 2>/dev/null; then
  :
else
  echo "warn: wx missing in AppImage Python — export will use scripts/fab wx stub"
fi
