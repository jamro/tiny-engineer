import { resolve } from "node:path";

import { getBaseUrl, getHttpAuthToken, resolveTransport } from "../config/env.js";
import { serialPlay, serialRequest } from "../daemon/robot.js";
import { formatRobotError, networkErrorDetail, postAnim, postPlay, robotFetch } from "./client.js";
import { withTransport } from "./with-transport.js";

/**
 * @typedef {{
 *   transport?: string,
 *   url?: string,
 *   port?: string | null,
 * }} TransportOpts
 */

/**
 * @param {TransportOpts} [opts]
 * @returns {string | null}
 */
function httpToken(opts = {}) {
  return getHttpAuthToken({ urlOverride: opts.url !== undefined });
}

/**
 * @param {TransportOpts} [opts]
 * @returns {{ transport: "http" | "serial", error?: undefined } | { error: string }}
 */
export function resolveRobotTransport(opts = {}) {
  return resolveTransport(opts.transport);
}

/**
 * @param {string} name
 * @param {TransportOpts} [opts]
 * @returns {Promise<import("./client.js").RobotResult & { target?: string }>}
 */
export async function robotSetAnim(name, opts = {}) {
  const result = await withTransport(opts, {
    serial: async () => {
      try {
        const body = await serialRequest("anim", { name }, { port: opts.port });
        if (body.ok === false) {
          return {
            ok: false,
            error: typeof body.error === "string" ? body.error : "serial anim failed",
          };
        }
        return { ok: true, target: "serial" };
      } catch (err) {
        return { ok: false, error: err instanceof Error ? err.message : String(err) };
      }
    },
    http: async () => {
      const baseUrl = opts.url ?? getBaseUrl();
      const httpResult = await postAnim(baseUrl, name, httpToken(opts));
      return { ...httpResult, target: baseUrl };
    },
  });
  return /** @type {import("./client.js").RobotResult & { target?: string }} */ (result);
}

/**
 * @param {{ wav: Uint8Array, wavPath?: string, name?: string } & TransportOpts} opts
 * @returns {Promise<import("./client.js").RobotResult & { target?: string }>}
 */
export async function robotPlay(opts) {
  const result = await withTransport(opts, {
    serial: async () => {
      if (!opts.wavPath) {
        return { ok: false, error: "serial play needs a WAV file path", target: "serial" };
      }
      try {
        const body = await serialPlay({
          path: resolve(opts.wavPath),
          name: opts.name ?? "talking",
          port: opts.port ?? null,
        });
        if (body.ok === false) {
          return {
            ok: false,
            error: typeof body.error === "string" ? body.error : "serial play failed",
            target: "serial",
          };
        }
        return { ok: true, target: "serial", body: JSON.stringify(body) };
      } catch (err) {
        return {
          ok: false,
          error: err instanceof Error ? err.message : String(err),
          target: "serial",
        };
      }
    },
    http: async () => {
      const baseUrl = opts.url ?? getBaseUrl();
      const httpResult = await postPlay({
        baseUrl,
        wav: opts.wav,
        name: opts.name,
        token: httpToken(opts),
      });
      return { ...httpResult, target: baseUrl };
    },
  });
  return /** @type {import("./client.js").RobotResult & { target?: string }} */ (result);
}

/**
 * @param {TransportOpts} [opts]
 * @returns {Promise<{ ok: boolean, detail: string, target?: string }>}
 */
export async function robotHealth(opts = {}) {
  const result = await withTransport(opts, {
    serial: async () => {
      try {
        const body = await serialRequest("health", {}, { port: opts.port });
        if (body.ok === false) {
          return {
            ok: false,
            detail: typeof body.error === "string" ? body.error : "serial health failed",
            target: "serial",
          };
        }
        return { ok: true, detail: JSON.stringify(body).slice(0, 120), target: "serial" };
      } catch (err) {
        return {
          ok: false,
          detail: err instanceof Error ? err.message : String(err),
          target: "serial",
        };
      }
    },
    http: async () => {
      const baseUrl = opts.url ?? getBaseUrl();
      try {
        const res = await robotFetch(
          baseUrl,
          "/health",
          { method: "GET", signal: AbortSignal.timeout(2000) },
          httpToken(opts),
        );
        const body = await res.text();
        if (res.ok) {
          return { ok: true, detail: body.trim() || `HTTP ${res.status}`, target: baseUrl };
        }
        return { ok: false, detail: body.trim() || `HTTP ${res.status}`, target: baseUrl };
      } catch (err) {
        return {
          ok: false,
          detail: networkErrorDetail(err),
          target: baseUrl,
        };
      }
    },
  });
  // withTransport resolve failures use `error`; health callers expect `detail`.
  if (result.ok === false && typeof result.error === "string" && result.detail === undefined) {
    return { ok: false, detail: result.error };
  }
  return /** @type {{ ok: boolean, detail: string, target?: string }} */ (result);
}

/**
 * @param {string} op
 * @param {Record<string, unknown>} [fields]
 * @param {TransportOpts} [opts]
 * @returns {Promise<import("./client.js").RobotResult & { bodyJson?: Record<string, unknown>, target?: string }>}
 */
export async function robotOp(op, fields = {}, opts = {}) {
  const result = await withTransport(opts, {
    serial: async () => {
      try {
        const body = await serialRequest(op, fields, { port: opts.port });
        if (body.ok === false) {
          return {
            ok: false,
            error: typeof body.error === "string" ? body.error : `${op} failed`,
            bodyJson: body,
            target: "serial",
          };
        }
        return { ok: true, bodyJson: body, target: "serial" };
      } catch (err) {
        return { ok: false, error: err instanceof Error ? err.message : String(err) };
      }
    },
    http: async () => ({
      ok: false,
      error: `HTTP transport does not support op=${op} via robotOp (use dedicated helpers)`,
    }),
  });
  return /** @type {import("./client.js").RobotResult & { bodyJson?: Record<string, unknown>, target?: string }} */ (
    result
  );
}

export { formatRobotError };
