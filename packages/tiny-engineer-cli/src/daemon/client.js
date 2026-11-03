import { spawn } from "node:child_process";
import { existsSync, mkdirSync, readFileSync, rmSync, statSync, writeFileSync, openSync, closeSync, writeSync } from "node:fs";
import { connect as netConnect } from "node:net";
import { SerialPort } from "serialport";

import { userConfigDir } from "../config/paths.js";
import { createPeer } from "./ipc.js";
import { daemonPaths } from "./paths.js";
import {
  decideDaemonAction,
  loadIdentity,
  packageRootFromScript,
  resolveCliScript,
  DISABLE_ERROR,
  YIELD_ERROR,
  YIELD_MS,
} from "./policy.js";
import { writeDaemonTarget } from "./target.js";
import { classifyPid, delay, pidLiveness, terminatePid } from "./proc.js";
import { wrapperAlive } from "./wrapper.js";

/**
 * @param {string} configDir
 * @param {number} [now]
 */
export function writeYield(configDir, now = Date.now()) {
  const paths = daemonPaths(configDir);
  mkdirSync(paths.dir, { recursive: true, mode: 0o700 });
  writeFileSync(paths.yieldFile, JSON.stringify({ until: now + YIELD_MS }), { mode: 0o600 });
}

/**
 * @param {string} configDir
 * @param {number} [now]
 */
export function yieldActive(configDir, now = Date.now()) {
  try {
    const until = JSON.parse(readFileSync(daemonPaths(configDir).yieldFile, "utf8")).until;
    return typeof until === "number" && until > now;
  } catch {
    return false;
  }
}

/**
 * @param {string} configDir
 */
export function clearYield(configDir) {
  rmSync(daemonPaths(configDir).yieldFile, { force: true });
}

/**
 * @param {string} configDir
 */
export function writeDisable(configDir) {
  const paths = daemonPaths(configDir);
  mkdirSync(paths.dir, { recursive: true, mode: 0o700 });
  writeFileSync(paths.disabled, "{}\n", { mode: 0o600 });
}

/**
 * @param {string} configDir
 */
export function disableActive(configDir) {
  return existsSync(daemonPaths(configDir).disabled);
}

/**
 * @param {string} configDir
 */
export function clearDisable(configDir) {
  rmSync(daemonPaths(configDir).disabled, { force: true });
}

/**
 * @param {string} configDir
 */
export function supervisorInstalled(configDir) {
  return existsSync(daemonPaths(configDir).supervisor);
}

/**
 * @param {string} configDir
 */
function readState(configDir) {
  try {
    return JSON.parse(readFileSync(daemonPaths(configDir).state, "utf8"));
  } catch {
    return null;
  }
}

/**
 * @param {string} configDir
 */
function clearStale(configDir) {
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
function tryLock(configDir) {
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
function unlock(configDir) {
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
async function pollHello(sockPath, timeoutMs) {
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

/**
 * @typedef {{
 *   configDir?: string,
 *   port?: string | null,
 *   forceStart?: boolean,
 *   now?: number,
 *   spawnFn?: (opts: { script: string, configDir: string, port?: string | null }) => void,
 *   wrapperStart?: (configDir: string) => void | Promise<void>,
 *   probePid?: (pid: number | undefined) => "none" | "dead" | "ours" | "foreign" | "unknown",
 *   terminate?: (pid: number) => Promise<void>,
 *   script?: string,
 * }} EnsureOpts
 */

/**
 * @param {EnsureOpts} [opts]
 */
export async function ensureDaemon(opts = {}) {
  const configDir = opts.configDir ?? userConfigDir();
  const paths = daemonPaths(configDir);
  const now = opts.now ?? Date.now();
  if (!opts.forceStart && disableActive(configDir)) {
    throw new Error(DISABLE_ERROR);
  }
  if (!opts.forceStart && yieldActive(configDir, now)) {
    throw new Error(YIELD_ERROR);
  }

  const script = opts.script ?? resolveCliScript();
  const identity = loadIdentity(packageRootFromScript(script));
  /** @type {{ helloMisses: number, spawns: number }} */
  const ctx = { helloMisses: 0, spawns: 0 };

  /**
   * @returns {Promise<{ socket: import("node:net").Socket, hello: Record<string, unknown>, peer: ReturnType<typeof createPeer> }>}
   */
  const obtain = async () => {
    const quick = await tryConnect(paths.sock, 250);
    if (quick) return quick;
    if (ctx.helloMisses >= 1) throw new Error("daemon failed to start");
    ctx.helloMisses += 1;

    const locked = tryLock(configDir);
    if (!locked) {
      const waited = await pollHello(paths.sock, 3000);
      if (waited) return waited;
      throw new Error("daemon failed to start");
    }
    try {
      const again = await tryConnect(paths.sock, 200);
      if (again) return again;
      const state = readState(configDir);
      const kind = (opts.probePid ?? ((pid) => classifyPid(pid)))(state?.pid);
      if (kind === "unknown") {
        throw new Error(`cannot identify process ${state?.pid}; run tiny-engineer daemon stop`);
      }
      if (kind === "ours" && state?.pid && state.pid !== process.pid) {
        await (opts.terminate ?? terminatePid)(state.pid);
      }
      if (kind === "ours" || kind === "foreign" || kind === "dead") clearStale(configDir);
      if (supervisorInstalled(configDir)) {
        const start = opts.wrapperStart ?? startWrapperProcess;
        await start(configDir);
      } else {
        ctx.spawns += 1;
        const spawnFn = opts.spawnFn ?? spawnDaemonProcess;
        spawnFn({ script, configDir, port: opts.port ?? null });
      }
      const hello = await pollHello(paths.sock, 3000);
      if (!hello) throw new Error("daemon failed to start");
      return hello;
    } finally {
      unlock(configDir);
    }
  };

  let conn = await obtain();
  const action = decideDaemonAction(identity, {
    proto: Number(conn.hello.proto),
    version: String(conn.hello.version ?? ""),
    channel: conn.hello.channel === "npm" ? "npm" : "dev",
    packageRoot: String(conn.hello.packageRoot ?? ""),
    sourceHash: String(conn.hello.sourceHash ?? ""),
  });
  if (action === "reject-proto") {
    conn.socket.destroy();
    throw new Error("daemon protocol unrecognized; update the CLI");
  }
  if (action === "replace") {
    if (supervisorInstalled(configDir)) {
      writeDaemonTarget(configDir, { execPath: process.execPath, script });
    }
    const stopped = await conn.peer.request("daemon.stop", { reason: "replace" }, 5000);
    if (stopped.error === "play-active") {
      return conn;
    }
    conn.socket.destroy();
    const pid = Number(conn.hello.pid);
    if (Number.isFinite(pid) && pid !== process.pid) {
      const start = Date.now();
      while (Date.now() - start < 2000) {
        if (pidLiveness(pid) === "dead") break;
        await delay(30);
      }
    }
    ctx.helloMisses = 0;
    conn = await obtain();
  }
  return conn;
}

/**
 * @param {string} op
 * @param {Record<string, unknown>} fields
 * @param {{ port?: string | null, configDir?: string }} [opts]
 */
export async function daemonRobotRequest(op, fields = {}, opts = {}) {
  const conn = await ensureDaemon({ configDir: opts.configDir, port: opts.port ?? null });
  try {
    const res = await conn.peer.request(
      "robot.request",
      { op, fields, port: opts.port ?? null },
      15_000,
    );
    if (typeof res.error === "string") throw new Error(res.error);
    return /** @type {Record<string, unknown>} */ (res.result && /** @type {{ body?: Record<string, unknown> }} */ (res.result).body) ?? {};
  } finally {
    conn.socket.end();
  }
}

/**
 * @param {{ path: string, name?: string, port?: string | null, configDir?: string }} opts
 */
export async function daemonPlay(opts) {
  const conn = await ensureDaemon({ configDir: opts.configDir, port: opts.port ?? null });
  try {
    const res = await conn.peer.request(
      "play",
      { path: opts.path, name: opts.name ?? "talking", port: opts.port ?? null },
      11 * 60 * 1000,
    );
    if (typeof res.error === "string") throw new Error(res.error);
    return /** @type {Record<string, unknown>} */ (
      /** @type {{ body?: Record<string, unknown> }} */ (res.result)?.body ?? {}
    );
  } finally {
    conn.socket.end();
  }
}

/**
 * @param {{
 *   channel: "serial.events" | "serial.logs",
 *   port?: string | null,
 *   configDir?: string,
 *   onLine: (line: string) => void,
 *   onLinkDown?: () => void,
 *   onSubscribed?: () => void,
 * }} opts
 * @returns {Promise<"stop" | "replace" | "drop">}
 */
export async function subscribeOnce(opts) {
  const conn = await ensureDaemon({ configDir: opts.configDir, port: opts.port ?? null });
  const res = await conn.peer.request(
    "subscribe",
    { channel: opts.channel, port: opts.port ?? null },
    5000,
  );
  if (typeof res.error === "string") {
    conn.socket.destroy();
    throw new Error(res.error);
  }
  opts.onSubscribed?.();
  return new Promise((resolve) => {
    let settled = false;
    const finish = (reason) => {
      if (settled) return;
      settled = true;
      resolve(reason);
    };
    conn.socket.on("close", () => {
      setTimeout(() => finish("drop"), 30);
    });
    conn.socket.on("error", () => {
      setTimeout(() => finish("drop"), 30);
    });
    conn.peer.on((msg) => {
      if (msg.method === "event" && msg.params && /** @type {{ channel?: string }} */ (msg.params).channel === opts.channel) {
        const data = /** @type {{ data?: string }} */ (msg.params).data;
        if (typeof data === "string") opts.onLine(data);
      }
      if (msg.method === "link-down") opts.onLinkDown?.();
      if (msg.method === "bye") {
        const reason = /** @type {{ reason?: string }} */ (msg.params)?.reason;
        finish(reason === "replace" ? "replace" : "stop");
      }
    });
  });
}

/**
 * @param {{
 *   channel: "serial.events" | "serial.logs",
 *   port?: string | null,
 *   configDir?: string,
 *   onLine: (line: string) => void,
 *   onLinkDown?: () => void,
 *   onSubscribed?: () => void,
 *   subscribe?: (opts: {
 *     channel: "serial.events" | "serial.logs",
 *     port?: string | null,
 *     configDir?: string,
 *     onLine: (line: string) => void,
 *     onLinkDown?: () => void,
 *     onSubscribed?: () => void,
 *   }) => Promise<"stop" | "replace" | "drop">,
 * }} opts
 */
export async function followDaemonChannel(opts) {
  const subscribe = opts.subscribe ?? ((inner) => subscribeOnce(inner));
  let drops = 0;
  for (;;) {
    const reason = await subscribe({
      ...opts,
      onSubscribed() {
        drops = 0;
        opts.onSubscribed?.();
      },
    });
    if (reason === "stop") return;
    if (reason === "replace") {
      drops = 0;
      continue;
    }
    const supervised = Boolean(opts.configDir && supervisorInstalled(opts.configDir));
    if (supervised) {
      drops += 1;
      if (drops > 40) throw new Error("daemon connection lost");
      await delay(30);
      continue;
    }
    drops += 1;
    if (drops > 1) throw new Error("daemon connection lost");
  }
}

/**
 * @param {string} configDir
 */
function installStatusLine(configDir) {
  return `install: ${supervisorInstalled(configDir) ? "on" : "off"}`;
}

/**
 * @param {string} configDir
 * @param {number} [now]
 */
function yieldStatusLine(configDir, now = Date.now()) {
  try {
    const until = JSON.parse(readFileSync(daemonPaths(configDir).yieldFile, "utf8")).until;
    if (typeof until !== "number" || until <= now) return "yield: off";
    const left = Math.ceil((until - now) / 1000);
    return `yield: ${left}s`;
  } catch {
    return "yield: off";
  }
}

/**
 * macOS lists `tty.*` while the daemon may hold the matching `cu.*` node.
 * @param {string} path
 */
function serialPortKey(path) {
  return path.replace(/^\/dev\/(?:tty|cu)\./, "");
}

/**
 * @param {string | null} openPort
 * @param {Array<{ path?: string }> | null} listed null when enumeration failed
 * @returns {string[]}
 */
function portStatusLines(openPort, listed) {
  const open = openPort && openPort !== "down" ? openPort : null;
  /** @type {string[]} */
  const lines = [`port: ${open ?? "down"}`];
  if (!listed) {
    lines.push("ports: unavailable");
    return lines;
  }
  const paths = listed.map((entry) => entry.path).filter((path) => typeof path === "string" && path);
  if (paths.length === 0) {
    lines.push("ports: none");
    return lines;
  }
  const openKey = open ? serialPortKey(open) : null;
  lines.push("ports:");
  for (const path of paths) {
    const mark = openKey && serialPortKey(path) === openKey ? " connected" : "";
    lines.push(`  ${path}${mark}`);
  }
  return lines;
}

/**
 * @param {{
 *   configDir?: string,
 *   now?: number,
 *   listPorts?: () => Promise<Array<{ path?: string }>>,
 * }} [opts]
 */
export async function daemonStatusText(opts = {}) {
  const configDir = opts.configDir ?? userConfigDir();
  const install = installStatusLine(configDir);
  const yieldLine = yieldStatusLine(configDir, opts.now ?? Date.now());
  const listPorts = opts.listPorts ?? (() => SerialPort.list());
  /** @type {ReturnType<typeof setTimeout> | undefined} */
  let timer;
  /** @type {Array<{ path?: string }> | null} */
  const listed = await Promise.race([
    (async () => listPorts())().catch(() => null),
    new Promise((resolve) => {
      timer = setTimeout(() => resolve(null), 1500);
    }),
  ]).finally(() => clearTimeout(timer));
  const conn = await tryConnect(daemonPaths(configDir).sock, 300);
  if (!conn) {
    return [`daemon is not running`, install, yieldLine, ...portStatusLines(null, listed), ""].join("\n");
  }
  try {
    const res = await conn.peer.request("daemon.status", {}, 1000);
    const body = /** @type {Record<string, unknown>} */ (res.result ?? {});
    const port = body.port ? String(body.port) : null;
    return [
      "daemon running",
      install,
      yieldLine,
      `pid: ${body.pid}`,
      `version: ${body.version}`,
      `channel: ${body.channel}`,
      `packageRoot: ${body.packageRoot}`,
      `proto: ${body.proto}`,
      ...portStatusLines(port, listed),
      "",
    ].join("\n");
  } finally {
    conn.socket.end();
  }
}

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
 * `daemon stop` without yield is a port release. `--yield` also refuses open.
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
