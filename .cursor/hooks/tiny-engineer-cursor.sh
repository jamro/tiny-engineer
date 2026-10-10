#!/usr/bin/env bash
# Resolve node (Cursor hooks often lack nvm on PATH), then run local CLI.
# Stdin is the Cursor hook JSON — must stay intact for `hook cursor`.

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
CLI="$ROOT/packages/tiny-engineer-cli/bin/tiny-engineer.js"

if ! command -v node >/dev/null 2>&1; then
  export NVM_DIR="${NVM_DIR:-$HOME/.nvm}"
  if [[ -s "$NVM_DIR/nvm.sh" ]]; then
    # shellcheck disable=SC1090
    . "$NVM_DIR/nvm.sh"
  fi
fi

if command -v node >/dev/null 2>&1; then
  exec node "$CLI" hook cursor
fi

for candidate in /opt/homebrew/bin/node /usr/local/bin/node "$HOME/.volta/bin/node"; do
  if [[ -x "$candidate" ]]; then
    exec "$candidate" "$CLI" hook cursor
  fi
done

echo "tiny-engineer: node not found (Cursor hooks have no nvm PATH)" >&2
exit 127
