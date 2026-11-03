import { chmodSync, mkdirSync, rmSync, writeFileSync } from "node:fs";
import { createServer } from "node:net";

import { connectSerialRobot } from "../robot/serial/client.js";
import { createSerialLink } from "./link.js";
import { appendLog } from "./log.js";
import { daemonPaths } from "./paths.js";
import { loadIdentity, packageRootFromScript, resolveCliScript } from "./policy.js";
import { createPeer } from "./ipc.js";

/**
 * @param {import("node:net").Socket} socket
 * @param {Record<string, unknown>} msg
 */
function send(socket, msg) {
  if (socket.destroyed) return;
  socket.write(`${JSON.stringify(msg)}\n`);
}

/**
 * Long-lived host daemon. Resolves when the process is asked to stop
 * unless `exitProcess` is set, in which case it exits.
 * @param {{
 *   configDir: string,
 *   preferredPort?: string | null,
 *   connect?: typeof connectSerialRobot,
 *   identity?: ReturnType<typeof loadIdentity>,
 *   exitProcess?: boolean,
 * }} opts
 */
export async function serveDaemon(opts) {
  const paths = daemonPaths(opts.configDir);
  const exitProcess = opts.exitProcess === true;
  const identity =
    opts.identity ?? loadIdentity(packageRootFromScript(resolveCliScript()));
  const link = createSerialLink({
    connect: opts.connect ?? ((connectOpts) => connectSerialRobot(connectOpts)),
    preferredPort: opts.preferredPort ?? null,
  });
  mkdirSync(paths.dir, { recursive: true, mode: 0o700 });

  /** @type {Set<import("node:net").Socket>} */
  const sockets = new Set();
  let shuttingDown = false;
  /** @type {(() => void) | null} */
  let finish = null;
  const done = new Promise((resolve) => {
    finish = resolve;
  });

  const writeState = () => {
    writeFileSync(
      paths.state,
      JSON.stringify({
        pid: process.pid,
        proto: identity.proto,
        version: identity.version,
        channel: identity.channel,
        packageRoot: identity.packageRoot,
        sourceHash: identity.sourceHash,
        port: link.port,
      }),
      { mode: 0o600 },
    );
  };

  const helloParams = () => ({
    proto: identity.proto,
    version: identity.version,
    channel: identity.channel,
    packageRoot: identity.packageRoot,
    sourceHash: identity.sourceHash,
    pid: process.pid,
    port: link.port,
  });

  /**
   * @param {"stop" | "replace"} reason
   */
  const shutdown = async (reason) => {
    if (shuttingDown) return;
    shuttingDown = true;
    const bye = { method: "bye", params: { reason } };
    for (const socket of sockets) {
      send(socket, bye);
      socket.end();
    }
    try {
      await link.close();
    } catch {
      // port already gone
    }
    writeState();
    await new Promise((resolve) => server.close(() => resolve(undefined)));
    // server.close unlinks a unix socket before this callback. A replacement
    // daemon may already own the path, so do not rmSync it here.
    process.off("SIGHUP", onHup);
    process.off("SIGTERM", onTerm);
    process.off("SIGINT", onTerm);
    appendLog(paths.log, `daemon exit reason=${reason}`);
    if (exitProcess) process.exit(0);
    finish?.();
  };

  const onHup = () => {};
  const onTerm = () => {
    void shutdown("stop");
  };
  process.on("SIGHUP", onHup);
  process.on("SIGTERM", onTerm);
  process.on("SIGINT", onTerm);

  const server = createServer((socket) => {
    sockets.add(socket);
    socket.on("close", () => sockets.delete(socket));
    socket.on("error", () => socket.destroy());
    send(socket, { method: "hello", params: helloParams() });
    const peer = createPeer(socket);
    peer.on((msg) => {
      if (msg.id == null || typeof msg.method !== "string") return;
      void handle(socket, /** @type {number} */ (msg.id), msg.method, msg.params ?? {});
    });
  });

  /**
   * @param {import("node:net").Socket} socket
   * @param {number} id
   * @param {string} method
   * @param {Record<string, unknown>} params
   */
  async function handle(socket, id, method, params) {
    try {
      if (method === "daemon.release") {
        const force = params.force === true;
        const released = await link.release(force);
        if (!released) {
          send(socket, { id, error: "play-active" });
          return;
        }
        writeState();
        send(socket, { id, result: { ok: true, port: link.port } });
        return;
      }
      if (method === "daemon.stop") {
        const reason = params.reason === "replace" ? "replace" : "stop";
        if (reason === "replace" && link.playActive) {
          send(socket, { id, error: "play-active" });
          return;
        }
        send(socket, { id, result: { ok: true } });
        await shutdown(reason);
        return;
      }
      if (method === "daemon.status") {
        send(socket, { id, result: { ok: true, ...helloParams(), playActive: link.playActive } });
        return;
      }
      if (method === "robot.request") {
        const port = typeof params.port === "string" ? params.port : null;
        const body = await link.request(
          String(params.op ?? ""),
          /** @type {Record<string, unknown>} */ (params.fields ?? {}),
          port,
        );
        writeState();
        send(socket, { id, result: { ok: true, body } });
        return;
      }
      if (method === "play") {
        const port = typeof params.port === "string" ? params.port : null;
        const body = await link.play(String(params.path ?? ""), String(params.name ?? "talking"), port);
        writeState();
        send(socket, { id, result: { ok: true, body } });
        return;
      }
      if (method === "subscribe") {
        const channel = params.channel === "serial.logs" ? "serial.logs" : "serial.events";
        const port = typeof params.port === "string" ? params.port : null;
        await link.ensure(port);
        writeState();
        const unsub = link.subscribe(channel, (line) => {
          send(socket, { method: "event", params: { channel, data: line } });
        });
        const unsubDown = channel === "serial.events"
          ? link.onLinkDown(() => {
            send(socket, { method: "link-down" });
          })
          : () => {};
        socket.on("close", () => {
          unsub();
          unsubDown();
        });
        send(socket, { id, result: { ok: true, port: link.port } });
        return;
      }
      send(socket, { id, error: `unknown method ${method}` });
    } catch (err) {
      const message = err instanceof Error ? err.message : String(err);
      appendLog(paths.log, message);
      send(socket, { id, error: message });
    }
  }

  if (process.platform !== "win32") rmSync(paths.sock, { force: true });
  await new Promise((resolve, reject) => {
    server.once("error", (err) => {
      if (/** @type {NodeJS.ErrnoException} */ (err).code === "EADDRINUSE") {
        appendLog(paths.log, "daemon socket busy");
        if (exitProcess) process.exit(0);
        reject(err);
        return;
      }
      reject(err);
    });
    server.listen(paths.sock, () => {
      if (process.platform !== "win32") chmodSync(paths.sock, 0o600);
      writeState();
      appendLog(paths.log, `daemon listening pid=${process.pid}`);
      resolve(undefined);
    });
  });

  await done;
}
