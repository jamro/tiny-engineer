/** Tools that map to the typing animation (from former hooks.json matcher). */
export const TYPING_TOOLS = new Set([
  "Write",
  "StrReplace",
  "EditNotebook",
  "Delete",
  "Shell",
  "Task",
  "Grep",
  "Glob",
  "WebSearch",
  "WebFetch",
  "CallMcpTool",
  "TodoWrite",
  "AwaitShell",
  "GenerateImage",
]);

const EVENT_ANIM = {
  sessionStart: "reading",
  beforeSubmitPrompt: "reading",
  beforeReadFile: "reading",
  afterAgentThought: "thinking",
  preCompact: "thinking",
  beforeShellExecution: "typing",
  subagentStart: "typing",
  afterFileEdit: "typing",
  stop: "ring",
};

/**
 * Map a Cursor hook payload to an animation name, or null to skip.
 * @param {{ hook_event_name?: string, tool_name?: string }} event
 * @returns {string | null}
 */
export function animationForEvent(event) {
  const name = event?.hook_event_name;
  if (!name) return null;

  if (name === "preToolUse") {
    const tool = event.tool_name;
    if (tool === "Read") return "reading";
    if (TYPING_TOOLS.has(tool)) return "typing";
    return null;
  }

  return EVENT_ANIM[name] ?? null;
}
