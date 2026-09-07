export const READING_TOOLS = new Set([
  "view_file",
  "grep_search",
  "find_by_name",
  "list_dir",
  "read_url_content",
  "read_resource",
  "list_resources",
]);

export const TYPING_TOOLS = new Set([
  "write_to_file",
  "replace_file_content",
  "run_command",
  "generate_image",
  "call_mcp_tool",
]);

export const THINKING_TOOLS = new Set([
  "ask_question",
  "invoke_subagent",
  "define_subagent",
  "manage_subagents",
  "manage_task",
  "schedule",
]);

/**
 * Map an Antigravity lifecycle event + payload to a robot animation name.
 * @param {string} hookType e.g. "PreInvocation", "PreToolUse", "PostToolUse", "Stop"
 * @param {Record<string, any>} payload Stdin JSON payload from Antigravity
 * @returns {string | null}
 */
export function animationForEvent(hookType, payload = {}) {
  switch (hookType) {
    case "PreInvocation":
      return "thinking";

    case "PreToolUse": {
      const toolName = payload?.toolCall?.name;
      if (!toolName) return "thinking";
      if (READING_TOOLS.has(toolName)) return "reading";
      if (TYPING_TOOLS.has(toolName)) return "typing";
      if (THINKING_TOOLS.has(toolName)) return "thinking";
      return "thinking";
    }

    case "PostToolUse": {
      if (payload?.error) {
        return "attention";
      }
      return null;
    }

    case "Stop": {
      const reason = payload?.terminationReason;
      if (payload?.error || reason === "error") {
        return "error";
      }
      if (reason === "aborted" || reason === "user_cancelled") {
        return "abort";
      }
      // Finished turn normally (model_stop / max_steps_exceeded / idle) -> ring the desk bell!
      return "ring";
    }

    default:
      return null;
  }
}
