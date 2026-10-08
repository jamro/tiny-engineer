import { formatFileModifications } from "./files.js";
import {
  applyInstall,
  configRelativePath,
  detect,
  planInstall,
} from "./install.js";
import { animationForEvent } from "./map.js";

/**
 * @param {string} stdinText
 * @returns {Record<string, unknown> | null}
 */
function parseStdinJson(stdinText) {
  if (!stdinText.trim()) return null;
  try {
    return JSON.parse(stdinText);
  } catch {
    return null;
  }
}

/** @type {import("../types.js").Integration} */
export const claudeCode = {
  id: "claude-code",
  name: "Claude Code",
  hardExitAfterPost: true,

  /**
   * @param {string[]} _argv
   * @param {string} stdinText
   */
  parseHook(_argv, stdinText) {
    const event = parseStdinJson(stdinText);
    if (!event || typeof event !== "object") return null;

    const name = event.hook_event_name;
    if (typeof name !== "string" || !name) return null;

    return {
      event: name,
      tool: typeof event.tool_name === "string" ? event.tool_name : undefined,
      status: typeof event.status === "string" ? event.status : undefined,
      meta: {
        source: event.source,
        reason: event.reason,
        raw: event,
      },
    };
  },

  /**
   * @param {import("../types.js").HookInput} input
   */
  mapToAnim(input) {
    const raw = input.meta?.raw;
    if (raw && typeof raw === "object") {
      return animationForEvent(
        /** @type {Parameters<typeof animationForEvent>[0]} */ (raw),
      );
    }
    return animationForEvent({
      hook_event_name: input.event,
      tool_name: input.tool,
      source: typeof input.meta?.source === "string" ? input.meta.source : undefined,
      reason: typeof input.meta?.reason === "string" ? input.meta.reason : undefined,
    });
  },

  /**
   * @param {import("../types.js").HookInput} input
   * @param {import("../../util/style.js").Style} style
   */
  describeFiles(input, style) {
    const raw = input.meta?.raw;
    if (!raw || typeof raw !== "object") return [];
    return formatFileModifications(/** @type {Record<string, unknown>} */ (raw), style);
  },

  // Claude Code adds SessionStart / UserPromptSubmit stdout to the model context.
  respond() {},

  detect,
  configRelativePath,
  planInstall,
  applyInstall,
};
