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
    const claims = JSON.parse(Buffer.from(payload, "base64").toString("utf8"));
    // JSON.parse("null") / primitives must not throw later at claims.type —
    // that would abort resolveCursorSessionCookie before Keychain/DB fallback.
    if (claims === null || typeof claims !== "object" || Array.isArray(claims)) {
      return {};
    }
    return /** @type {Record<string, unknown>} */ (claims);
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
 * @param {AbortSignal | undefined} signal
 * @returns {boolean}
 */
function isAborted(signal) {
  return signal != null && signal.aborted;
}

/**
 * @param {string} command
 * @param {string[]} args
 * @param {number} timeoutMs
 * @param {AbortSignal} [signal]
 * @returns {Promise<string | null>}
 */
function spawnStdout(command, args, timeoutMs, signal) {
  return new Promise((resolve) => {
    if (isAborted(signal)) {
      resolve(null);
      return;
    }

    /** @type {import("node:child_process").ChildProcessWithoutNullStreams} */
    let child;
    try {
      child = spawn(command, args, { stdio: ["ignore", "pipe", "ignore"] });
    } catch {
      resolve(null);
      return;
    }

    let settled = false;
    let out = "";

    /** @param {string | null} value */
    const finish = (value) => {
      if (settled) return;
      settled = true;
      clearTimeout(timer);
      signal?.removeEventListener("abort", onAbort);
      resolve(value);
    };

    const onAbort = () => {
      child.kill("SIGKILL");
      finish(null);
    };

    const timer = setTimeout(() => {
      child.kill("SIGKILL");
      finish(null);
    }, timeoutMs);

    signal?.addEventListener("abort", onAbort, { once: true });

    child.stdout.setEncoding("utf8");
    child.stdout.on("data", (chunk) => {
      out += chunk;
    });
    child.on("error", () => {
      finish(null);
    });
    child.on("close", (code) => {
      if (code !== 0) {
        finish(null);
        return;
      }
      finish(out.trim() || null);
    });
  });
}

/**
 * @param {AbortSignal} [signal]
 * @returns {Promise<string | null>}
 */
async function fromMacosKeychain(signal) {
  if (process.platform !== "darwin") return null;
  return spawnStdout(
    "security",
    ["find-generic-password", "-s", KEYCHAIN_SERVICE, "-w"],
    SOURCE_TIMEOUT_MS,
    signal,
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
 * @param {AbortSignal} [signal]
 * @returns {Promise<string | null>}
 */
async function tokenFromStateDb(dbPath, signal) {
  // Escape single quotes for sqlite string literal.
  const key = STATE_DB_KEY.replace(/'/g, "''");
  const sql = `SELECT value FROM ItemTable WHERE key = '${key}' LIMIT 1;`;
  const out = await spawnStdout(
    "sqlite3",
    ["-readonly", dbPath, sql],
    SOURCE_TIMEOUT_MS,
    signal,
  );
  if (!out) return null;
  return out.replace(/^"|"$/g, "").trim() || null;
}

/**
 * First usable raw token from a candidate list (skips api_key / malformed).
 * @param {string[]} tokens
 * @returns {string | null}
 */
export function selectUsableStateDbToken(tokens) {
  for (const token of tokens) {
    if (token && cookieFromToken(token)) return token;
  }
  return null;
}

/**
 * @param {AbortSignal} [signal]
 * @returns {Promise<string | null>}
 */
async function fromStateDb(signal) {
  for (const path of stateDbCandidates()) {
    if (isAborted(signal)) return null;
    const token = await tokenFromStateDb(path, signal);
    const usable = selectUsableStateDbToken(token ? [token] : []);
    if (usable) return usable;
  }
  return null;
}

/**
 * Resolve WorkOS session cookie value `id::jwt` for cursor.com dashboard APIs.
 * Returns null when no usable web session is found (never throws).
 *
 * @param {{
 *   signal?: AbortSignal,
 *   sources?: Array<(signal?: AbortSignal) => Promise<string | null>>,
 * }} [opts]
 * @returns {Promise<string | null>}
 */
export async function resolveCursorSessionCookie(opts = {}) {
  const signal = opts.signal;
  const sources = opts.sources ?? [fromMacosKeychain, fromStateDb];
  try {
    if (isAborted(signal)) return null;

    const env = process.env.CURSOR_SESSION_TOKEN?.trim();
    if (env) {
      const cookie = cookieFromToken(env);
      if (cookie) return cookie;
    }

    for (const source of sources) {
      if (isAborted(signal)) return null;
      const token = await source(signal);
      if (!token) continue;
      const cookie = cookieFromToken(token);
      if (cookie) return cookie;
    }
  } catch {
    // fail-open
  }
  return null;
}
