#!/usr/bin/env bash
# Fire-and-forget robot animation via HTTP. Does not block the agent.
cat >/dev/null
curl -4 -sS -m 2 -X POST "http://tiny-engineer.local/anim?name=${1}" >/dev/null 2>&1 &
exit 0
