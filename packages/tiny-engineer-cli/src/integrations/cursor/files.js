import { createStyle } from "../../util/style.js";

const WRITE_TOOLS = new Set([
  "Write",
  "StrReplace",
  "EditNotebook",
  "Delete",
]);

const TRUNC = 60;

/**
 * @param {unknown} value
 * @returns {string}
 */
function collapse(value) {
  return String(value ?? "")
    .replace(/\r?\n/g, "\\n")
    .replace(/\s+/g, " ")
    .trim();
}

/**
 * @param {string} text
 * @param {number} [max]
 * @returns {string}
 */
function truncate(text, max = TRUNC) {
  if (text.length <= max) return text;
  return `${text.slice(0, max - 1)}…`;
}

/**
 * Format Cursor hook payload file changes for stderr.
 * @param {Record<string, unknown> | null | undefined} raw
 * @param {import("../../util/style.js").Style} [style]
 * @returns {string[]}
 */
export function formatFileModifications(raw, style = createStyle({ color: false })) {
  if (!raw || typeof raw !== "object") return [];

  const event = raw.hook_event_name;
  /** @type {string[]} */
  const lines = [];

  if (event === "afterFileEdit" || event === "afterTabFileEdit") {
    const filePath = typeof raw.file_path === "string" ? raw.file_path : null;
    if (!filePath) return [];

    const edits = Array.isArray(raw.edits) ? raw.edits : [];
    const n = edits.length;
    const label = n === 1 ? "1 edit" : `${n} edits`;
    lines.push(`${style.cyan("✎")}  modified: ${filePath} (${label})`);

    for (const edit of edits) {
      if (!edit || typeof edit !== "object") continue;
      const oldS = truncate(collapse(/** @type {{ old_string?: unknown }} */ (edit).old_string));
      const newS = truncate(collapse(/** @type {{ new_string?: unknown }} */ (edit).new_string));
      lines.push(
        `   ${style.red("−")} ${style.dim(`"${oldS}"`)} → ${style.green("+")} ${style.green(`"${newS}"`)}`,
      );
    }
    return lines;
  }

  if (event === "preToolUse" && typeof raw.tool_name === "string" && WRITE_TOOLS.has(raw.tool_name)) {
    const toolInput = raw.tool_input;
    const filePath =
      toolInput &&
      typeof toolInput === "object" &&
      typeof /** @type {{ file_path?: unknown }} */ (toolInput).file_path === "string"
        ? /** @type {{ file_path: string }} */ (toolInput).file_path
        : null;
    if (filePath) {
      lines.push(`${style.cyan("✎")}  touching: ${filePath}`);
    }
  }

  return lines;
}
