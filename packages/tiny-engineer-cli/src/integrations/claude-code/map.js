export const READING_TOOLS = new Set([
  "Read",
  "Grep",
  "Glob",
  "WebFetch",
  "WebSearch",
]);

export const TYPING_TOOLS = new Set([
  "Bash",
  "PowerShell",
  "Edit",
  "Write",
  "NotebookEdit",
]);

const EVENT_ANIM = new Map([
  ["UserPromptSubmit", "reading"],
  ["SubagentStart", "thinking"],
  ["PostToolBatch", "thinking"],
  ["PostToolUseFailure", "error"],
  ["PreCompact", "thinking"],
  ["PermissionRequest", "attention"],
  ["PermissionDenied", "abort"],
  ["Notification", "attention"],
  ["Stop", "ring"],
  ["StopFailure", "error"],
]);

/**
 * Map a Claude Code hook payload to an animation name, or null to skip.
 * @param {{ hook_event_name?: string, tool_name?: string, source?: string, reason?: string }} event
 * @returns {string | null}
 */
export function animationForEvent(event) {
  const name = event?.hook_event_name;
  if (!name) return null;

  if (name === "PreToolUse") {
    const tool = event.tool_name;
    if (READING_TOOLS.has(tool)) return "reading";
    if (TYPING_TOOLS.has(tool)) return "typing";
    return null;
  }

  // Only a fresh launch welcomes; resume, /clear, compaction, fork, or an
  // unknown source wakes up, so one session never plays welcome twice.
  if (name === "SessionStart") {
    return event.source === "startup" ? "welcome" : "wakeup";
  }

  // /clear and /resume end one session and start the next at once. The
  // SessionStart that follows owns the pose; a sleep posted alongside it makes
  // the robot drop the wakeup, so skip it.
  if (name === "SessionEnd") {
    return event.reason === "clear" || event.reason === "resume" ? null : "sleep";
  }

  return EVENT_ANIM.get(name) ?? null;
}
