#!/usr/bin/env bash
# Drive Tiny Engineer poses from Claude Code hook events.
# Usage: pose.sh <anim-name> <event-name>
#
# Two rules matter here:
#   1. Never block the agent. The POST is detached, so a powered-off robot
#      costs ~0ms instead of stalling every tool call on a connect timeout.
#   2. Never fail. Always exit 0 -- a hook error must not break the session.

POSE="${1:-none}"
EVENT="${2:-unknown}"
URL="${TINY_ENGINEER_URL:-http://tiny-engineer.local}"

# Drain stdin (Claude Code sends the event JSON there); ignore content.
cat >/dev/null 2>&1

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
LOG="$ROOT/.claude/tiny-engineer-hooks.log"
TS="$(date -Iseconds 2>/dev/null || date '+%Y-%m-%dT%H:%M:%S%z')"
printf '%s %s -> %s\n' "$TS" "$EVENT" "$POSE" >>"$LOG" 2>/dev/null

# Detached and silent. -4 avoids an IPv6-first answer costing a round trip.
(
  curl -4 -sS --connect-timeout 1 -m 2 -X POST \
    ${TINY_ENGINEER_TOKEN:+-H "Authorization: Bearer $TINY_ENGINEER_TOKEN"} \
    "$URL/anim?name=$POSE" >/dev/null 2>&1 &
) >/dev/null 2>&1

exit 0
