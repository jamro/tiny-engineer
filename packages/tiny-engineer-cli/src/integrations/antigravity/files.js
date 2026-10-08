import { createStyle } from "../../util/style.js";
import { TYPING_TOOLS } from "./map.js";

const PATH_KEYS = ["path", "file_path", "TargetFile", "target_file", "filePath"];

/**
 * @param {Record<string, unknown>} toolCall
 * @returns {string | null}
 */
function pathFromToolCall(toolCall) {
  for (const key of PATH_KEYS) {
    if (typeof toolCall[key] === "string" && toolCall[key]) {
      return /** @type {string} */ (toolCall[key]);
    }
  }
  const args = toolCall.args ?? toolCall.arguments;
  if (args && typeof args === "object") {
    const record = /** @type {Record<string, unknown>} */ (args);
    for (const key of PATH_KEYS) {
      if (typeof record[key] === "string" && record[key]) {
        return /** @type {string} */ (record[key]);
      }
    }
  }
  return null;
}

/**
 * @param {string} hookType
 * @param {Record<string, unknown> | null | undefined} payload
 * @param {import("../../util/style.js").Style} [style]
 * @returns {string[]}
 */
export function formatFileModifications(
  hookType,
  payload,
  style = createStyle({ color: false }),
) {
  if (hookType !== "PreToolUse" || !payload || typeof payload !== "object") {
    return [];
  }

  const toolCall = payload.toolCall;
  if (!toolCall || typeof toolCall !== "object") return [];

  const call = /** @type {Record<string, unknown>} */ (toolCall);
  const name = typeof call.name === "string" ? call.name : "";
  if (!TYPING_TOOLS.has(name)) return [];

  const filePath = pathFromToolCall(call);
  if (!filePath) return [];
  return [`${style.cyan("✎")}  touching: ${filePath}`];
}
