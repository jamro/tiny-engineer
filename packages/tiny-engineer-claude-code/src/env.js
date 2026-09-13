import { readFileSync } from "node:fs";
import { join } from "node:path";

const TOKEN_KEY = "TINY_ENGINEER_TOKEN";
const URL_KEY = "TINY_ENGINEER_URL";
export const DEFAULT_URL = "http://tiny-engineer.local";

/**
 * Parse one dotenv line into [key, value] or null.
 * @param {string} line
 * @returns {[string, string] | null}
 */
function parseLine(line) {
  const trimmed = line.trim();
  if (!trimmed || trimmed.startsWith("#")) return null;

  const eq = trimmed.indexOf("=");
  if (eq <= 0) return null;

  const key = trimmed.slice(0, eq).trim();
  if (!/^[A-Za-z_][A-Za-z0-9_]*$/.test(key)) return null;

  let value = trimmed.slice(eq + 1).trim();
  if (
    (value.startsWith('"') && value.endsWith('"')) ||
    (value.startsWith("'") && value.endsWith("'"))
  ) {
    value = value.slice(1, -1);
  }
  return [key, value];
}

/**
 * Load `<dir>/.env` into process.env for keys not already set.
 * Claude Code runs hooks from the session's working directory, which can be a
 * subdirectory, so prefer the project root it exports.
 * Missing file / read errors are ignored.
 * @param {string} [dir]
 */
export function loadDotEnv(dir = process.env.CLAUDE_PROJECT_DIR || process.cwd()) {
  let raw;
  try {
    raw = readFileSync(join(dir, ".env"), "utf8");
  } catch {
    return;
  }

  for (const line of raw.split(/\r?\n/)) {
    const parsed = parseLine(line);
    if (!parsed) continue;
    const [key, value] = parsed;
    if (process.env[key] === undefined) {
      process.env[key] = value;
    }
  }
}

/**
 * @returns {string | null} trimmed TINY_ENGINEER_TOKEN, or null if unset/empty
 */
export function getToken() {
  const value = process.env[TOKEN_KEY];
  if (value === undefined) return null;
  const trimmed = value.trim();
  return trimmed ? trimmed : null;
}

/**
 * @returns {string} TINY_ENGINEER_URL without trailing slashes, or the default
 */
export function getBaseUrl() {
  const value = process.env[URL_KEY];
  if (value === undefined) return DEFAULT_URL;
  const trimmed = value.trim().replace(/\/+$/, "");
  return trimmed || DEFAULT_URL;
}
