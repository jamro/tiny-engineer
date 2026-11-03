import { spawn } from "node:child_process";
import { chmodSync, existsSync, rmSync } from "node:fs";
import { connect as netConnect, createServer } from "node:net";

import { daemonPaths } from "./paths.js";
import { delay } from "./proc.js";
import { readDaemonTarget } from "./target.js";

/**
 * A live wrapper holds wrapperLock. A recycled pid file does not count.
 * @param {string} configDir
 * @returns {Promise<boolean>}
 */
export function wrapperAlive(configDir) {
  return lockHeld(daemonPaths(configDir).wrapperLock);
}

/**
 * @param {string} lockPath
 * @returns {Promise<boolean>}
 */
function lockHeld(lockPath) {
  return new Promise((resolve) => {
    const socket = netConnect(lockPath);
    const finish = (value) => {
      socket.removeAllListeners();
      socket.destroy();
      resolve(value);
    };
    socket.setTimeout(200);
    socket.once("connect", () => finish(true));
    socket.once("timeout", () => finish(false));
    socket.once("error", () => finish(false));
  });
}

/**
 * @param {string} configDir
 * @returns {Promise<{ close: () => Promise<void> } | null>}
 */
async function acquireWrapperLock(configDir) {
  const lockPath = daemonPaths(configDir).wrapperLock;
  if (await lockHeld(lockPath)) return null;
  if (process.platform !== "win32") rmSync(lockPath, { force: true });
  const server = createServer((socket) => {
    socket.end();
  });
  try {
    const listening = await new Promise((resolve, reject) => {
      server.once("error", (err) => {
        const code = /** @type {NodeJS.ErrnoException} */ (err).code;
        if (code === "EADDRINUSE") resolve(false);
        else reject(err);
      });
      server.listen(lockPath, () => {
        if (process.platform !== "win32") chmodSync(lockPath, 0o600);
        resolve(true);
      });
    });
    if (!listening) {
      server.close();
      return null;
    }
  } catch (err) {
    server.close();
    throw err;
  }
  return {
    close() {
      return new Promise((resolve) => {
        server.close(() => resolve());
      });
    },
  };
}

/**
 * Parent loop. Restarts a child that had accepted a hello.
 * Stops after a few exits that never accept.
 * A disable file or a missing supervisor file exits 0.
 * @param {{
 *   configDir?: string,
 *   spawnChild?: (target: { execPath: string, script: string }) => { exited: Promise<number | null>, pid?: number },
 *   accepted?: () => Promise<boolean>,
 *   sleep?: (ms: number) => Promise<void>,
 *   now?: () => number,
 * }} [opts]
 * @returns {Promise<number>}
 */
export async function runWrapper(opts = {}) {
  const configDir = opts.configDir ?? process.env.TINY_ENGINEER_CONFIG_DIR ?? "";
  if (!configDir) return 1;
  if (wrapperShouldStop(configDir)) return 0;
  const lock = await acquireWrapperLock(configDir);
  if (!lock) return 0;
  try {
    const sleep = opts.sleep ?? delay;
    const now = opts.now ?? Date.now;
    /** @type {number[]} */
    const early = [];
    for (;;) {
      if (wrapperShouldStop(configDir)) return 0;
      const target = readDaemonTarget(configDir);
      if (!target) return 1;
      const child = (opts.spawnChild ?? ((next) => spawnServeChild(next, configDir)))(target);
      const accepted = await (opts.accepted ?? (() => waitForHello(configDir, child.pid)))();
      const code = await child.exited;
      if (wrapperShouldStop(configDir)) return 0;
      if (accepted) {
        await sleep(50);
        continue;
      }
      const stamp = now();
      early.push(stamp);
      while (early.length && stamp - early[0] > 60_000) early.shift();
      if (early.length >= 3) return 0;
      await sleep(30);
    }
  } finally {
    await lock.close();
  }
}

/**
 * Disable refuses auto-start. A missing supervisor file means uninstall.
 * @param {string} configDir
 */
function wrapperShouldStop(configDir) {
  const paths = daemonPaths(configDir);
  return existsSync(paths.disabled) || !existsSync(paths.supervisor);
}

/**
 * @param {{ execPath: string, script: string }} target
 * @param {string} configDir
 */
function spawnServeChild(target, configDir) {
  const child = spawn(target.execPath, [target.script, "daemon", "--serve"], {
    env: { ...process.env, TINY_ENGINEER_CONFIG_DIR: configDir },
    stdio: "ignore",
  });
  return {
    pid: child.pid,
    exited: new Promise((resolve) => {
      child.once("exit", (code) => resolve(code));
      child.once("error", () => resolve(1));
    }),
  };
}

/**
 * @param {string} configDir
 * @param {number | undefined} pid
 */
async function waitForHello(configDir, pid) {
  if (!Number.isInteger(pid) || pid <= 0) return false;
  const sock = daemonPaths(configDir).sock;
  const deadline = Date.now() + 2000;
  while (Date.now() < deadline) {
    const seen = await readHelloPid(sock);
    if (seen === pid) return true;
    if (seen != null) return false;
    await delay(30);
  }
  return false;
}

/**
 * Pid from the first hello line, or null when the socket is not ready.
 * A hello with no integer pid is NaN so the caller rejects it.
 * @param {string} sock
 * @returns {Promise<number | null>}
 */
function readHelloPid(sock) {
  return new Promise((resolve) => {
    const socket = netConnect(sock);
    let buf = "";
    let settled = false;
    const finish = (value) => {
      if (settled) return;
      settled = true;
      socket.removeAllListeners();
      socket.destroy();
      resolve(value);
    };
    socket.setTimeout(100);
    socket.once("timeout", () => finish(null));
    socket.once("error", () => finish(null));
    socket.on("data", (chunk) => {
      if (settled) return;
      buf += chunk.toString("utf8");
      const nl = buf.indexOf("\n");
      if (nl < 0) return;
      try {
        const msg = JSON.parse(buf.slice(0, nl));
        if (msg.method !== "hello" || !msg.params) {
          finish(null);
          return;
        }
        const helloPid = Number(msg.params.pid);
        finish(Number.isInteger(helloPid) ? helloPid : Number.NaN);
      } catch {
        finish(null);
      }
    });
  });
}
