/** Tools that map to the typing animation (from former hooks.json matcher). */
export const TYPING_TOOLS = new Set([
  "Write",
  "StrReplace",
  "EditNotebook",
  "Delete",
  "Shell",
  "Task",
  "CallMcpTool",
  "TodoWrite",
  "GenerateImage",
]);

export const READING_TOOLS = new Set([
  "Read",
  "Grep",
  "Glob",
  "WebSearch",
  "WebFetch",
  "AwaitShell",
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
};

/**
 * Map a Cursor hook payload to an animation name, or null to skip.
 * Empty conversation_id or generation_id skips the pose (missing IDs still map).
 * @param {{ hook_event_name?: string, tool_name?: string, status?: string, conversation_id?: string, generation_id?: string }} event
 * @returns {string | null}
 */
export function animationForEvent(event) {
  if (event?.conversation_id === "" || event?.generation_id === "") return null;

  const name = event?.hook_event_name;
  if (!name) return null;

  if (name === "preToolUse") {
    const tool = event.tool_name;
    if (READING_TOOLS.has(tool)) return "reading";
    if (TYPING_TOOLS.has(tool)) return "typing";
    return null;
  }

  if (name === "stop") {
    if (event.status === "aborted") return "abort";
    if (event.status === "error") return "error";
    return "ring";
  }

  return EVENT_ANIM[name] ?? null;
}
