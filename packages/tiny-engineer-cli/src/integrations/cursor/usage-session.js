import { spawn } from "node:child_process";
import { homedir } from "node:os";
import { join } from "node:path";

const KEYCHAIN_SERVICE = "cursor-access-token";
const STATE_DB_KEY = "cursorAuth/accessToken";
const SOURCE_TIMEOUT_MS = 2000;

/**
 * @param {string} token
 * @returns {Record<string, unknown>}
 */
export function jwtClaims(token) {
  try {
    const parts = token.split(".");
    if (parts.length < 2) return {};
    let payload = parts[1].replace(/-/g, "+").replace(/_/g, "/");
    payload += "=".repeat((4 - (payload.length % 4)) % 4);
    return JSON.parse(Buffer.from(payload, "base64").toString("utf8"));
  } catch {
    return {};
  }
}

/**
 * @param {Record<string, unknown>} claims
 * @returns {string | null}
 */
export function cookieIdFromClaims(claims) {
  const sub = claims.sub;
  if (typeof sub !== "string" || !sub) return null;
  const parts = sub.split("|");
  return parts[parts.length - 1] || null;
}

/**
 * @param {string} token
 * @returns {string | null} full `id::jwt` cookie value, or null if unusable
 */
export function cookieFromToken(token) {
  const raw = token.trim().replace(/%3A%3A/gi, "::");
  if (!raw) return null;
  if (raw.includes("::")) return raw;

  const claims = jwtClaims(raw);
  if (claims.type === "api_key_token") return null;
  const id = cookieIdFromClaims(claims);
  if (!id) return null;
  return `${id}::${raw}`;
}

/**
 * @param {string} command
 * @param {string[]} args
 * @param {number} timeoutMs
 * @returns {Promise<string | null>}
 */
function spawnStdout(command, args, timeoutMs) {
  return new Promise((resolve) => {
    /** @type {import("node:child_process").ChildProcessWithoutNullStreams} */
    let child;
    try {
      child = spawn(command, args, { stdio: ["ignore", "pipe", "ignore"] });
    } catch {
      resolve(null);
      return;
    }

    let out = "";
    const timer = setTimeout(() => {
      child.kill("SIGKILL");
      resolve(null);
    }, timeoutMs);

    child.stdout.setEncoding("utf8");
    child.stdout.on("data", (chunk) => {
      out += chunk;
    });
    child.on("error", () => {
      clearTimeout(timer);
      resolve(null);
    });
    child.on("close", (code) => {
      clearTimeout(timer);
      if (code !== 0) {
        resolve(null);
        return;
      }
      const trimmed = out.trim();
      resolve(trimmed || null);
    });
  });
}

/**
 * @returns {Promise<string | null>}
 */
async function fromMacosKeychain() {
  if (process.platform !== "darwin") return null;
  return spawnStdout(
    "security",
    ["find-generic-password", "-s", KEYCHAIN_SERVICE, "-w"],
    SOURCE_TIMEOUT_MS,
  );
}

/**
 * @returns {string[]}
 */
export function stateDbCandidates() {
  const home = homedir();
  /** @type {string} */
  let base;
  if (process.platform === "darwin") {
    base = join(home, "Library", "Application Support");
  } else if (process.platform === "win32") {
    base = process.env.APPDATA || join(home, "AppData", "Roaming");
  } else {
    base = process.env.XDG_CONFIG_HOME || join(home, ".config");
  }
  return ["Cursor", "Cursor Nightly"].map((app) =>
    join(base, app, "User", "globalStorage", "state.vscdb"),
  );
}

/**
 * @param {string} dbPath
 * @returns {Promise<string | null>}
 */
async function tokenFromStateDb(dbPath) {
  // Escape single quotes for sqlite string literal.
  const key = STATE_DB_KEY.replace(/'/g, "''");
  const sql = `SELECT value FROM ItemTable WHERE key = '${key}' LIMIT 1;`;
  const out = await spawnStdout("sqlite3", ["-readonly", dbPath, sql], SOURCE_TIMEOUT_MS);
  if (!out) return null;
  return out.replace(/^"|"$/g, "").trim() || null;
}

/**
 * @returns {Promise<string | null>}
 */
async function fromStateDb() {
  for (const path of stateDbCandidates()) {
    const token = await tokenFromStateDb(path);
    if (token) return token;
  }
  return null;
}

/**
 * Resolve WorkOS session cookie value `id::jwt` for cursor.com dashboard APIs.
 * Returns null when no usable web session is found (never throws).
 *
 * @returns {Promise<string | null>}
 */
export async function resolveCursorSessionCookie() {
  try {
    const env = process.env.CURSOR_SESSION_TOKEN?.trim();
    if (env) {
      const cookie = cookieFromToken(env);
      if (cookie) return cookie;
    }

    for (const source of [fromMacosKeychain, fromStateDb]) {
      const token = await source();
      if (!token) continue;
      const cookie = cookieFromToken(token);
      if (cookie) return cookie;
    }
  } catch {
    // fail-open
  }
  return null;
}
