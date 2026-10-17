import { existsSync, mkdirSync, readFileSync, unlinkSync, writeFileSync } from "node:fs";
import { homedir } from "node:os";
import { dirname, join } from "node:path";

/**
 * @returns {string}
 */
export function defaultCachePath() {
  return join(homedir(), ".config", "tiny-engineer", "serial-port.json");
}

/**
 * @param {string} [cachePath]
 * @returns {string | null}
 */
export function readPortCache(cachePath = defaultCachePath()) {
  if (!existsSync(cachePath)) return null;
  try {
    const raw = readFileSync(cachePath, "utf8");
    const parsed = JSON.parse(raw);
    if (typeof parsed?.port === "string" && parsed.port.trim()) {
      return parsed.port.trim();
    }
  } catch {
    // ignore corrupt cache
  }
  return null;
}

/**
 * @param {string} port
 * @param {string} [cachePath]
 */
export function writePortCache(port, cachePath = defaultCachePath()) {
  mkdirSync(dirname(cachePath), { recursive: true });
  writeFileSync(cachePath, `${JSON.stringify({ port, updatedAt: Date.now() }, null, 2)}\n`, "utf8");
}

/**
 * @param {string} [cachePath]
 */
export function clearPortCache(cachePath = defaultCachePath()) {
  if (!existsSync(cachePath)) return;
  try {
    unlinkSync(cachePath);
  } catch {
    // ignore
  }
}
