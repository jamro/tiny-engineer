import { userConfigDir } from "../config/paths.js";
import {
  clearStale,
  pollHello,
  readState,
  spawnDaemonProcess,
  startWrapperProcess,
  tryConnect,
  tryLock,
  unlock,
} from "./connect.js";
import { disableActive, supervisorInstalled, yieldActive } from "./flags.js";
import {
  decideDaemonAction,
  loadIdentity,
  packageRootFromScript,
  resolveCliScript,
  DISABLE_ERROR,
  YIELD_ERROR,
} from "./policy.js";
import { classifyPid, delay, pidLiveness, terminatePid } from "./proc.js";
import { daemonPaths } from "./paths.js";
import { writeDaemonTarget } from "./target.js";

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
   * @returns {Promise<{ socket: import("node:net").Socket, hello: Record<string, unknown>, peer: ReturnType<typeof import("./ipc.js").createPeer> }>}
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
        throw new Error(
          `cannot identify process ${state?.pid}; daemon release only frees the port — check daemon status or end that process yourself`,
        );
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
    if (typeof stopped.error === "string") {
      conn.socket.destroy();
      throw new Error(stopped.error);
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
