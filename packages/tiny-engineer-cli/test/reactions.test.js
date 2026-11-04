import assert from "node:assert/strict";
import { existsSync, mkdirSync, readFileSync, rmSync, statSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";

import { daemonHelp } from "../src/daemon/commands.js";
import { daemonRobotRequest, daemonStatusText, disableDaemon, releaseDaemon } from "../src/daemon/client.js";
import { daemonPaths } from "../src/daemon/paths.js";
import { delay } from "../src/daemon/proc.js";
import { serveDaemon } from "../src/daemon/server.js";
import { reactionsSuspended } from "../src/daemon/reactions.js";

function freshDir() {
  const root = existsSync("/tmp") ? "/tmp" : tmpdir();
  const dir = join(root, `te-react-${process.pid}-${Math.random().toString(16).slice(2)}`);
  mkdirSync(dir);
  return dir;
}

/**
 * @param {string} port
 * @param {{ closeDelayMs?: number }} [opts]
 */
function fakeClient(port, { closeDelayMs = 0 } = {}) {
  /** @type {(payload: Buffer) => void} */
  let onEvent = () => {};
  /** @type {(() => void) | null} */
  let onClose = null;
  return {
    port,
    armed: false,
    request: async () => ({ ok: true }),
    play: async () => ({ ok: true }),
    /**
     * @param {(payload: Buffer) => void} fn
     */
    setEventHandler(fn) {
      onEvent = fn;
    },
    setLogHandler() {},
    /**
     * @param {() => void} fn
     */
    setCloseHandler(fn) {
      onClose = fn;
      this.armed = true;
    },
    close: async () => {
      if (closeDelayMs > 0) await delay(closeDelayMs);
    },
    drop() {
      onClose?.();
    },
    /**
     * @param {string} line
     */
    emit(line) {
      onEvent(Buffer.from(line));
    },
  };
}

/**
 * @param {string} dir
 * @param {string} body
 */
function writeEnv(dir, body) {
  writeFileSync(join(dir, "env"), body, { mode: 0o600 });
}

/**
 * @param {string} dir
 * @param {unknown} doc
 */
function writeRules(dir, doc) {
  writeFileSync(daemonPaths(dir).rules, JSON.stringify(doc), { mode: 0o600 });
}

const click = { v: 1, type: "button.click", source: "push-button", ms: 1 };

test("daemon help mentions armed serial open and the suspend latch", () => {
  const help = daemonHelp();
  assert.match(help, /serial reactions are armed/);
  assert.match(help, /Suspends serial reaction auto-open/);
  assert.match(help, /HTTP reactions stay up/);
});

test("missing rules leave the port closed until a serial command", async () => {
  const dir = freshDir();
  let opens = 0;
  const serving = serveDaemon({
    configDir: dir,
    exitProcess: false,
    connect: async () => {
      opens += 1;
      return { port: "/dev/fake", client: fakeClient("/dev/fake") };
    },
  });
  try {
    await delay(80);
    assert.equal(opens, 0);
    const status = await daemonStatusText({ configDir: dir, listPorts: async () => [] });
    assert.match(status, /daemon running/);
    assert.match(status, /port: down/);
    assert.match(status, /reaction: off/);
    assert.match(status, /rules: 0 ok/);
    assert.match(status, /last: none/);
    await daemonRobotRequest("health", {}, { configDir: dir });
    assert.equal(opens, 1);
  } finally {
    await disableDaemon({ configDir: dir });
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("stopped status does not require a live daemon", async () => {
  const dir = freshDir();
  const text = await daemonStatusText({ configDir: dir, listPorts: async () => [] });
  assert.match(text, /daemon is not running/);
  rmSync(dir, { recursive: true, force: true });
});

test("serial reactions open on start and one event starts each rule once", async () => {
  const dir = freshDir();
  const hits = join(dir, "hits");
  writeEnv(dir, "TINY_ENGINEER_TRANSPORT=serial\n");
  const script = `require('fs').appendFileSync(${JSON.stringify(hits)}, 'hit\\n')`;
  writeRules(dir, {
    v: 1,
    rules: [
      { on: "button.click", run: [process.execPath, "-e", script] },
      { on: "button.click", run: [process.execPath, "-e", script] },
    ],
  });
  let opens = 0;
  /** @type {ReturnType<typeof fakeClient> | null} */
  let client = null;
  const serving = serveDaemon({
    configDir: dir,
    exitProcess: false,
    retryMs: 50,
    connect: async () => {
      opens += 1;
      client = fakeClient("/dev/fake");
      return { port: "/dev/fake", client };
    },
  });
  try {
    const start = Date.now();
    while (opens < 1 && Date.now() - start < 2000) await delay(20);
    assert.equal(opens, 1);
    client?.emit(JSON.stringify(click));
    const wait = Date.now();
    while ((!existsSync(hits) || readFileSync(hits, "utf8").split("\n").filter(Boolean).length < 2)
      && Date.now() - wait < 2000) {
      await delay(20);
    }
    assert.equal(readFileSync(hits, "utf8").trim().split("\n").length, 2);
    const status = await daemonStatusText({ configDir: dir, listPorts: async () => [] });
    assert.match(status, /reaction: up \/dev\/fake/);
    assert.match(status, /rules: 2 ok/);
  } finally {
    await disableDaemon({ configDir: dir });
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("release suspends serial reopen until a command, and yield blocks open", async () => {
  const dir = freshDir();
  writeEnv(dir, "TINY_ENGINEER_TRANSPORT=serial\n");
  writeRules(dir, { v: 1, rules: [{ on: "button.click", run: ["/bin/true"] }] });
  let opens = 0;
  const serving = serveDaemon({
    configDir: dir,
    exitProcess: false,
    retryMs: 40,
    connect: async () => {
      opens += 1;
      return { port: "/dev/fake", client: fakeClient("/dev/fake", { closeDelayMs: 80 }) };
    },
  });
  try {
    const start = Date.now();
    while (opens < 1 && Date.now() - start < 2000) await delay(20);
    assert.equal(opens, 1);
    await releaseDaemon({ configDir: dir });
    assert.equal(reactionsSuspended(dir), true);
    assert.equal(statSync(daemonPaths(dir).reactionsSuspended).mode & 0o777, 0o600);
    await delay(120);
    assert.equal(opens, 1);
    await daemonRobotRequest("health", {}, { configDir: dir });
    assert.equal(opens, 2);
    assert.equal(reactionsSuspended(dir), false);
  } finally {
    await disableDaemon({ configDir: dir });
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("serial link drop reopens the port for armed reactions", async () => {
  const dir = freshDir();
  writeEnv(dir, "TINY_ENGINEER_TRANSPORT=serial\n");
  writeRules(dir, { v: 1, rules: [{ on: "button.click", run: ["/bin/true"] }] });
  let opens = 0;
  /** @type {ReturnType<typeof fakeClient> | null} */
  let client = null;
  const serving = serveDaemon({
    configDir: dir,
    exitProcess: false,
    retryMs: 40,
    connect: async () => {
      opens += 1;
      client = fakeClient("/dev/fake");
      return { port: "/dev/fake", client };
    },
  });
  try {
    const start = Date.now();
    while ((!client?.armed) && Date.now() - start < 2000) await delay(20);
    assert.equal(opens, 1);
    assert.equal(client?.armed, true);
    client?.drop();
    const again = Date.now();
    while (opens < 2 && Date.now() - again < 2000) await delay(20);
    assert.equal(opens, 2);
    const status = await daemonStatusText({ configDir: dir, listPorts: async () => [] });
    assert.match(status, /reaction: up \/dev\/fake/);
  } finally {
    await disableDaemon({ configDir: dir });
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("an active yield does not open the port for reactions", async () => {
  const dir = freshDir();
  writeEnv(dir, "TINY_ENGINEER_TRANSPORT=serial\n");
  writeRules(dir, { v: 1, rules: [{ on: "button.click", run: ["/bin/true"] }] });
  writeFileSync(daemonPaths(dir).yieldFile, JSON.stringify({ until: Date.now() + 60_000 }), { mode: 0o600 });
  let opens = 0;
  const serving = serveDaemon({
    configDir: dir,
    exitProcess: false,
    retryMs: 40,
    connect: async () => {
      opens += 1;
      return { port: "/dev/fake", client: fakeClient("/dev/fake") };
    },
  });
  try {
    await delay(120);
    assert.equal(opens, 0);
    const status = await daemonStatusText({ configDir: dir, listPorts: async () => [] });
    assert.match(status, /reaction: suspended/);
  } finally {
    await disableDaemon({ configDir: dir });
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("http reactions ignore serial events and release", async () => {
  const dir = freshDir();
  const hits = join(dir, "hits");
  writeEnv(dir, "TINY_ENGINEER_TRANSPORT=http\nTINY_ENGINEER_URL=http://robot.local\nTINY_ENGINEER_TOKEN=secret-token\n");
  const script = `require('fs').appendFileSync(${JSON.stringify(hits)}, 'hit\\n')`;
  writeRules(dir, { v: 1, rules: [{ on: "button.click", run: [process.execPath, "-e", script] }] });
  /** @type {ReturnType<typeof fakeSocket> | null} */
  let socket = null;
  let opens = 0;
  /** @type {ReturnType<typeof fakeClient> | null} */
  let client = null;
  /**
   * @param {string} url
   */
  function fakeSocket(url) {
    /** @type {Record<string, Function[]>} */
    const handlers = {};
    const sock = {
      url,
      closed: false,
      /**
       * @param {string} ev
       * @param {Function} fn
       */
      on(ev, fn) {
        (handlers[ev] ??= []).push(fn);
        if (ev === "open") fn();
      },
      /**
       * @param {string} ev
       * @param {unknown} [data]
       */
      emit(ev, data) {
        for (const fn of handlers[ev] ?? []) fn(data);
      },
      close() {
        this.closed = true;
        this.emit("close");
      },
      terminate() {
        this.closed = true;
      },
    };
    return sock;
  }
  const serving = serveDaemon({
    configDir: dir,
    exitProcess: false,
    retryMs: 10_000,
    connect: async () => {
      opens += 1;
      client = fakeClient("/dev/fake");
      return { port: "/dev/fake", client };
    },
    openWebSocket: async (url) => {
      socket = fakeSocket(url);
      return socket;
    },
  });
  try {
    const start = Date.now();
    while (!socket && Date.now() - start < 2000) await delay(20);
    assert.ok(socket);
    assert.match(socket.url, /token=secret-token/);
    await daemonRobotRequest("health", {}, { configDir: dir });
    assert.equal(opens, 1);
    client?.emit(JSON.stringify(click));
    socket.emit("message", JSON.stringify(click));
    const wait = Date.now();
    while ((!existsSync(hits) || readFileSync(hits, "utf8").trim() !== "hit") && Date.now() - wait < 2000) {
      await delay(20);
    }
    assert.equal(readFileSync(hits, "utf8").trim(), "hit");
    await delay(50);
    assert.equal(readFileSync(hits, "utf8").trim(), "hit");
    await releaseDaemon({ configDir: dir });
    assert.equal(socket.closed, false);
    const status = await daemonStatusText({ configDir: dir, listPorts: async () => [] });
    assert.equal(status.includes("secret-token"), false);
    assert.match(status, /reaction: up/);
    const log = readFileSync(daemonPaths(dir).log, "utf8");
    assert.equal(log.includes("secret-token"), false);
  } finally {
    await disableDaemon({ configDir: dir });
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("https base does not open a socket", async () => {
  const dir = freshDir();
  writeEnv(dir, "TINY_ENGINEER_TRANSPORT=http\nTINY_ENGINEER_URL=https://robot.local\nTINY_ENGINEER_TOKEN=secret-token\n");
  writeRules(dir, { v: 1, rules: [{ on: "button.click", run: ["/bin/true"] }] });
  let opened = 0;
  const serving = serveDaemon({
    configDir: dir,
    exitProcess: false,
    openWebSocket: async () => {
      opened += 1;
      throw new Error("should not open");
    },
  });
  try {
    const start = Date.now();
    let status = "";
    while (!status.includes("reaction: error") && Date.now() - start < 2000) {
      status = await daemonStatusText({ configDir: dir, listPorts: async () => [] });
      await delay(20);
    }
    assert.match(status, /reaction: error/);
    assert.equal(opened, 0);
    assert.equal(status.includes("secret-token"), false);
    assert.equal(readFileSync(daemonPaths(dir).log, "utf8").includes("secret-token"), false);
  } finally {
    await disableDaemon({ configDir: dir });
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("reload keeps an open serial port and an invalid file keeps the previous rules", async () => {
  const dir = freshDir();
  writeEnv(dir, "TINY_ENGINEER_TRANSPORT=serial\n");
  writeRules(dir, { v: 1, rules: [{ on: "button.click", run: ["/bin/true"] }] });
  let opens = 0;
  /** @type {(() => Promise<void>) | null} */
  let reload = null;
  const serving = serveDaemon({
    configDir: dir,
    exitProcess: false,
    connect: async () => {
      opens += 1;
      return { port: "/dev/fake", client: fakeClient("/dev/fake") };
    },
    controls: (api) => {
      reload = api.reload;
    },
  });
  try {
    const start = Date.now();
    while (opens < 1 && Date.now() - start < 2000) await delay(20);
    writeRules(dir, {
      v: 1,
      rules: [
        { on: "button.click", run: ["/bin/true"] },
        { on: "button.up", run: ["/bin/true"] },
      ],
    });
    await reload?.();
    assert.equal(opens, 1);
    let status = await daemonStatusText({ configDir: dir, listPorts: async () => [] });
    assert.match(status, /rules: 2 ok/);
    writeFileSync(daemonPaths(dir).rules, "{");
    await reload?.();
    status = await daemonStatusText({ configDir: dir, listPorts: async () => [] });
    assert.match(status, /daemon running/);
    assert.match(status, /rules: 2 file error/);
    assert.equal(opens, 1);
  } finally {
    await disableDaemon({ configDir: dir });
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("transport switch closes the socket before opening serial", async () => {
  const dir = freshDir();
  writeEnv(dir, "TINY_ENGINEER_TRANSPORT=http\nTINY_ENGINEER_URL=http://robot.local\n");
  writeRules(dir, { v: 1, rules: [{ on: "button.click", run: ["/bin/true"] }] });
  /** @type {{ closed: boolean } | null} */
  let socket = null;
  let serialAt = 0;
  let closedBeforeSerial = false;
  /** @type {(() => Promise<void>) | null} */
  let reload = null;
  const serving = serveDaemon({
    configDir: dir,
    exitProcess: false,
    retryMs: 10_000,
    controls: (api) => {
      reload = api.reload;
    },
    connect: async () => {
      serialAt = Date.now();
      closedBeforeSerial = Boolean(socket?.closed);
      return { port: "/dev/fake", client: fakeClient("/dev/fake") };
    },
    openWebSocket: async () => {
      /** @type {Record<string, Function[]>} */
      const handlers = {};
      const sock = {
        closed: false,
        /**
         * @param {string} ev
         * @param {Function} fn
         */
        on(ev, fn) {
          (handlers[ev] ??= []).push(fn);
          if (ev === "open") fn();
        },
        /**
         * @param {string} ev
         */
        emit(ev) {
          for (const fn of handlers[ev] ?? []) fn();
        },
        close() {
          this.closed = true;
          this.emit("close");
        },
        terminate() {
          this.closed = true;
        },
      };
      socket = sock;
      return sock;
    },
  });
  try {
    const start = Date.now();
    while (!socket && Date.now() - start < 2000) await delay(20);
    writeEnv(dir, "TINY_ENGINEER_TRANSPORT=serial\n");
    await reload?.();
    const wait = Date.now();
    while (serialAt === 0 && Date.now() - wait < 2000) await delay(20);
    assert.equal(closedBeforeSerial, true);
    assert.ok(serialAt > 0);
  } finally {
    await disableDaemon({ configDir: dir });
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("a reaction child does not block anim and SIGTERM kills it", async () => {
  const dir = freshDir();
  const pidFile = join(dir, "child.pid");
  writeEnv(dir, "TINY_ENGINEER_TRANSPORT=serial\n");
  const script = `require('fs').writeFileSync(${JSON.stringify(pidFile)}, String(process.pid)); setInterval(() => {}, 1000)`;
  writeRules(dir, { v: 1, rules: [{ on: "button.click", run: [process.execPath, "-e", script] }] });
  /** @type {ReturnType<typeof fakeClient> | null} */
  let client = null;
  const serving = serveDaemon({
    configDir: dir,
    exitProcess: false,
    connect: async () => {
      client = fakeClient("/dev/fake");
      return { port: "/dev/fake", client };
    },
  });
  try {
    const start = Date.now();
    while (!client && Date.now() - start < 2000) await delay(20);
    client?.emit(JSON.stringify(click));
    const wait = Date.now();
    while (!existsSync(pidFile) && Date.now() - wait < 2000) await delay(20);
    const pid = Number(readFileSync(pidFile, "utf8"));
    assert.ok(pid > 0);
    const healthStarted = Date.now();
    await daemonRobotRequest("health", {}, { configDir: dir });
    assert.ok(Date.now() - healthStarted < 1500);
    process.kill(pid, 0);
    await disableDaemon({ configDir: dir });
    const deadWait = Date.now();
    let dead = false;
    while (Date.now() - deadWait < 2000) {
      try {
        process.kill(pid, 0);
        await delay(20);
      } catch {
        dead = true;
        break;
      }
    }
    assert.equal(dead, true);
  } finally {
    await disableDaemon({ configDir: dir }).catch(() => {});
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});
