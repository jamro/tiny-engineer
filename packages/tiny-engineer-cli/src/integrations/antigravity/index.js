import { formatFileModifications } from "./files.js";
import {
  applyInstall,
  configRelativePath,
  detect,
  planInstall,
} from "./install.js";
import { animationForEvent, defaultResponseForHook } from "./map.js";

/**
 * @param {string} stdinText
 * @returns {Record<string, unknown>}
 */
function parseStdinJson(stdinText) {
  if (!stdinText.trim()) return {};
  try {
    const parsed = JSON.parse(stdinText);
    return parsed && typeof parsed === "object" ? parsed : {};
  } catch {
    return {};
  }
}

/** @type {import("../types.js").Integration} */
export const antigravity = {
  id: "antigravity",
  name: "Antigravity",

  /**
   * Hook type on argv (`PreToolUse`, `Stop`, …); payload on stdin.
   * Never returns null — Antigravity always needs a decision JSON response.
   * @param {string[]} argv
   * @param {string} stdinText
   */
  parseHook(argv, stdinText) {
    const hookType = argv[0] || "unknown";
    const payload = parseStdinJson(stdinText);
    const toolCall = payload.toolCall;
    const toolName =
      toolCall && typeof toolCall === "object" && typeof toolCall.name === "string"
        ? toolCall.name
        : undefined;

    return {
      event: hookType,
      tool: toolName,
      meta: { raw: payload },
    };
  },

  /**
   * @param {import("../types.js").HookInput} input
   */
  mapToAnim(input) {
    const raw =
      input.meta?.raw && typeof input.meta.raw === "object"
        ? /** @type {Record<string, unknown>} */ (input.meta.raw)
        : {};
    return animationForEvent(input.event, raw);
  },

  /**
   * @param {import("../types.js").HookInput} input
   * @param {import("../../util/style.js").Style} style
   */
  describeFiles(input, style) {
    const raw =
      input.meta?.raw && typeof input.meta.raw === "object"
        ? /** @type {Record<string, unknown>} */ (input.meta.raw)
        : {};
    return formatFileModifications(input.event, raw, style);
  },

  /**
   * Always print Antigravity decision JSON on stdout (not quiet-sensitive).
   * @param {import("../types.js").HookRespondContext} ctx
   */
  respond(ctx) {
    const hookType = ctx.input?.event ?? "unknown";
    console.log(JSON.stringify(defaultResponseForHook(hookType)));
  },

  detect,
  configRelativePath,
  planInstall,
  applyInstall,
};
