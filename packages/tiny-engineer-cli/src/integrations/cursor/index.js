import { formatFileModifications } from "./files.js";
import {
  applyInstall,
  applyUninstall,
  configRelativePath,
  detect,
  planInstall,
  planUninstall,
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
export const cursor = {
  id: "cursor",
  name: "Cursor",

  /**
   * Cursor pipes JSON with hook_event_name on stdin; argv is unused.
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
        conversation_id: event.conversation_id,
        generation_id: event.generation_id,
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
      return animationForEvent(/** @type {Parameters<typeof animationForEvent>[0]} */ (raw));
    }
    return animationForEvent({
      hook_event_name: input.event,
      tool_name: input.tool,
      status: input.status,
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

  // Cursor has no stdout contract; stay silent.
  respond() {},

  detect,
  configRelativePath,
  planInstall,
  applyInstall,
  planUninstall,
  applyUninstall,
};
