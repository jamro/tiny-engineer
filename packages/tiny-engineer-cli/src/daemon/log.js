import { closeSync, ftruncateSync, openSync, statSync, appendFileSync, mkdirSync } from "node:fs";
import { dirname } from "node:path";

export const LOG_MAX_BYTES = 1024 * 1024;

/**
 * Append one line. If the file is already at or over maxBytes, truncate then write.
 * @param {string} path
 * @param {string} line
 * @param {number} [maxBytes]
 */
export function appendLog(path, line, maxBytes = LOG_MAX_BYTES) {
  mkdirSync(dirname(path), { recursive: true, mode: 0o700 });
  const text = line.endsWith("\n") ? line : `${line}\n`;
  let size = 0;
  try {
    size = statSync(path).size;
  } catch {
    size = 0;
  }
  if (size >= maxBytes) {
    const fd = openSync(path, "r+");
    try {
      ftruncateSync(fd, 0);
    } finally {
      closeSync(fd);
    }
  }
  appendFileSync(path, text, { mode: 0o600 });
}
