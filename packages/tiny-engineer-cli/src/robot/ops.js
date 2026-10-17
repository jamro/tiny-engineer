import { getBaseUrl, getToken, resolveTransport } from "../config/env.js";
import { robotFetch } from "./client.js";
import { serialRequest } from "./serial/client.js";

/**
 * @typedef {{ transport?: string, url?: string, port?: string | null }} TransportOpts
 */

/**
 * @param {Response} response
 * @returns {Promise<import("./client.js").RobotResult & { bodyJson?: Record<string, unknown> }>}
 */
async function fromHttpResponse(response) {
  const text = await response.text();
  /** @type {Record<string, unknown> | undefined} */
  let bodyJson;
  try {
    bodyJson = text ? JSON.parse(text) : undefined;
  } catch {
    // not JSON
  }
  if (response.ok) {
    return { ok: true, status: response.status, body: text, bodyJson };
  }
  const error =
    typeof bodyJson?.error === "string"
      ? bodyJson.error
      : text.trim() || `HTTP ${response.status}`;
  return { ok: false, status: response.status, body: text, bodyJson, error };
}

/**
 * @param {string} op
 * @param {Record<string, unknown>} fields
 * @param {TransportOpts} opts
 * @param {() => Promise<import("./client.js").RobotResult & { bodyJson?: Record<string, unknown> }>} httpFn
 */
async function withTransport(op, fields, opts, httpFn) {
  const resolved = resolveTransport(opts.transport);
  if (resolved.error) {
    return { ok: false, error: resolved.error };
  }
  if (resolved.transport === "serial") {
    try {
      const body = await serialRequest(op, fields, { port: opts.port });
      if (body.ok === false) {
        return {
          ok: false,
          error: typeof body.error === "string" ? body.error : `${op} failed`,
          bodyJson: body,
        };
      }
      return { ok: true, bodyJson: body };
    } catch (err) {
      return { ok: false, error: err instanceof Error ? err.message : String(err) };
    }
  }
  return httpFn();
}

/**
 * @param {TransportOpts} [opts]
 */
export async function robotSettingsGet(opts = {}) {
  return withTransport("settings.get", {}, opts, async () => {
    const res = await robotFetch(
      opts.url ?? getBaseUrl(),
      "/settings",
      { method: "GET", signal: AbortSignal.timeout(5000) },
      getToken(),
    );
    return fromHttpResponse(res);
  });
}

/**
 * @param {Record<string, unknown>} patch
 * @param {TransportOpts} [opts]
 */
export async function robotSettingsSet(patch, opts = {}) {
  return withTransport("settings.set", patch, opts, async () => {
    const res = await robotFetch(
      opts.url ?? getBaseUrl(),
      "/settings",
      {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(patch),
        signal: AbortSignal.timeout(5000),
      },
      getToken(),
    );
    return fromHttpResponse(res);
  });
}

/**
 * @param {string} name audio|bell|screen|movement|led
 * @param {TransportOpts} [opts]
 */
export async function robotHardwareTest(name, opts = {}) {
  const path =
    name === "bell" ? "/test/audio/bell" : name === "audio" ? "/test/audio" : `/test/${name}`;
  return withTransport("test", { name }, opts, async () => {
    const res = await robotFetch(
      opts.url ?? getBaseUrl(),
      path,
      { method: "POST", signal: AbortSignal.timeout(60_000) },
      getToken(),
    );
    return fromHttpResponse(res);
  });
}

/**
 * @param {number} index
 * @param {number} angle
 * @param {TransportOpts} [opts]
 */
export async function robotServo(index, angle, opts = {}) {
  return withTransport("servo", { index, angle }, opts, async () => {
    const res = await robotFetch(
      opts.url ?? getBaseUrl(),
      "/test/servo",
      {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ index, angle }),
        signal: AbortSignal.timeout(30_000),
      },
      getToken(),
    );
    return fromHttpResponse(res);
  });
}

/**
 * @param {TransportOpts} [opts]
 */
export async function robotFactoryReset(opts = {}) {
  return withTransport("factory_reset", {}, opts, async () => {
    const res = await robotFetch(
      opts.url ?? getBaseUrl(),
      "/settings/reset",
      { method: "POST", signal: AbortSignal.timeout(10_000) },
      getToken(),
    );
    return fromHttpResponse(res);
  });
}
