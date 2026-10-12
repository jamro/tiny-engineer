import { robotFetch } from "./http.js";
import { request } from "./transport.js";

export { robotFetch };

/** Allowed POST /anim?name= values (firmware allow-list). */
export const ANIM_NAMES = new Set([
  "none",
  "typing",
  "reading",
  "thinking",
  "talking",
  "ring",
  "welcome",
  "attention",
  "error",
  "abort",
  "dead",
  "wakeup",
  "sleep",
]);

/** Allowed POST /play/<name> path segments. */
export const PLAY_ANIM_NAMES = new Set([
  "talking",
  "typing",
  "reading",
  "thinking",
  "none",
]);

/**
 * @typedef {{ ok: boolean, status?: number, body?: string, error?: string }} RobotResult
 */

/**
 * @param {string} name
 * @returns {string | null} error message, or null if valid
 */
export function validateAnimName(name) {
  if (ANIM_NAMES.has(name)) return null;
  const allowed = [...ANIM_NAMES].join(", ");
  return `unknown animation: ${name} (allowed: ${allowed})`;
}

/**
 * @param {string} name
 * @returns {string | null} error message, or null if valid
 */
export function validatePlayAnimName(name) {
  if (PLAY_ANIM_NAMES.has(name)) return null;
  const allowed = [...PLAY_ANIM_NAMES].join(", ");
  return `unknown play animation: ${name} (allowed: ${allowed})`;
}

/**
 * @param {RobotResult} result
 * @returns {string}
 */
export function formatRobotError(result) {
  if (result.error) return result.error;
  if (result.status !== undefined) return `HTTP ${result.status}`;
  return "robot request failed";
}

/**
 * @param {string} body
 * @param {number} status
 * @returns {string}
 */
function errorFromBody(body, status) {
  try {
    const json = JSON.parse(body);
    if (typeof json?.error === "string" && json.error) return json.error;
  } catch {
    // not JSON
  }
  const trimmed = body?.trim();
  if (trimmed) return trimmed;
  return `HTTP ${status}`;
}

/**
 * @param {unknown} err
 * @returns {string}
 */
function unreachableMessage(err) {
  const detail =
    err && typeof err === "object" && "cause" in err && err.cause instanceof Error
      ? err.cause.message
      : err instanceof Error
        ? err.message
        : String(err);
  return `robot unreachable: ${detail}`;
}

/**
 * POST /anim?name=…. Never throws — returns a RobotResult.
 * @param {string | import("./transport.js").Transport} baseUrlOrTransport
 * @param {string} animName
 * @param {string | null} [token]
 * @returns {Promise<RobotResult>}
 */
export async function postAnim(baseUrlOrTransport, animName, token = null) {
  const transport =
    typeof baseUrlOrTransport === "string"
      ? { type: /** @type {const} */ ("http"), baseUrl: baseUrlOrTransport }
      : baseUrlOrTransport;
  return request(transport, {
    method: "POST",
    path: "/anim",
    query: { name: animName },
    token,
    timeoutMs: 2000,
  });
}

// The robot answers once the clip has finished playing.
const PLAY_TIMEOUT_MS = 10 * 60 * 1000;

/**
 * POST a WAV to /play (or /play/<name>). Never throws — returns a RobotResult.
 * @param {{ baseUrl: string, wav: Uint8Array, name?: string, token?: string | null }} request
 * @returns {Promise<RobotResult>}
 */
export async function postPlay({ baseUrl, wav, name, token }) {
  try {
    const response = await robotFetch(
      baseUrl,
      name ? `/play/${encodeURIComponent(name)}` : "/play",
      {
        method: "POST",
        headers: { "Content-Type": "audio/wav" },
        body: wav,
        signal: AbortSignal.timeout(PLAY_TIMEOUT_MS),
      },
      token,
    );
    const body = await response.text();
    if (response.ok) {
      return { ok: true, status: response.status, body };
    }
    return {
      ok: false,
      status: response.status,
      body,
      error: errorFromBody(body, response.status),
    };
  } catch (err) {
    return { ok: false, error: unreachableMessage(err) };
  }
}
