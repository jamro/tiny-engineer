import { getBaseUrl, getSerialPath } from "../config/env.js";
import { robotFetch } from "./http.js";
import { detectSerialPort } from "./serial-detect.js";
import { serialRequest } from "./serial.js";

/** @typedef {import("./client.js").RobotResult} RobotResult */

/**
 * @typedef {{ type: "http", baseUrl: string } | { type: "serial", path: string }} Transport
 */

/**
 * Resolve CLI transport.
 * Passing both `--url` and `--serial` is an error (caller checks flags).
 *
 * Precedence: explicit serial (including `"auto"`) → explicit url →
 * TINY_ENGINEER_SERIAL → HTTP (TINY_ENGINEER_URL / default).
 *
 * @param {{ url?: string, serial?: string }} opts
 * @returns {Promise<Transport>}
 */
export async function resolveTransport({ url, serial } = {}) {
  const serialSpec = serial ?? (url ? undefined : getSerialPath()) ?? undefined;

  if (serialSpec) {
    if (serialSpec === "auto") {
      const path = await detectSerialPort();
      return { type: "serial", path };
    }
    return { type: "serial", path: serialSpec };
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
