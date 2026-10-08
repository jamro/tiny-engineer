import { readFileSync } from "node:fs";
import { homedir } from "node:os";
import { join, resolve } from "node:path";

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
 * Load one `.env` file into process.env for keys not already set.
 * A token already in the process env must not be sent to a URL from project
 * `.env`, so the file skips `TINY_ENGINEER_URL` when a token was inherited.
 * Missing file / read errors are ignored.
 * @param {string} filePath
 */
export function loadDotEnvFile(filePath) {
  let raw;
  try {
    raw = readFileSync(filePath, "utf8");
  } catch {
    return;
  }

  const tokenInherited = Boolean(process.env[TOKEN_KEY]?.trim());

  for (const line of raw.split(/\r?\n/)) {
    const parsed = parseLine(line);
    if (!parsed) continue;
    const [key, value] = parsed;
    if (key === URL_KEY && tokenInherited) continue;
    if (process.env[key] === undefined) {
      process.env[key] = value;
    }
  }
}

/**
 * Load env from cwd, Claude project dir (if set), and Antigravity global config.
 * @param {string} [cwd]
 */
export function loadDotEnv(cwd = process.cwd()) {
  const loaded = new Set();

  /**
   * @param {string} dir
   */
  function loadDir(dir) {
    const abs = resolve(dir);
    if (loaded.has(abs)) return;
    loaded.add(abs);
    loadDotEnvFile(join(abs, ".env"));
  }

  loadDir(cwd);

  const claudeRoot = process.env.CLAUDE_PROJECT_DIR?.trim();
  if (claudeRoot) loadDir(claudeRoot);

  loadDotEnvFile(join(homedir(), ".gemini", "config", ".env"));
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
