import { createStyle } from "../../util/style.js";

const PATH_WRITE_TOOLS = new Set(["Edit", "Write", "NotebookEdit"]);

/**
 * @param {Record<string, unknown> | null | undefined} raw
 * @param {import("../../util/style.js").Style} [style]
 * @returns {string[]}
 */
export function formatFileModifications(raw, style = createStyle({ color: false })) {
  if (!raw || typeof raw !== "object") return [];
  if (raw.hook_event_name !== "PreToolUse") return [];
  if (typeof raw.tool_name !== "string" || !PATH_WRITE_TOOLS.has(raw.tool_name)) {
    return [];
  }

  const toolInput = raw.tool_input;
  let filePath = null;
  if (typeof raw.file_path === "string") {
    filePath = raw.file_path;
  } else if (toolInput && typeof toolInput === "object") {
    const input = /** @type {Record<string, unknown>} */ (toolInput);
    if (typeof input.file_path === "string") filePath = input.file_path;
    else if (typeof input.path === "string") filePath = input.path;
  }

  if (!filePath) return [];
  return [`${style.cyan("✎")}  touching: ${filePath}`];
}
