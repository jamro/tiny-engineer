import { daemonPlay, daemonProbe, daemonRobotRequest } from "./client.js";

/**
 * One-shot robot op through the host daemon. The daemon owns the USB port.
 * SerialRobotClient runs only inside that process.
 * @param {string} op
 * @param {Record<string, unknown>} [fields]
 * @param {{ port?: string | null, timeoutMs?: number }} [opts]
 * @returns {Promise<Record<string, unknown>>}
 */
export function serialRequest(op, fields = {}, opts = {}) {
  /** @type {{ port: string | null, timeoutMs?: number }} */
  const rpcOpts = { port: opts.port ?? null };
  if (opts.timeoutMs != null) rpcOpts.timeoutMs = opts.timeoutMs;
  return daemonRobotRequest(op, fields, rpcOpts);
}

/**
 * WAV play through the host daemon. The daemon reads the file and streams it.
 * SerialRobotClient runs only inside that process.
 * @param {{ path: string, name?: string, port?: string | null }} opts
 * @returns {Promise<Record<string, unknown>>}
 */
export function serialPlay(opts) {
  return daemonPlay({
    path: opts.path,
    name: opts.name ?? "talking",
    port: opts.port ?? null,
  });
}

/**
 * Ordered sync probe through the host daemon. Does not write the port cache.
 * @param {{ candidates: string[], configDir?: string }} opts
 */
export function serialProbe(opts) {
  return daemonProbe({
    candidates: opts.candidates,
    configDir: opts.configDir,
  });
}
