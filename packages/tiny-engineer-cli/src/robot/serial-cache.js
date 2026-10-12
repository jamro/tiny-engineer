import { mkdirSync, readFileSync, unlinkSync, writeFileSync } from "node:fs";
import { homedir } from "node:os";
import { dirname, join } from "node:path";

/** @type {string | null | undefined} */
let memoryPath;

/** @type {string | undefined} */
let cacheFileOverride;

/**
 * Override cache file path (tests). Pass undefined to restore default.
 * @param {string | undefined} filePath
 */
export function setSerialCacheFileForTests(filePath) {
  cacheFileOverride = filePath;
  memoryPath = undefined;
}

/**
 * @returns {string}
 */
export function defaultSerialCacheFile() {
  return join(homedir(), ".cache", "tiny-engineer", "serial-port.json");
}

/**
 * @returns {string}
 */
function cacheFile() {
  return cacheFileOverride ?? defaultSerialCacheFile();
}

/**
 * @returns {string | null}
 */
export function readSerialCache() {
  if (memoryPath !== undefined) {
    return memoryPath;
  }

  try {
    const raw = readFileSync(cacheFile(), "utf8");
    const json = JSON.parse(raw);
    if (typeof json?.path === "string" && json.path.trim()) {
      memoryPath = json.path.trim();
      return memoryPath;
    }
  } catch {
    // missing / invalid
  }

  memoryPath = null;
  return null;
}

/**
 * @param {string} path
 */
export function writeSerialCache(path) {
  const trimmed = path.trim();
  memoryPath = trimmed;
  const file = cacheFile();
  try {
    mkdirSync(dirname(file), { recursive: true });
    writeFileSync(
      file,
      `${JSON.stringify({ path: trimmed, updatedAt: Date.now() })}\n`,
      "utf8",
    );
  } catch {
    // best-effort; memory still holds path for this process
  }
}

export function clearSerialCache() {
  memoryPath = null;
  try {
    unlinkSync(cacheFile());
  } catch {
    // already gone
  }
}
