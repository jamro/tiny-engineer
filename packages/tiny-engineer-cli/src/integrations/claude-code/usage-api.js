import { spawn } from "node:child_process";

export const USAGE_URL = "https://api.anthropic.com/api/oauth/usage";
export const OAUTH_BETA = "oauth-2025-04-20";

/**
 * @returns {string[]}
 */
export function curlArgs() {
  return ["--config", "-"];
}

/**
 * Bearer stays in the config body, not in argv.
 * @param {string} token
 * @param {number} timeoutSec
 * @returns {string | null}
 */
export function curlConfig(token, timeoutSec) {
  if (!token || /[\r\n"]/.test(token)) return null;
  return [
    `header = "Authorization: Bearer ${token}"`,
    `header = "anthropic-beta: ${OAUTH_BETA}"`,
    `url = "${USAGE_URL}"`,
    "silent",
    `max-time = ${timeoutSec}`,
    "",
  ].join("\n");
}

/**
 * @param {AbortSignal | undefined} signal
 * @param {number} timeoutMs
 * @returns {{ signal: AbortSignal, done: () => void }}
 */
function composeSignal(signal, timeoutMs) {
  const ac = new AbortController();
  const timer = setTimeout(() => ac.abort(), timeoutMs);
  const onAbort = () => ac.abort();
  if (signal?.aborted) ac.abort();
  else signal?.addEventListener("abort", onAbort, { once: true });
  return {
    signal: ac.signal,
    done() {
      clearTimeout(timer);
      signal?.removeEventListener("abort", onAbort);
    },
  };
}

/**
 * @param {typeof fetch} fetchImpl
 * @param {string} token
 * @param {number} timeoutMs
 * @param {AbortSignal | undefined} signal
 * @returns {Promise<{ kind: "ok", json: Record<string, unknown> } | { kind: "retry" } | { kind: "fail" }>}
 */
async function requestFetch(fetchImpl, token, timeoutMs, signal) {
  const composed = composeSignal(signal, timeoutMs);
  try {
    const res = await fetchImpl(USAGE_URL, {
      method: "GET",
      headers: {
        Authorization: `Bearer ${token}`,
        "anthropic-beta": OAUTH_BETA,
        Accept: "application/json",
      },
      signal: composed.signal,
    });
    if (res.status === 403) return { kind: "retry" };
    if (!res.ok) return { kind: "fail" };
    const json = await res.json();
    if (!json || typeof json !== "object") return { kind: "fail" };
    return { kind: "ok", json: /** @type {Record<string, unknown>} */ (json) };
  } catch {
    if (signal?.aborted) return { kind: "fail" };
    return { kind: "retry" };
  } finally {
    composed.done();
  }
}

/**
 * @param {string} token
 * @param {{ timeoutMs: number, signal?: AbortSignal }} opts
 * @returns {Promise<Record<string, unknown> | null>}
 */
function defaultSpawnCurl(token, opts) {
  const config = curlConfig(token, Math.max(opts.timeoutMs / 1000, 0.1));
  if (!config || opts.signal?.aborted) return Promise.resolve(null);

  return new Promise((resolve) => {
    /** @type {import("node:child_process").ChildProcessWithoutNullStreams} */
    let child;
    try {
      child = spawn("curl", curlArgs(), { stdio: ["pipe", "pipe", "ignore"] });
    } catch {
      resolve(null);
      return;
    }

    let settled = false;
    let out = "";
    const signal = opts.signal;

    /** @param {Record<string, unknown> | null} value */
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
    }, opts.timeoutMs);

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
      try {
        const json = JSON.parse(out);
        finish(json && typeof json === "object" ? json : null);
      } catch {
        finish(null);
      }
    });
    child.stdin.write(config);
    child.stdin.end();
  });
}

/**
 * Fetch Claude subscription usage. Node fetch first; one curl retry on 403 or network error.
 *
 * @param {string} token
 * @param {{
 *   timeoutMs?: number,
 *   signal?: AbortSignal,
 *   fetchImpl?: typeof fetch,
 *   spawnCurl?: (token: string, opts: { timeoutMs: number, signal?: AbortSignal }) => Promise<Record<string, unknown> | null>,
 * }} [opts]
 * @returns {Promise<Record<string, unknown> | null>}
 */
export async function fetchClaudeUsage(token, opts = {}) {
  if (opts.signal?.aborted) return null;
  const timeoutMs = opts.timeoutMs ?? 1500;
  const fetchImpl = opts.fetchImpl ?? globalThis.fetch;
  const primary = await requestFetch(fetchImpl, token, timeoutMs, opts.signal);
  if (primary.kind === "ok") return primary.json;
  if (primary.kind !== "retry" || opts.signal?.aborted) return null;
  const spawnCurl = opts.spawnCurl ?? defaultSpawnCurl;
  return spawnCurl(token, { timeoutMs, signal: opts.signal });
}
