#!/usr/bin/env bash
# Append Cursor hook events to project-root cursor_events.log.
# Separate from anim.sh — logging only.
# Line format: <ISO datetime> <event_name> <event_data_json>

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
LOG="$ROOT/cursor_events.log"
EVENT_NAME="${1:-unknown}"
INPUT="$(cat)"
TS="$(date -Iseconds 2>/dev/null || date '+%Y-%m-%dT%H:%M:%S%z')"
DATA="$(printf '%s' "$INPUT" | tr '\n\r' '  ')"

printf '%s %s %s\n' "$TS" "$EVENT_NAME" "$DATA" >>"$LOG"
exit 0
