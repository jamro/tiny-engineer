import { spawn } from "node:child_process";
import { readFileSync } from "node:fs";
import { homedir } from "node:os";
import { join } from "node:path";

export const KEYCHAIN_SERVICE = "Claude Code-credentials";
const SOURCE_TIMEOUT_MS = 2000;

/**
 * @param {string | null | undefined} raw
 * @returns {string | null}
 */
export function accessTokenFromCredentials(raw) {
  if (typeof raw !== "string") return null;
  const trimmed = raw.trim();
  if (!trimmed) return null;
  if (!trimmed.startsWith("{")) return trimmed;

  try {
    const parsed = JSON.parse(trimmed);
    const token = parsed?.claudeAiOauth?.accessToken;
    if (typeof token !== "string") return null;
    const bearer = token.trim();
    return bearer || null;
  } catch {
    return null;
  }
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
    child.on("error", () => finish(null));
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
 * @param {string} [home]
 * @returns {string}
 */
export function credentialsPath(home = homedir()) {
  return join(home, ".claude", ".credentials.json");
}

/**
 * @param {string} [home]
 * @returns {string | null}
 */
function readCredentialsFile(home = homedir()) {
  try {
    return readFileSync(credentialsPath(home), "utf8");
  } catch {
    return null;
  }
}

/**
 * Resolve a Claude Code OAuth access token. Read-only: never writes or refreshes.
 *
 * @param {{
 *   signal?: AbortSignal,
 *   sources?: Array<(signal?: AbortSignal) => Promise<string | null>>,
 *   readFile?: () => string | null | Promise<string | null>,
 * }} [opts]
 * @returns {Promise<string | null>}
 */
export async function resolveClaudeAccessToken(opts = {}) {
  const signal = opts.signal;
  const sources = opts.sources ?? [fromMacosKeychain];
  const readFile = opts.readFile ?? (() => readCredentialsFile());
  try {
    if (isAborted(signal)) return null;
    for (const source of sources) {
      if (isAborted(signal)) return null;
      const raw = await source(signal);
      const token = accessTokenFromCredentials(raw);
      if (token) return token;
    }
    if (isAborted(signal)) return null;
    return accessTokenFromCredentials(await readFile());
  } catch {
    return null;
  }
}
