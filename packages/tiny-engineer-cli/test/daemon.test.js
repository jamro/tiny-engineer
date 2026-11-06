import assert from "node:assert/strict";
import { spawn, spawnSync } from "node:child_process";
import { existsSync, mkdirSync, mkdtempSync, readFileSync, rmSync, statSync, utimesSync, writeFileSync } from "node:fs";
import { createServer } from "node:net";
import { tmpdir } from "node:os";
import { join, resolve } from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

import { daemonHelp, parseDaemonArgs, runDaemon } from "../src/daemon/commands.js";
import {
  daemonPlay,
  daemonProbe,
  daemonRobotRequest,
  daemonServeArgs,
  daemonStatusText,
  disableActive,
  disableDaemon,
  ensureDaemon,
  followDaemonChannel,
  releaseDaemon,
  subscribeOnce,
  tryConnect,
  clearYield,
  yieldActive,
} from "../src/daemon/client.js";
import { createSerialLink } from "../src/daemon/link.js";
import { appendLog } from "../src/daemon/log.js";
import { daemonIpcPath, daemonPaths } from "../src/daemon/paths.js";
import { delay, readProcessCommand, terminatePid } from "../src/daemon/proc.js";
import { loadIdentity } from "../src/daemon/policy.js";
import { serveDaemon } from "../src/daemon/server.js";
import { createOutput } from "../src/util/output.js";
import { robotPlay, robotSetAnim } from "../src/robot/transport.js";
import { SerialRobotClient } from "../src/robot/serial/client.js";

const cli = fileURLToPath(new URL("../bin/tiny-engineer.js", import.meta.url));
const packageRoot = fileURLToPath(new URL("..", import.meta.url));

function tempConfig() {
  const root = existsSync("/tmp") ? "/tmp" : tmpdir();
  return mkdtempSync(join(root, "te-daemon-"));
}

function quietOut() {
  return createOutput({
    quiet: true,
    log: () => {},
    error: () => {},
  });
}

/**
 * @param {string} configDir
 * @param {() => Promise<{ port: string, client: Record<string, unknown> }>} connect
 */
function startServer(configDir, connect, identity) {
  const serving = serveDaemon({
    configDir,
    exitProcess: false,
    connect,
    identity,
  });
  return serving;
}

async function waitForStatus(configDir) {
  const start = Date.now();
  while (Date.now() - start < 3000) {
    const text = await daemonStatusText({ configDir });
    if (text.includes("daemon running")) return text;
    await delay(20);
  }
  throw new Error("daemon did not start");
}

test("appendLog truncates at the cap", () => {
  const dir = tempConfig();
  try {
    const path = join(dir, "daemon.log");
    appendLog(path, "aaaa", 4);
    appendLog(path, "bbbb", 4);
    const text = readFileSync(path, "utf8");
    assert.equal(text, "bbbb\n");
    assert.ok(statSync(path).size < 20);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("terminatePid sends SIGTERM then SIGKILL", async () => {
  /** @type {string[]} */
  const signals = [];
  let alive = true;
  await terminatePid(42, {
    signal: (_pid, sig) => {
      signals.push(String(sig));
      if (sig === "SIGKILL") alive = false;
    },
    alive: () => alive,
    waitMs: 30,
  });
  assert.deepEqual(signals, ["SIGTERM", "SIGKILL"]);
});

test("daemon help lists release, disable, install, and plain stop stays up", () => {
  const help = daemonHelp();
  assert.match(help, /release \[--yield\]/);
  assert.match(help, /disable/);
  assert.match(help, /install/);
  assert.match(help, /uninstall/);
  assert.match(help, /Deprecated alias of release/);
  assert.match(help, /--yield/);
  assert.match(help, /ignored while daemon install is on/);
  assert.deepEqual(daemonServeArgs("/bin/tiny-engineer.js", "/dev/b"), [
    "/bin/tiny-engineer.js",
    "daemon",
    "--serve",
    "--port",
    "/dev/b",
  ]);
  const cliHelp = spawnSync(process.execPath, [cli, "--help"], { encoding: "utf8" });
  assert.equal(cliHelp.status, 0);
  assert.match(cliHelp.stdout, /release/);
  assert.match(cliHelp.stdout, /disable/);
  assert.match(cliHelp.stdout, /release closes USB and stays up/);
  assert.match(cliHelp.stdout, /stop is a deprecated alias of release/);
  const daemonCli = spawnSync(process.execPath, [cli, "daemon", "--help"], { encoding: "utf8" });
  assert.equal(daemonCli.status, 0);
  assert.match(daemonCli.stdout, /release \[--yield\]/);
  assert.match(daemonCli.stdout, /disable/);
  assert.match(daemonCli.stdout, /Deprecated alias of release/);
  assert.equal(parseDaemonArgs(["stop", "--yield"]).yield, true);
  assert.equal(parseDaemonArgs(["release", "--yield"]).yield, true);
});

test("stop deprecates toward release and release --yield sets yield", async () => {
  const dir = tempConfig();
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = dir;
  /** @type {string[]} */
  const stderr = [];
  const origWrite = process.stderr.write.bind(process.stderr);
  process.stderr.write = /** @type {typeof process.stderr.write} */ ((chunk, ...rest) => {
    stderr.push(String(chunk));
    return origWrite(chunk, ...rest);
  });
  try {
    const exitBefore = process.exitCode;
    await runDaemon(quietOut(), ["stop"]);
    assert.equal(process.exitCode ?? 0, 0);
    assert.match(stderr.join(""), /prefer tiny-engineer daemon release/);
    assert.equal(yieldActive(dir), false);
    stderr.length = 0;
    await runDaemon(quietOut(), ["release", "--yield"]);
    assert.equal(process.exitCode ?? 0, 0);
    assert.equal(yieldActive(dir), true);
    assert.doesNotMatch(stderr.join(""), /deprecated/);
    stderr.length = 0;
    clearYield(dir);
    await runDaemon(quietOut(), ["stop", "--yield"]);
    assert.equal(process.exitCode ?? 0, 0);
    assert.equal(yieldActive(dir), true);
    assert.match(stderr.join(""), /prefer tiny-engineer daemon release --yield/);
    process.exitCode = exitBefore;
  } finally {
    process.stderr.write = origWrite;
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("stop is idempotent and --yield blocks auto-start until start", async () => {
  const dir = tempConfig();
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = dir;
  try {
    await runDaemon(quietOut(), ["stop"]);
    assert.equal(process.exitCode ?? 0, 0);
    assert.equal(yieldActive(dir), false);
    await runDaemon(quietOut(), ["stop", "--yield"]);
    assert.equal(yieldActive(dir), true);
    let spawns = 0;
    await assert.rejects(
      () => ensureDaemon({ configDir: dir, spawnFn: () => { spawns += 1; } }),
      /yielded for upload/,
    );
    assert.equal(spawns, 0);
    const exitCode = process.exitCode;
    await runDaemon(quietOut(), ["start"]);
    assert.equal(process.exitCode ?? 0, 0);
    process.exitCode = exitCode;
    assert.equal(yieldActive(dir), false);
    const status = await daemonStatusText({ configDir: dir });
    assert.match(status, /daemon running/);
  } finally {
    spawnSync(process.execPath, [cli, "daemon", "disable"], {
      env: { ...process.env, TINY_ENGINEER_CONFIG_DIR: dir },
      stdio: "ignore",
    });
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("stop without yield keeps the pid and the next request reopens the port", async () => {
  const dir = tempConfig();
  let opens = 0;
  const serving = startServer(dir, async () => {
    opens += 1;
    return fakeSession("/dev/fake");
  });
  try {
    await daemonRobotRequest("health", {}, { configDir: dir });
    const before = await daemonStatusText({ configDir: dir });
    const pid = before.match(/pid: (\d+)/)?.[1];
    assert.ok(pid);
    await releaseDaemon({ configDir: dir });
    const mid = await daemonStatusText({ configDir: dir });
    assert.match(mid, new RegExp(`pid: ${pid}`));
    assert.match(mid, /port: down/);
    assert.equal(opens, 1);
    await daemonRobotRequest("health", {}, { configDir: dir });
    assert.equal(opens, 2);
    assert.match(await daemonStatusText({ configDir: dir }), new RegExp(`pid: ${pid}`));
  } finally {
    await disableDaemon({ configDir: dir });
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("release during play leaves the port open", async () => {
  const dir = tempConfig();
  const wav = join(dir, "clip.wav");
  /** @type {(value: Record<string, unknown>) => void} */
  let finish = () => {};
  /** @type {Promise<void> | undefined} */
  let serving;
  /** @type {Promise<unknown> | undefined} */
  let playing;
  try {
    mkdirSync(dir, { recursive: true });
    writeFileSync(wav, Buffer.alloc(4));
    /** @type {() => void} */
    let markStarted = () => {};
    const started = new Promise((resolve) => {
      markStarted = resolve;
    });
    serving = startServer(dir, async () => ({
      port: "/dev/fake",
      client: {
        request: async () => ({ ok: true }),
        play: () => {
          markStarted();
          return new Promise((resolve) => { finish = resolve; });
        },
        setEventHandler() {},
        setLogHandler() {},
        setCloseHandler() {},
        close: async () => {},
      },
    }));
    playing = daemonPlay({ path: wav, configDir: dir });
    const startedInTime = await Promise.race([
      started.then(() => true),
      delay(2000).then(() => false),
    ]);
    assert.equal(startedInTime, true);
    await assert.rejects(() => releaseDaemon({ configDir: dir }), /play-active/);
    assert.match(await daemonStatusText({ configDir: dir }), /port: \/dev\/fake/);
  } finally {
    const played = playing?.catch(() => {});
    finish({ ok: true });
    await disableDaemon({ configDir: dir }).catch(() => {});
    await played;
    if (serving) await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("yield keeps the daemon and expiry does not reopen for a subscriber", async () => {
  const dir = tempConfig();
  let opens = 0;
  const serving = startServer(dir, async () => {
    opens += 1;
    return fakeSession("/dev/fake");
  });
  try {
    await daemonRobotRequest("health", {}, { configDir: dir });
    const sub = await tryConnect(daemonPaths(dir).sock, 500);
    assert.ok(sub);
    const subscribed = await sub.peer.request("subscribe", { channel: "serial.events" }, 1000);
    assert.equal(subscribed.error, undefined);
    await releaseDaemon({ configDir: dir, yield: true });
    assert.match(await daemonStatusText({ configDir: dir }), /daemon running/);
    assert.match(await daemonStatusText({ configDir: dir }), /port: down/);
    assert.match(await daemonStatusText({ configDir: dir }), /yield: \d+s/);
    let spawns = 0;
    await assert.rejects(
      () => ensureDaemon({ configDir: dir, spawnFn: () => { spawns += 1; } }),
      /yielded for upload/,
    );
    assert.equal(spawns, 0);
    writeFileSync(daemonPaths(dir).yieldFile, JSON.stringify({ until: Date.now() - 1 }));
    await delay(20);
    assert.equal(opens, 1);
    assert.match(await daemonStatusText({ configDir: dir }), /port: down/);
    assert.match(await daemonStatusText({ configDir: dir }), /yield: off/);
    sub.socket.end();
  } finally {
    await disableDaemon({ configDir: dir });
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("disable reports an unwritable config directory", async () => {
  const dir = tempConfig();
  const blocked = join(dir, "not-a-directory");
  writeFileSync(blocked, "x");
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  const exitCode = process.exitCode;
  process.env.TINY_ENGINEER_CONFIG_DIR = blocked;
  process.exitCode = undefined;
  /** @type {string[]} */
  const errors = [];
  try {
    await runDaemon(createOutput({
      error(line) {
        errors.push(String(line));
      },
    }), ["disable"]);
    assert.equal(process.exitCode, 1);
    assert.equal(errors.length, 1);
    assert.match(errors[0], /error:/);
  } finally {
    process.exitCode = exitCode;
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("disable blocks auto-start until start and plain stop does not", async () => {
  const dir = tempConfig();
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = dir;
  const exitCode = process.exitCode;
  try {
    await runDaemon(quietOut(), ["stop"]);
    assert.equal(disableActive(dir), false);
    await runDaemon(quietOut(), ["disable"]);
    assert.equal(disableActive(dir), true);
    assert.equal(statSync(daemonPaths(dir).disabled).mode & 0o777, 0o600);
    let spawns = 0;
    await assert.rejects(
      () => ensureDaemon({ configDir: dir, spawnFn: () => { spawns += 1; } }),
      /daemon disabled/,
    );
    assert.equal(spawns, 0);
    await runDaemon(quietOut(), ["start"]);
    assert.equal(process.exitCode ?? 0, 0);
    assert.equal(disableActive(dir), false);
    const status = await daemonStatusText({ configDir: dir });
    assert.match(status, /daemon running/);
    assert.match(status, /port: down/);
    await disableDaemon({ configDir: dir });
    await runDaemon(quietOut(), ["restart"]);
    assert.equal(disableActive(dir), false);
    assert.match(await daemonStatusText({ configDir: dir }), /daemon running/);
  } finally {
    process.exitCode = exitCode;
    spawnSync(process.execPath, [cli, "daemon", "disable"], {
      env: { ...process.env, TINY_ENGINEER_CONFIG_DIR: dir },
      stdio: "ignore",
    });
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
  }
});

/**
 * @param {string} dir
 * @param {string} body
 * @param {Date} [mtime]
 */
function writeLock(dir, body, mtime) {
  const path = daemonPaths(dir).lock;
  writeFileSync(path, body);
  if (mtime) utimesSync(path, mtime, mtime);
}

test("stale lock is replaced once and a fresh live lock is kept", async () => {
  const deadDir = tempConfig();
  const emptyDir = tempConfig();
  const oldDir = tempConfig();
  const freshDir = tempConfig();
  /** @type {Promise<void> | undefined} */
  let deadServing;
  /** @type {Promise<void> | undefined} */
  let emptyServing;
  /** @type {Promise<void> | undefined} */
  let oldServing;
  try {
    writeLock(deadDir, "99999999");
    let deadSpawns = 0;
    await ensureDaemon({
      configDir: deadDir,
      spawnFn: () => {
        deadSpawns += 1;
        deadServing = startServer(deadDir, async () => fakeSession("/dev/fake"));
      },
    });
    assert.equal(deadSpawns, 1);

    writeLock(emptyDir, "");
    let emptySpawns = 0;
    await ensureDaemon({
      configDir: emptyDir,
      spawnFn: () => {
        emptySpawns += 1;
        emptyServing = startServer(emptyDir, async () => fakeSession("/dev/fake"));
      },
    });
    assert.equal(emptySpawns, 1);

    writeLock(oldDir, String(process.pid), new Date(Date.now() - 30_000));
    let oldSpawns = 0;
    await ensureDaemon({
      configDir: oldDir,
      spawnFn: () => {
        oldSpawns += 1;
        oldServing = startServer(oldDir, async () => fakeSession("/dev/fake"));
      },
    });
    assert.equal(oldSpawns, 1);

    writeLock(freshDir, String(process.pid));
    let freshSpawns = 0;
    await assert.rejects(
      () => ensureDaemon({
        configDir: freshDir,
        spawnFn: () => { freshSpawns += 1; },
      }),
      /failed to start/,
    );
    assert.equal(freshSpawns, 0);
  } finally {
    await disableDaemon({ configDir: deadDir }).catch(() => {});
    await disableDaemon({ configDir: emptyDir }).catch(() => {});
    await disableDaemon({ configDir: oldDir }).catch(() => {});
    await deadServing;
    await emptyServing;
    await oldServing;
    rmSync(deadDir, { recursive: true, force: true });
    rmSync(emptyDir, { recursive: true, force: true });
    rmSync(oldDir, { recursive: true, force: true });
    rmSync(freshDir, { recursive: true, force: true });
  }
});

test("failed hello spawns once", async () => {
  const dir = tempConfig();
  try {
    let spawns = 0;
    await assert.rejects(
      () => ensureDaemon({ configDir: dir, spawnFn: () => { spawns += 1; } }),
      /failed to start/,
    );
    assert.equal(spawns, 1);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("foreign pid is not signaled and a dead pid is replaced", async () => {
  const dir = tempConfig();
  try {
    writeFileSync(daemonPaths(dir).state, JSON.stringify({ pid: process.pid }));
    /** @type {number[]} */
    const killed = [];
    let spawns = 0;
    /** @type {Promise<void> | undefined} */
    let serving;
    await ensureDaemon({
      configDir: dir,
      probePid: () => "foreign",
      terminate: async (pid) => { killed.push(pid); },
      spawnFn: () => {
        spawns += 1;
        serving = startServer(dir, async () => fakeSession("/dev/fake"));
      },
    });
    assert.deepEqual(killed, []);
    assert.equal(spawns, 1);
    await disableDaemon({ configDir: dir });
    await serving;
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("unknown pid fails closed", async () => {
  const dir = tempConfig();
  try {
    mkdirSync(dir, { recursive: true });
    writeFileSync(daemonPaths(dir).state, JSON.stringify({ pid: 12345 }));
    let spawns = 0;
    await assert.rejects(
      () => ensureDaemon({
        configDir: dir,
        probePid: (pid) => (pid ? "unknown" : "none"),
        spawnFn: () => { spawns += 1; },
      }),
      /cannot identify process|only frees the port/,
    );
    assert.equal(spawns, 0);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("wedged daemon is terminated then replaced", async () => {
  const dir = tempConfig();
  try {
    mkdirSync(dir, { recursive: true });
    writeFileSync(daemonPaths(dir).state, JSON.stringify({ pid: 77 }));
    /** @type {number[]} */
    const killed = [];
    let spawns = 0;
    /** @type {Promise<void> | undefined} */
    let serving;
    await ensureDaemon({
      configDir: dir,
      probePid: () => "ours",
      terminate: async (pid) => { killed.push(pid); },
      spawnFn: () => {
        spawns += 1;
        serving = startServer(dir, async () => fakeSession("/dev/fake"));
      },
    });
    assert.deepEqual(killed, [77]);
    assert.equal(spawns, 1);
    await disableDaemon({ configDir: dir });
    await serving;
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("windows ipc path is a named pipe derived from the config dir", () => {
  const a = daemonIpcPath("/cfg/a", "win32");
  const b = daemonIpcPath("/cfg/b", "win32");
  assert.match(a, /^\\\\\.\\pipe\\tiny-engineer-[0-9a-f]{16}$/);
  assert.match(b, /^\\\\\.\\pipe\\tiny-engineer-[0-9a-f]{16}$/);
  assert.notEqual(a, b);
  assert.equal(daemonIpcPath("/cfg/a", "win32"), a);
  const dir = tempConfig();
  try {
    assert.equal(daemonIpcPath(dir, "darwin"), join(dir, "daemon.sock"));
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("two starts share one daemon", async () => {
  const dir = tempConfig();
  try {
    let spawns = 0;
    /** @type {Promise<void> | undefined} */
    let serving;
    const spawnFn = () => {
      spawns += 1;
      if (spawns === 1) serving = startServer(dir, async () => fakeSession("/dev/fake"));
    };
    await Promise.all([
      ensureDaemon({ configDir: dir, spawnFn }),
      ensureDaemon({ configDir: dir, spawnFn }),
    ]);
    assert.equal(spawns, 1);
    const text = await daemonStatusText({ configDir: dir });
    assert.match(text, /daemon running/);
    assert.match(text, /proto: 1/);
    assert.match(text, /port: down/);
    assert.match(text, /channel: dev/);
    assert.match(text, /source: [0-9a-f]{8}/);
    assert.ok(text.includes(`packageRoot: ${packageRoot.replace(/\/$/, "")}`));
    if (process.platform !== "win32") {
      const mode = statSync(daemonPaths(dir).sock).mode & 0o777;
      assert.equal(mode, 0o600);
    }
    await disableDaemon({ configDir: dir });
    await serving;
    const stopped = await daemonStatusText({ configDir: dir });
    assert.match(stopped, /not running/);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("serial requests share one open and a subscriber can anim", async () => {
  const dir = tempConfig();
  /** @type {Promise<void> | undefined} */
  let serving;
  /** @type {Promise<void> | undefined} */
  let following;
  try {
    let opens = 0;
    /** @type {ReturnType<typeof fakeSession> | null} */
    let session = null;
    let ready = () => {};
    const readyP = new Promise((resolve) => { ready = resolve; });
    serving = startServer(dir, async ({ port }) => {
      opens += 1;
      session = fakeSession(port || "/dev/a", ready);
      return session;
    });
    const lines = [];
    let linkDowns = 0;
    following = followDaemonChannel({
      configDir: dir,
      channel: "serial.events",
      onLine: (line) => lines.push(line),
      onLinkDown: () => { linkDowns += 1; },
    });
    await readyP;
    await delay(30);
    session?.emit('{"v":1}');
    await daemonRobotRequest("anim", { name: "ring" }, { configDir: dir, port: "/dev/a" });
    session?.emit('{"v":2}');
    await delay(30);
    assert.equal(opens, 1);
    assert.deepEqual(lines, ['{"v":1}', '{"v":2}']);
    await assert.rejects(
      () => daemonRobotRequest("anim", { name: "ring" }, { configDir: dir, port: "/dev/b" }),
      /daemon is on \/dev\/a, this command asked for \/dev\/b/,
    );
    assert.equal(opens, 1);
    session?.drop();
    await delay(30);
    assert.equal(linkDowns, 1);
    assert.deepEqual(lines, ['{"v":1}', '{"v":2}']);
    await daemonRobotRequest("health", {}, { configDir: dir });
    assert.equal(opens, 2);
  } finally {
    await disableDaemon({ configDir: dir }).catch(() => {});
    await following;
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("client reset does not kill the daemon", async () => {
  const dir = tempConfig();
  /** @type {Promise<void> | undefined} */
  let serving;
  try {
    /** @type {ReturnType<typeof fakeSession> | null} */
    let session = null;
    let ready = () => {};
    const readyP = new Promise((resolve) => { ready = resolve; });
    serving = startServer(dir, async ({ port }) => {
      session = fakeSession(port || "/dev/a", ready);
      return session;
    });
    const sock = daemonPaths(dir).sock;
    for (let i = 0; i < 50 && !existsSync(sock); i += 1) await delay(20);
    const conn = await tryConnect(sock, 500);
    assert.ok(conn);
    const subscribed = conn.peer.request("subscribe", { channel: "serial.events" }, 1000);
    await readyP;
    assert.equal((await subscribed).error, undefined);
    conn.socket.destroy();
    await delay(20);
    session?.emit('{"v":1}');
    await delay(20);
    const status = await daemonStatusText({ configDir: dir });
    assert.match(status, /daemon running/);
  } finally {
    await disableDaemon({ configDir: dir }).catch(() => {});
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("overlapping ensure opens the port once", async () => {
  let opens = 0;
  /** @type {() => void} */
  let release = () => {};
  const gate = new Promise((resolve) => {
    release = resolve;
  });
  const link = createSerialLink({
    connect: async ({ port }) => {
      opens += 1;
      await gate;
      return fakeSession(port || "/dev/a");
    },
  });
  const first = link.ensure(null);
  const second = link.ensure("/dev/a");
  assert.equal(opens, 1);
  release();
  const [a, b] = await Promise.all([first, second]);
  assert.equal(a, b);
  assert.equal(a.port, "/dev/a");
  await assert.rejects(() => link.ensure("/dev/other"), /daemon is on \/dev\/a, this command asked for \/dev\/other/);
  assert.equal(opens, 1);
});

test("stale close leaves the replacement session", async () => {
  /** @type {(() => void) | null} */
  let finishFirstClose = null;
  let opens = 0;
  const link = createSerialLink({
    connect: async ({ port }) => {
      opens += 1;
      const which = opens;
      /** @type {(() => void) | null} */
      let onClose = null;
      return {
        port: port || "/dev/a",
        client: {
          setEventHandler() {},
          setLogHandler() {},
          setCloseHandler(fn) { onClose = fn; },
          close: () => new Promise((resolve) => {
            const finish = () => {
              onClose?.();
              resolve(undefined);
            };
            if (which === 1) finishFirstClose = finish;
            else finish();
          }),
        },
      };
    },
  });
  let downs = 0;
  link.onLinkDown(() => { downs += 1; });
  await link.ensure("/dev/old");
  const releasing = link.release();
  await link.ensure("/dev/new");
  assert.equal(link.port, "/dev/new");
  assert.equal(downs, 1);
  assert.ok(finishFirstClose);
  finishFirstClose();
  assert.equal(await releasing, true);
  assert.equal(link.port, "/dev/new");
  assert.equal(downs, 1);
  assert.equal(opens, 2);
});

test("tryConnect ignores chunks after hello", async () => {
  const dir = tempConfig();
  const sock = join(dir, "t.sock");
  mkdirSync(dir, { recursive: true });
  /** @type {import("node:net").Socket | null} */
  let remote = null;
  const server = createServer((socket) => {
    remote = socket;
    socket.write(`${JSON.stringify({ method: "hello", params: { pid: 1 } })}\n`);
    for (let i = 0; i < 5; i += 1) {
      setTimeout(() => {
        socket.write(`${JSON.stringify({ method: "event", params: { data: String(i) } })}\n`);
      }, 20 * (i + 1));
    }
  });
  await new Promise((resolve) => server.listen(sock, resolve));
  try {
    const conn = await tryConnect(sock, 500);
    assert.ok(conn);
    /** @type {string[]} */
    const lines = [];
    conn.peer.on((msg) => {
      if (msg.method !== "event") return;
      const data = /** @type {{ data?: string }} */ (msg.params)?.data;
      if (typeof data === "string") lines.push(data);
    });
    await delay(160);
    assert.equal(conn.socket.listenerCount("data"), 2);
    assert.deepEqual(lines, ["0", "1", "2", "3", "4"]);
    conn.socket.end();
  } finally {
    remote?.destroy();
    server.close();
    rmSync(dir, { recursive: true, force: true });
  }
});

test("serial play sends an absolute wav path", async () => {
  const dir = tempConfig();
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = dir;
  /** @type {Promise<void> | undefined} */
  let serving;
  try {
    serving = startServer(dir, async ({ port }) => fakeSession(port || "/dev/a"));
    const sock = daemonPaths(dir).sock;
    for (let i = 0; i < 50 && !existsSync(sock); i += 1) await delay(20);
    const result = await robotPlay({
      wav: new Uint8Array(),
      wavPath: "clip.wav",
      transport: "serial",
    });
    assert.equal(result.ok, false);
    const abs = resolve("clip.wav");
    assert.notEqual(abs, "clip.wav");
    assert.ok(result.error?.includes(abs), result.error);
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    await disableDaemon({ configDir: dir }).catch(() => {});
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("failed play sends play_end and keeps the port", async () => {
  const dir = tempConfig();
  const wav = join(dir, "clip.wav");
  try {
    mkdirSync(dir, { recursive: true });
    writeFileSync(wav, Buffer.alloc(8));
    const client = new SerialRobotClient("/dev/fake");
    /** @type {string[]} */
    const ops = [];
    client._port = { isOpen: true, write(_frame, cb) { cb(null); } };
    client.request = async (op, fields = {}) => {
      ops.push(op);
      if (op === "play") return { ok: true, stream_id: 3, credits: 64 };
      if (op === "play_end") return { ok: true, stream_id: fields.stream_id };
      return { ok: true };
    };
    client.waitCredit = async () => {
      throw new Error("serial play rejected: over credits");
    };
    let closed = false;
    const realClose = client.close.bind(client);
    client.close = async () => {
      closed = true;
      await realClose();
    };
    const serving = startServer(dir, async () => ({ port: "/dev/fake", client }));
    await assert.rejects(() => daemonPlay({ path: wav, configDir: dir }), /over credits/);
    assert.ok(ops.includes("play_end"));
    assert.equal(closed, false);
    assert.equal(client._port.isOpen, true);
    await disableDaemon({ configDir: dir });
    await serving;
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("replace during play aborts play and stops the daemon", async () => {
  const dir = tempConfig();
  const wav = join(dir, "clip.wav");
  /** @type {Awaited<ReturnType<typeof tryConnect>>} */
  let player = null;
  try {
    mkdirSync(dir, { recursive: true });
    writeFileSync(wav, Buffer.alloc(4));
    /** @type {(value: Record<string, unknown>) => void} */
    let release = () => {};
    let closed = false;
    const client = {
      request: async () => ({ ok: true }),
      play: () => new Promise((resolve) => { release = resolve; }),
      setEventHandler() {},
      setLogHandler() {},
      setCloseHandler() {},
      close: async () => {
        closed = true;
        release({ ok: false, error: "closed" });
      },
    };
    const serving = startServer(dir, async () => ({ port: "/dev/fake", client }));
    for (let i = 0; i < 50 && !existsSync(daemonPaths(dir).sock); i += 1) await delay(20);
    player = await tryConnect(daemonPaths(dir).sock, 500);
    assert.ok(player);
    void player.peer.request("play", { path: wav, name: "talking" }, 60_000).catch(() => {});
    await delay(30);
    const stopper = await tryConnect(daemonPaths(dir).sock, 500);
    assert.ok(stopper);
    const stopped = await stopper.peer.request("daemon.stop", { reason: "replace" }, 1000);
    assert.equal(stopped.error, undefined);
    assert.equal(stopped.result && /** @type {{ ok?: boolean }} */ (stopped.result).ok, true);
    stopper.socket.destroy();
    await serving;
    assert.equal(closed, true);
    assert.match(await daemonStatusText({ configDir: dir }), /daemon is not running/);
  } finally {
    player?.socket.destroy();
    await disableDaemon({ configDir: dir }).catch(() => {});
    rmSync(dir, { recursive: true, force: true });
  }
});

test("newer client replaces daemon during play", async () => {
  const dir = tempConfig();
  const wav = join(dir, "clip.wav");
  const root = packageRoot.replace(/\/$/, "");
  const current = loadIdentity(root);
  /** @type {Promise<void> | undefined} */
  let servingOld;
  /** @type {Promise<void> | undefined} */
  let servingNew;
  /** @type {Awaited<ReturnType<typeof tryConnect>>} */
  let player = null;
  try {
    mkdirSync(dir, { recursive: true });
    writeFileSync(wav, Buffer.alloc(4));
    /** @type {(value: Record<string, unknown>) => void} */
    let release = () => {};
    const client = {
      request: async () => ({ ok: true }),
      play: () => new Promise((resolve) => { release = resolve; }),
      setEventHandler() {},
      setLogHandler() {},
      setCloseHandler() {},
      close: async () => { release({ ok: false, error: "closed" }); },
    };
    servingOld = startServer(dir, async () => ({ port: "/dev/fake", client }), {
      ...current,
      proto: 0,
    });
    const sock = daemonPaths(dir).sock;
    for (let i = 0; i < 50 && !existsSync(sock); i += 1) await delay(20);
    player = await tryConnect(sock, 500);
    assert.ok(player);
    void player.peer.request("play", { path: wav, name: "talking" }, 60_000).catch(() => {});
    await delay(30);
    assert.match(await daemonStatusText({ configDir: dir }), /proto: 0/);
    const conn = await ensureDaemon({
      configDir: dir,
      spawnFn: () => {
        servingNew = startServer(dir, async () => fakeSession("/dev/fake"));
      },
    });
    conn.socket.end();
    const after = await daemonStatusText({ configDir: dir });
    assert.match(after, new RegExp(`proto: ${current.proto}`));
    assert.doesNotMatch(after, /proto: 0/);
    await servingOld.catch(() => {});
    await disableDaemon({ configDir: dir }).catch(() => {});
    if (servingNew) await servingNew.catch(() => {});
  } finally {
    player?.socket.destroy();
    rmSync(dir, { recursive: true, force: true });
  }
});

test("follow exits on stop, resubscribes on replace, retries one drop", async () => {
  /** @type {string[]} */
  const script = [];
  await followDaemonChannel({
    channel: "serial.events",
    onLine: () => {},
    subscribe: async () => {
      script.push("stop");
      return "stop";
    },
  });
  assert.deepEqual(script, ["stop"]);

  /** @type {string[]} */
  const replaced = [];
  await followDaemonChannel({
    channel: "serial.events",
    onLine: () => {},
    subscribe: async () => {
      replaced.push("x");
      return replaced.length === 1 ? "replace" : "stop";
    },
  });
  assert.equal(replaced.length, 2);

  await assert.rejects(
    () => followDaemonChannel({
      channel: "serial.events",
      onLine: () => {},
      subscribe: async () => { throw new Error("no link"); },
    }),
    /no link/,
  );

  let drops = 0;
  await assert.rejects(
    () => followDaemonChannel({
      channel: "serial.events",
      onLine: () => {},
      subscribe: async () => {
        drops += 1;
        if (drops === 1) return "drop";
        throw new Error("daemon connection lost");
      },
    }),
    /daemon connection lost/,
  );
  assert.equal(drops, 2);

  let recovered = 0;
  await followDaemonChannel({
    channel: "serial.events",
    onLine: () => {},
    subscribe: async (inner) => {
      recovered += 1;
      inner.onSubscribed?.();
      return recovered < 3 ? "drop" : "stop";
    },
  });
  assert.equal(recovered, 3);
});

test("subscribeOnce reports a live subscription before the socket ends", async () => {
  const dir = tempConfig();
  /** @type {Promise<void> | undefined} */
  let serving;
  /** @type {Promise<unknown> | undefined} */
  let following;
  try {
    serving = startServer(dir, async ({ port }) => fakeSession(port || "/dev/a"));
    const sock = daemonPaths(dir).sock;
    for (let i = 0; i < 50 && !existsSync(sock); i += 1) await delay(20);
    let subscribed = 0;
    following = subscribeOnce({
      channel: "serial.events",
      configDir: dir,
      onLine: () => {},
      onSubscribed: () => { subscribed += 1; },
    });
    for (let i = 0; i < 50 && subscribed === 0; i += 1) await delay(20);
    assert.equal(subscribed, 1);
    assert.match(await daemonStatusText({ configDir: dir }), /daemon running/);
  } finally {
    await disableDaemon({ configDir: dir }).catch(() => {});
    await following;
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("http anim does not start a daemon", async () => {
  const dir = tempConfig();
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = dir;
  try {
    const result = await robotSetAnim("ring", { transport: "http", url: "http://127.0.0.1:1" });
    assert.equal(result.ok, false);
    assert.equal(existsSync(daemonPaths(dir).sock), false);
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("daemon start survives the parent and SIGHUP", async () => {
  const dir = tempConfig();
  const env = { ...process.env, TINY_ENGINEER_CONFIG_DIR: dir };
  try {
    const start = spawn(process.execPath, [cli, "daemon", "start"], { env, stdio: "ignore" });
    const code = await new Promise((resolve) => start.once("exit", resolve));
    assert.equal(code, 0);
    const state = JSON.parse(readFileSync(daemonPaths(dir).state, "utf8"));
    process.kill(state.pid, "SIGHUP");
    await delay(100);
    const status = spawnSync(process.execPath, [cli, "daemon", "status"], { env, encoding: "utf8" });
    assert.match(status.stdout, /daemon running/);
    assert.match(status.stdout, new RegExp(`pid: ${state.pid}`));
    const again = spawnSync(process.execPath, [cli, "daemon", "start"], { env, encoding: "utf8" });
    assert.equal(again.status, 0);
    const after = JSON.parse(readFileSync(daemonPaths(dir).state, "utf8"));
    assert.equal(after.pid, state.pid);
  } finally {
    spawnSync(process.execPath, [cli, "daemon", "disable"], { env, stdio: "ignore" });
    rmSync(dir, { recursive: true, force: true });
  }
});

test("start --port warns while daemon install is on", async () => {
  const dir = tempConfig();
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  const exitCode = process.exitCode;
  process.env.TINY_ENGINEER_CONFIG_DIR = dir;
  process.exitCode = 0;
  /** @type {string[]} */
  const notes = [];
  const out = createOutput({
    error(line) {
      notes.push(String(line));
    },
  });
  const serving = startServer(dir, async () => fakeSession("/dev/fake"));
  try {
    for (let i = 0; i < 50 && !existsSync(daemonPaths(dir).state); i += 1) await delay(20);
    assert.equal(existsSync(daemonPaths(dir).state), true);
    await runDaemon(out, ["start", "--port", "/dev/ignored"]);
    assert.equal(process.exitCode ?? 0, 0);
    assert.equal(notes.some((line) => line.includes("ignored while daemon install is on")), false);
    writeFileSync(daemonPaths(dir).supervisor, "{}\n");
    notes.length = 0;
    await runDaemon(out, ["start", "--port", "/dev/ignored"]);
    assert.equal(process.exitCode ?? 0, 0);
    assert.equal(notes.some((line) => line.includes("warning: --port is ignored while daemon install is on")), true);
  } finally {
    process.exitCode = exitCode;
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    await disableDaemon({ configDir: dir }).catch(() => {});
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("daemon restart --port is the port the new process prefers", async () => {
  const dir = tempConfig();
  const env = { ...process.env, TINY_ENGINEER_CONFIG_DIR: dir };
  try {
    const start = spawnSync(process.execPath, [cli, "daemon", "start"], { env, encoding: "utf8" });
    assert.equal(start.status, 0, start.stderr);
    const before = JSON.parse(readFileSync(daemonPaths(dir).state, "utf8"));
    const restart = spawnSync(process.execPath, [cli, "daemon", "restart", "--port", "/dev/new"], {
      env,
      encoding: "utf8",
    });
    assert.equal(restart.status, 0, restart.stderr);
    const after = JSON.parse(readFileSync(daemonPaths(dir).state, "utf8"));
    assert.notEqual(after.pid, before.pid);
    assert.match(String(readProcessCommand(after.pid)), /--port \/dev\/new/);
  } finally {
    spawnSync(process.execPath, [cli, "daemon", "disable"], { env, stdio: "ignore" });
    rmSync(dir, { recursive: true, force: true });
  }
});

test("preferred port is used when the request omits one", async () => {
  const dir = tempConfig();
  try {
    /** @type {Array<string | null | undefined>} */
    const seen = [];
    const serving = serveDaemon({
      configDir: dir,
      exitProcess: false,
      preferredPort: "/dev/preferred",
      connect: async ({ port }) => {
        seen.push(port);
        return fakeSession(port || "/dev/none");
      },
    });
    await daemonRobotRequest("health", {}, { configDir: dir });
    assert.deepEqual(seen, ["/dev/preferred"]);
    await disableDaemon({ configDir: dir });
    await serving;
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("robot.probe walks free candidates without writing cache flags", async () => {
  const dir = tempConfig();
  /** @type {Array<{ port?: string | null, writeCache?: boolean }>} */
  const opens = [];
  try {
    const serving = startServer(dir, async (opts) => {
      opens.push({ port: opts.port, writeCache: opts.writeCache });
      if (opts.port === "/dev/bad") throw new Error("open failed");
      return fakeSession(opts.port || "/dev/none");
    });
    const sock = daemonPaths(dir).sock;
    for (let i = 0; i < 50 && !existsSync(sock); i += 1) await delay(20);
    const body = await daemonProbe({
      configDir: dir,
      candidates: ["/dev/bad", "/dev/good"],
    });
    assert.equal(body.ok, true);
    assert.equal(body.port, "/dev/good");
    assert.deepEqual(
      body.results.map((row) => ({ path: row.path, ok: row.ok, detail: row.detail })),
      [
        { path: "/dev/bad", ok: false, detail: "open failed" },
        { path: "/dev/good", ok: true, detail: "sync ok" },
      ],
    );
    assert.deepEqual(opens, [
      { port: "/dev/bad", writeCache: false },
      { port: "/dev/good", writeCache: false },
    ]);
    assert.match(await daemonStatusText({ configDir: dir }), /port: \/dev\/good/);
    await disableDaemon({ configDir: dir });
    await serving;
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("robot.probe syncs a held port and skips other candidates", async () => {
  const dir = tempConfig();
  try {
    let opens = 0;
    const serving = startServer(dir, async ({ port }) => {
      opens += 1;
      return fakeSession(port || "/dev/held");
    });
    const sock = daemonPaths(dir).sock;
    for (let i = 0; i < 50 && !existsSync(sock); i += 1) await delay(20);
    await daemonRobotRequest("health", {}, { configDir: dir, port: "/dev/held" });
    assert.equal(opens, 1);
    const body = await daemonProbe({
      configDir: dir,
      candidates: ["/dev/held", "/dev/other"],
    });
    assert.equal(body.ok, true);
    assert.equal(opens, 1);
    assert.equal(body.results[0]?.ok, true);
    assert.equal(body.results[1]?.skipped, true);
    assert.match(String(body.results[1]?.detail), /daemon holds \/dev\/held/);
    await disableDaemon({ configDir: dir });
    await serving;
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("robot.probe refuses while play is active", async () => {
  const dir = tempConfig();
  const wav = join(dir, "clip.wav");
  /** @type {(value: Record<string, unknown>) => void} */
  let finish = () => {};
  /** @type {Promise<void> | undefined} */
  let serving;
  /** @type {Promise<unknown> | undefined} */
  let playing;
  try {
    mkdirSync(dir, { recursive: true });
    writeFileSync(wav, Buffer.alloc(4));
    /** @type {() => void} */
    let markStarted = () => {};
    const started = new Promise((resolve) => {
      markStarted = resolve;
    });
    serving = startServer(dir, async () => ({
      port: "/dev/fake",
      client: {
        request: async () => ({ ok: true }),
        play: () => {
          markStarted();
          return new Promise((resolve) => {
            finish = resolve;
          });
        },
        setEventHandler() {},
        setLogHandler() {},
        setCloseHandler() {},
        close: async () => {},
      },
    }));
    playing = daemonPlay({ path: wav, configDir: dir });
    const startedInTime = await Promise.race([
      started.then(() => true),
      delay(2000).then(() => false),
    ]);
    assert.equal(startedInTime, true);
    await assert.rejects(
      () => daemonProbe({ configDir: dir, candidates: ["/dev/fake"] }),
      /play-active/,
    );
  } finally {
    const played = playing?.catch(() => {});
    finish({ ok: true });
    await disableDaemon({ configDir: dir }).catch(() => {});
    await played;
    if (serving) await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

/**
 * @param {string} port
 * @param {() => void} [onEventHandler]
 */
function fakeSession(port, onEventHandler) {
  /** @type {((payload: Buffer) => void) | null} */
  let onEvent = null;
  /** @type {(() => void) | null} */
  let onClose = null;
  return {
    port,
    client: {
      request: async (op, fields) => ({ ok: true, op, ...fields }),
      play: async () => ({ ok: true }),
      setEventHandler(fn) {
        onEvent = fn;
        onEventHandler?.();
      },
      setLogHandler() {},
      setCloseHandler(fn) { onClose = fn; },
      close: async () => { onClose?.(); },
    },
    emit(text) { onEvent?.(Buffer.from(text)); },
    drop() { onClose?.(); },
  };
}
