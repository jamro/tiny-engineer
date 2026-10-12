import { getBaseUrl, getSerialPath } from "../config/env.js";
import { robotFetch } from "./http.js";
import { serialRequest } from "./serial.js";

/** @typedef {import("./client.js").RobotResult} RobotResult */

/**
 * @typedef {{ type: "http", baseUrl: string } | { type: "serial", path: string }} Transport
 */

/**
 * Resolve CLI transport. Flag `--serial` / env serial beat HTTP unless only `--url` is set
 * without serial. Passing both `--url` and `--serial` is an error (caller checks flags).
 *
 * Precedence: explicit serialPath → TINY_ENGINEER_SERIAL → HTTP (url / env / default).
 *
 * @param {{ url?: string, serial?: string }} opts
 * @returns {Transport}
 */
export function resolveTransport({ url, serial } = {}) {
  if (serial) {
    return { type: "serial", path: serial };
  }
  const envSerial = getSerialPath();
  if (envSerial) {
    return { type: "serial", path: envSerial };
  }
  return { type: "http", baseUrl: url ?? getBaseUrl() };
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
 * @param {unknown} body
 * @param {number} status
 * @returns {string}
 */
function errorFromBody(body, status) {
  try {
    const json = typeof body === "string" ? JSON.parse(body) : body;
    if (typeof json?.error === "string" && json.error) return json.error;
  } catch {
    // not JSON
  }
  const trimmed = typeof body === "string" ? body.trim() : "";
  if (trimmed) return trimmed;
  return `HTTP ${status}`;
}

/**
 * @param {Transport} transport
 * @param {{ method: string, path: string, query?: Record<string, string | undefined>, token?: string | null, timeoutMs?: number }} opts
 * @returns {Promise<RobotResult>}
 */
export async function request(transport, opts) {
  if (transport.type === "serial") {
    return serialRequest(transport.path, opts);
  }

  const params = new URLSearchParams();
  for (const [k, v] of Object.entries(opts.query ?? {})) {
    if (v !== undefined) params.set(k, v);
  }
  const qs = params.toString();
  const path = qs ? `${opts.path}?${qs}` : opts.path;

  try {
    const response = await robotFetch(
      transport.baseUrl,
      path,
      {
        method: opts.method,
        signal: AbortSignal.timeout(opts.timeoutMs ?? 2000),
      },
      opts.token ?? null,
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

/**
 * @param {Transport} transport
 * @returns {string}
 */
export function transportLabel(transport) {
  return transport.type === "serial" ? transport.path : transport.baseUrl;
}
