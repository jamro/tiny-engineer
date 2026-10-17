import { getBaseUrl, getToken, resolveTransport } from "../config/env.js";
import { formatRobotError, postAnim, postPlay, robotFetch } from "./client.js";
import { connectSerialRobot, serialRequest } from "./serial/client.js";

/**
 * @typedef {{
 *   transport?: string,
 *   url?: string,
 *   port?: string | null,
 * }} TransportOpts
 */

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
  const resolved = resolveTransport(opts.transport);
  if (resolved.error) {
    return { ok: false, error: resolved.error };
  }

  if (resolved.transport === "serial") {
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
  }

  const baseUrl = opts.url ?? getBaseUrl();
  const result = await postAnim(baseUrl, name, getToken());
  return { ...result, target: baseUrl };
}

/**
 * @param {{ wav: Uint8Array, name?: string } & TransportOpts} opts
 * @returns {Promise<import("./client.js").RobotResult & { target?: string }>}
 */
export async function robotPlay(opts) {
  const resolved = resolveTransport(opts.transport);
  if (resolved.error) {
    return { ok: false, error: resolved.error };
  }
  if (resolved.transport === "serial") {
    let session;
    try {
      session = await connectSerialRobot({ port: opts.port });
      const body = await session.client.play(opts.wav, opts.name ?? "talking");
      if (body.ok === false) {
        return {
          ok: false,
          error: typeof body.error === "string" ? body.error : "serial play failed",
          target: "serial",
        };
      }
      return { ok: true, target: "serial", body: JSON.stringify(body) };
    } catch (err) {
      return { ok: false, error: err instanceof Error ? err.message : String(err), target: "serial" };
    } finally {
      await session?.client.close().catch(() => {});
    }
  }

  const baseUrl = opts.url ?? getBaseUrl();
  const result = await postPlay({
    baseUrl,
    wav: opts.wav,
    name: opts.name,
    token: getToken(),
  });
  return { ...result, target: baseUrl };
}

/**
 * @param {TransportOpts} [opts]
 * @returns {Promise<{ ok: boolean, detail: string, target?: string }>}
 */
export async function robotHealth(opts = {}) {
  const resolved = resolveTransport(opts.transport);
  if (resolved.error) {
    return { ok: false, detail: resolved.error };
  }

  if (resolved.transport === "serial") {
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
  }

  const baseUrl = opts.url ?? getBaseUrl();
  try {
    const res = await robotFetch(
      baseUrl,
      "/health",
      { method: "GET", signal: AbortSignal.timeout(2000) },
      getToken(),
    );
    const body = await res.text();
    if (res.ok) {
      return { ok: true, detail: body.trim() || `HTTP ${res.status}`, target: baseUrl };
    }
    return { ok: false, detail: body.trim() || `HTTP ${res.status}`, target: baseUrl };
  } catch (err) {
    return {
      ok: false,
      detail: err instanceof Error ? err.message : String(err),
      target: baseUrl,
    };
  }
}

/**
 * @param {string} op
 * @param {Record<string, unknown>} [fields]
 * @param {TransportOpts} [opts]
 * @returns {Promise<import("./client.js").RobotResult & { bodyJson?: Record<string, unknown>, target?: string }>}
 */
export async function robotOp(op, fields = {}, opts = {}) {
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
          target: "serial",
        };
      }
      return { ok: true, bodyJson: body, target: "serial" };
    } catch (err) {
      return { ok: false, error: err instanceof Error ? err.message : String(err) };
    }
  }

  return {
    ok: false,
    error: `HTTP transport does not support op=${op} via robotOp (use dedicated helpers)`,
  };
}

export { formatRobotError };
