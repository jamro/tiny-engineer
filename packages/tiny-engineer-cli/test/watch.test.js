import test from "node:test";
import assert from "node:assert/strict";
import { existsSync, mkdtempSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

import {
  eventsWebSocketUrl,
  handleWatchSerialMessage,
  redactEventsUrl,
  runWatch,
} from "../src/commands/watch.js";
import { disableDaemon, ensureDaemon, followDaemonChannel, releaseDaemon, stopDaemon } from "../src/daemon/client.js";
import { uninstallSupervisor } from "../src/daemon/install.js";
import { daemonPaths } from "../src/daemon/paths.js";
import { delay } from "../src/daemon/proc.js";
import { serveDaemon } from "../src/daemon/server.js";
import { packMessage, SerialMsgType } from "../src/robot/serial/protocol.js";
import { createOutput } from "../src/util/output.js";

test("eventsWebSocketUrl rewrites http to ws port 81 /events", () => {
  assert.equal(
    eventsWebSocketUrl("http://tiny-engineer.local"),
    "ws://tiny-engineer.local:81/events",
  );
  assert.equal(
    eventsWebSocketUrl("http://robot.example/path", "secret"),
    "ws://robot.example:81/events?token=secret",
  );
  assert.equal(
    eventsWebSocketUrl("http://robot.example", "two words"),
    "ws://robot.example:81/events?token=two%20words",
  );
  assert.equal(
    eventsWebSocketUrl("http://robot.example", "a+b"),
    "ws://robot.example:81/events?token=a%2Bb",
  );
});

test("eventsWebSocketUrl rejects https bases", () => {
  assert.throws(
    () => eventsWebSocketUrl("https://robot.example"),
    /http: robot URL/,
  );
  assert.throws(
    () => eventsWebSocketUrl("https://robot.example", "secret"),
    /no TLS on port 81/,
  );
});

test("redactEventsUrl hides token", () => {
  const redacted = redactEventsUrl("ws://host:81/events?token=super-secret");
  assert.match(redacted, /token=\*\*\*/);
  assert.doesNotMatch(redacted, /super-secret/);
});

test("handleWatchSerialMessage prints Evt frames only", () => {
  /** @type {string[]} */
  const lines = [];
  const evt = packMessage(SerialMsgType.Evt, 0, '{"v":1,"type":"button.down"}');
  assert.equal(handleWatchSerialMessage(evt, (l) => lines.push(l)), true);
  assert.deepEqual(lines, ['{"v":1,"type":"button.down"}']);

  const log = packMessage(SerialMsgType.Log, 0, '{"t":"noise"}');
  assert.equal(handleWatchSerialMessage(log, (l) => lines.push(l)), false);
  assert.deepEqual(lines, ['{"v":1,"type":"button.down"}']);
});

test("serial watch runs without a robot URL", async () => {
  const dir = mkdtempSync(join(existsSync("/tmp") ? "/tmp" : tmpdir(), "te-watch-"));
  const prevCfg = process.env.TINY_ENGINEER_CONFIG_DIR;
  const prevUrl = process.env.TINY_ENGINEER_URL;
  process.env.TINY_ENGINEER_CONFIG_DIR = dir;
  delete process.env.TINY_ENGINEER_URL;
  const exitCode = process.exitCode;
  /** @type {string[]} */
  const lines = [];
  const out = createOutput({
    log: (m) => lines.push(String(m)),
    error: (m) => lines.push(`E:${m}`),
  });
  const serving = serveDaemon({
    configDir: dir,
    exitProcess: false,
    connect: async ({ port }) => ({
      port: port || "/dev/fake",
      client: {
        request: async () => ({ ok: true }),
        play: async () => ({ ok: true }),
        setEventHandler() {},
        setLogHandler() {},
        setCloseHandler() {},
        close: async () => {},
      },
    }),
  });
  try {
    for (let i = 0; i < 50 && !existsSync(daemonPaths(dir).sock); i += 1) await delay(20);
    const pending = runWatch(out, ["--transport", "serial"]);
    await delay(80);
    await disableDaemon({ configDir: dir });
    await pending;
    await serving;
    assert.ok(lines.some((line) => line.includes("watch serial")));
    assert.equal(lines.some((line) => line.includes("error:")), false);
    assert.equal(process.exitCode ?? 0, 0);
  } finally {
    process.exitCode = exitCode;
    await disableDaemon({ configDir: dir }).catch(() => {});
    if (prevCfg === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prevCfg;
    if (prevUrl === undefined) delete process.env.TINY_ENGINEER_URL;
    else process.env.TINY_ENGINEER_URL = prevUrl;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("serial watch stays up across release and yield and ends on disable", async () => {
  const dir = mkdtempSync(join(existsSync("/tmp") ? "/tmp" : tmpdir(), "te-watch-"));
  const prevCfg = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = dir;
  const exitCode = process.exitCode;
  /** @type {string[]} */
  const lines = [];
  const out = createOutput({
    log: (m) => lines.push(String(m)),
    error: (m) => lines.push(`E:${m}`),
  });
  const serving = serveDaemon({
    configDir: dir,
    exitProcess: false,
    connect: async ({ port }) => ({
      port: port || "/dev/fake",
      client: {
        request: async () => ({ ok: true }),
        play: async () => ({ ok: true }),
        setEventHandler() {},
        setLogHandler() {},
        setCloseHandler() {},
        close: async () => {},
      },
    }),
  });
  try {
    for (let i = 0; i < 50 && !existsSync(daemonPaths(dir).sock); i += 1) await delay(20);
    const pending = runWatch(out, ["--transport", "serial"]);
    await delay(60);
    await stopDaemon({ configDir: dir });
    const stillUpStop = await Promise.race([
      pending.then(() => "done"),
      delay(40).then(() => "up"),
    ]);
    assert.equal(stillUpStop, "up");
    await releaseDaemon({ configDir: dir });
    const stillUp = await Promise.race([
      pending.then(() => "done"),
      delay(40).then(() => "up"),
    ]);
    assert.equal(stillUp, "up");
    await releaseDaemon({ configDir: dir, yield: true });
    const stillUpYield = await Promise.race([
      pending.then(() => "done"),
      delay(40).then(() => "up"),
    ]);
    assert.equal(stillUpYield, "up");
    const marked = lines.length;
    writeFileSync(daemonPaths(dir).yieldFile, JSON.stringify({ until: Date.now() - 1 }));
    await delay(50);
    assert.equal(lines.length, marked);
    await disableDaemon({ configDir: dir });
    await pending;
    await serving;
    assert.equal(process.exitCode ?? 0, 0);
  } finally {
    process.exitCode = exitCode;
    await disableDaemon({ configDir: dir }).catch(() => {});
    if (prevCfg === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prevCfg;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("supervised watch survives more than one drop and prints a later frame", async () => {
  const dir = mkdtempSync(join(existsSync("/tmp") ? "/tmp" : tmpdir(), "te-watch-sup-"));
  writeFileSync(daemonPaths(dir).supervisor, "{}\n");
  let n = 0;
  /** @type {string[]} */
  const lines = [];
  await followDaemonChannel({
    configDir: dir,
    channel: "serial.events",
    onLine: (line) => lines.push(line),
    subscribe: async (opts) => {
      n += 1;
      if (n < 3) return "drop";
      opts.onLine("{\"later\":true}");
      return "stop";
    },
  });
  assert.equal(n, 3);
  assert.deepEqual(lines, ["{\"later\":true}"]);
  rmSync(dir, { recursive: true, force: true });
});

test("uninstall ends watch without a disable file and a later command may spawn", async () => {
  const dir = mkdtempSync(join(existsSync("/tmp") ? "/tmp" : tmpdir(), "te-watch-un-"));
  const prevCfg = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = dir;
  writeFileSync(daemonPaths(dir).supervisor, "{}\n");
  const serving = serveDaemon({
    configDir: dir,
    exitProcess: false,
    connect: async ({ port }) => ({
      port: port || "/dev/fake",
      client: {
        request: async () => ({ ok: true }),
        play: async () => ({ ok: true }),
        setEventHandler() {},
        setLogHandler() {},
        setCloseHandler() {},
        close: async () => {},
      },
    }),
  });
  const out = createOutput({ log() {}, error() {} });
  try {
    for (let i = 0; i < 50 && !existsSync(daemonPaths(dir).sock); i += 1) await delay(20);
    const pending = runWatch(out, ["--transport", "serial"]);
    await delay(60);
    await uninstallSupervisor({ configDir: dir, registrar: { uninstall() {} } });
    await pending;
    await serving;
    assert.equal(existsSync(daemonPaths(dir).disabled), false);
    let spawns = 0;
    await assert.rejects(
      () => ensureDaemon({
        configDir: dir,
        spawnFn() { spawns += 1; },
        wrapperStart() { throw new Error("wrapper"); },
      }),
      /daemon failed to start/,
    );
    assert.equal(spawns, 1);
  } finally {
    await disableDaemon({ configDir: dir }).catch(() => {});
    if (prevCfg === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prevCfg;
    rmSync(dir, { recursive: true, force: true });
  }
});
