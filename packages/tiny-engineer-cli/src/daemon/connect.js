import { spawn } from "node:child_process";
import { mkdirSync, readFileSync, rmSync, statSync, openSync, closeSync, writeSync } from "node:fs";
import { connect as netConnect } from "node:net";

import { createPeer } from "./ipc.js";
import { daemonPaths } from "./paths.js";
import { delay, pidLiveness } from "./proc.js";
import { wrapperAlive } from "./wrapper.js";

/**
 * @param {string} configDir
 */
export function readState(configDir) {
  try {
    return JSON.parse(readFileSync(daemonPaths(configDir).state, "utf8"));
  } catch {
    return null;
  }
}

/**
 * @param {string} configDir
 */
export function clearStale(configDir) {
  const paths = daemonPaths(configDir);
  if (process.platform !== "win32") rmSync(paths.sock, { force: true });
  rmSync(paths.state, { force: true });
}

/** Longer than the ~3s pollHello hold, so a live starter is not stolen. */
const STALE_LOCK_MS = 10_000;

/**
 * @param {string} lockPath
 * @returns {boolean}
 */
function createLock(lockPath) {
  try {
    const fd = openSync(lockPath, "wx", 0o600);
    writeSync(fd, String(process.pid));
    closeSync(fd);
    return true;
  } catch (err) {
    if (err && typeof err === "object" && "code" in err && err.code === "EEXIST") return false;
    throw err;
  }
}

/**
 * A hard kill between create and unlock leaves the file. Drop it when the
 * owner is gone, or when the file is older than a live start can last.
 * @param {string} lockPath
 */
function lockIsStale(lockPath) {
  let text = "";
  try {
    text = readFileSync(lockPath, "utf8").trim();
  } catch {
    return true;
  }
  const pid = Number(text);
  if (!text || !Number.isInteger(pid) || pid <= 0 || pidLiveness(pid) === "dead") return true;
  try {
    return Date.now() - statSync(lockPath).mtimeMs > STALE_LOCK_MS;
  } catch {
    return true;
  }
}

/**
 * @param {string} configDir
 */
export function tryLock(configDir) {
  const paths = daemonPaths(configDir);
  mkdirSync(paths.dir, { recursive: true, mode: 0o700 });
  if (createLock(paths.lock)) return true;
  if (!lockIsStale(paths.lock)) return false;
  rmSync(paths.lock, { force: true });
  return createLock(paths.lock);
}

/**
 * @param {string} configDir
 */
export function unlock(configDir) {
  rmSync(daemonPaths(configDir).lock, { force: true });
}

/**
 * @param {string} sockPath
 * @param {number} [timeoutMs]
 */
export function tryConnect(sockPath, timeoutMs = 300) {
  return new Promise((resolve) => {
    const socket = netConnect(sockPath);
    let buf = "";
    let settled = false;
    const finish = (value) => {
      if (settled) return;
      settled = true;
      clearTimeout(timer);
      resolve(value);
    };
    const timer = setTimeout(() => {
      socket.destroy();
      finish(null);
    }, timeoutMs);
    socket.on("error", () => {
      socket.destroy();
      finish(null);
    });
    socket.on("data", (chunk) => {
      if (settled) return;
      buf += chunk.toString("utf8");
      const nl = buf.indexOf("\n");
      if (nl < 0) return;
      try {
        const msg = JSON.parse(buf.slice(0, nl));
        if (msg.method !== "hello" || !msg.params) {
          socket.destroy();
          finish(null);
          return;
        }
        const peer = createPeer(socket, buf.slice(nl + 1));
        finish({ socket, hello: msg.params, peer });
      } catch {
        socket.destroy();
        finish(null);
      }
    });
  });
}

/**
 * @param {string} sockPath
 * @param {number} timeoutMs
 */
export async function pollHello(sockPath, timeoutMs) {
  const start = Date.now();
  while (Date.now() - start < timeoutMs) {
    const conn = await tryConnect(sockPath, 200);
    if (conn) return conn;
    await delay(30);
  }
  return null;
}

/**
 * @param {{ script: string, configDir: string, port?: string | null }} opts
 */
export function spawnDaemonProcess({ script, configDir, port = null }) {
  /** @type {string[]} */
  const args = [script, "daemon", "--serve"];
  if (port) args.push("--port", port);
  const child = spawn(process.execPath, args, {
    detached: true,
    stdio: "ignore",
    env: { ...process.env, TINY_ENGINEER_CONFIG_DIR: configDir },
  });
  child.unref();
  return child;
}

/**
 * Start the login wrapper when it is not already alive.
 * Does not spawn `daemon --serve` itself.
 * @param {string} configDir
 */
export async function startWrapperProcess(configDir) {
  if (await wrapperAlive(configDir)) return;
  const paths = daemonPaths(configDir);
  const child = spawn(process.execPath, [paths.wrapper], {
    detached: true,
    stdio: "ignore",
    env: { ...process.env, TINY_ENGINEER_CONFIG_DIR: configDir },
  });
  child.unref();
}

/**
 * @param {string} script
 * @param {string | null | undefined} port
 */
export function daemonServeArgs(script, port) {
  /** @type {string[]} */
  const args = [script, "daemon", "--serve"];
  if (port) args.push("--port", port);
  return args;
}
