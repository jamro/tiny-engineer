import { existsSync } from "node:fs";

import { userConfigDir } from "../config/paths.js";
import { tryConnect } from "./connect.js";
import { writeDisable, writeYield } from "./flags.js";
import { daemonPaths } from "./paths.js";
import { delay } from "./proc.js";

/**
 * Close the serial port and leave the process up.
 * `yield` writes the 180s refusal and forces the port closed during play.
 * @param {{ configDir?: string, yield?: boolean, now?: number }} [opts]
 */
export async function releaseDaemon(opts = {}) {
  const configDir = opts.configDir ?? userConfigDir();
  const paths = daemonPaths(configDir);
  if (opts.yield) writeYield(configDir, opts.now ?? Date.now());
  const conn = await tryConnect(paths.sock, 300);
  if (!conn) return;
  try {
    const res = await conn.peer.request("daemon.release", { force: opts.yield === true }, 2000);
    if (typeof res.error === "string") throw new Error(res.error);
  } finally {
    conn.socket.destroy();
  }
}

/**
 * Deprecated alias of {@link releaseDaemon}. Prefer `releaseDaemon` / CLI `daemon release`.
 * @deprecated Prefer {@link releaseDaemon}.
 * @param {{ configDir?: string, yield?: boolean, now?: number }} [opts]
 */
export async function stopDaemon(opts = {}) {
  await releaseDaemon(opts);
}

/**
 * Ask the process to exit. Does not write a disable file.
 * A refused replace returns "play-active" without waiting for the socket.
 * @param {string} configDir
 * @param {"stop" | "replace"} [reason]
 * @returns {Promise<"play-active" | void>}
 */
export async function shutdownDaemon(configDir, reason = "stop") {
  const paths = daemonPaths(configDir);
  const conn = await tryConnect(paths.sock, 300);
  if (!conn) return;
  /** @type {Record<string, unknown> | undefined} */
  let reply;
  try {
    reply = await conn.peer.request("daemon.stop", { reason }, 2000);
  } catch {
    // process may already be gone
  } finally {
    conn.socket.destroy();
  }
  if (reply?.error === "play-active") return "play-active";
  const deadline = Date.now() + 2000;
  while (Date.now() < deadline) {
    if (process.platform === "win32") {
      const probe = await tryConnect(paths.sock, 200);
      if (!probe) return;
      probe.socket.destroy();
    } else if (!existsSync(paths.sock)) {
      return;
    }
    await delay(30);
  }
}

/**
 * Sticky opt-out. The process exits and auto-start stays refused.
 * @param {{ configDir?: string }} [opts]
 */
export async function disableDaemon(opts = {}) {
  const configDir = opts.configDir ?? userConfigDir();
  writeDisable(configDir);
  await shutdownDaemon(configDir, "stop");
}
