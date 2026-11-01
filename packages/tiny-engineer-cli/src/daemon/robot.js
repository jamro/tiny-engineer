import { daemonPlay, daemonRobotRequest } from "./client.js";

/**
 * One-shot robot op through the host daemon. The daemon owns the USB port.
 * SerialRobotClient runs only inside that process.
 * @param {string} op
 * @param {Record<string, unknown>} [fields]
 * @param {{ port?: string | null }} [opts]
 * @returns {Promise<Record<string, unknown>>}
 */
export function serialRequest(op, fields = {}, opts = {}) {
  return daemonRobotRequest(op, fields, { port: opts.port ?? null });
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
