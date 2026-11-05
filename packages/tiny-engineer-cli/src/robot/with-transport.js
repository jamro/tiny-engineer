import { resolveTransport } from "../config/env.js";

/**
 * Shared HTTP vs serial router. Callers supply serial/http work; this only
 * resolves transport and invokes the matching callback (no `target` injection).
 *
 * @typedef {{ transport?: string, url?: string, port?: string | null }} TransportOpts
 *
 * @param {TransportOpts} opts
 * @param {{
 *   serial: () => Promise<Record<string, unknown>>,
 *   http: () => Promise<Record<string, unknown>>,
 * }} handlers
 * @returns {Promise<Record<string, unknown>>}
 */
export async function withTransport(opts, { serial, http }) {
  const resolved = resolveTransport(opts.transport);
  if (resolved.error) {
    return { ok: false, error: resolved.error };
  }
  if (resolved.transport === "serial") {
    return serial();
  }
  return http();
}
