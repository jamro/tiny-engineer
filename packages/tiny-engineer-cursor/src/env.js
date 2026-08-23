import { readFileSync } from "node:fs";
import { join } from "node:path";

const TOKEN_KEY = "TINY_ENGINEER_TOKEN";

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
 * Load `<cwd>/.env` into process.env for keys not already set.
 * Missing file / read errors are ignored.
 * @param {string} [cwd]
 */
export function loadDotEnv(cwd = process.cwd()) {
  let raw;
  try {
    raw = readFileSync(join(cwd, ".env"), "utf8");
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
