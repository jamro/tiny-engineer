import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import { existsSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { createServer } from "node:net";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

import { daemonRobotRequest, daemonStatusText, disableDaemon, ensureDaemon, tryConnect } from "../src/daemon/client.js";
import {
  installSupervisor,
  launchdPlist,
  systemdUnit,
  uninstallSupervisor,
  unitSpec,
  windowsTaskArgs,
  wrapperSource,
} from "../src/daemon/install.js";
import { daemonPaths } from "../src/daemon/paths.js";
import { loadIdentity, packageRootFromScript } from "../src/daemon/policy.js";
import { delay } from "../src/daemon/proc.js";
import { serveDaemon } from "../src/daemon/server.js";
import { readDaemonTarget } from "../src/daemon/target.js";

const cli = fileURLToPath(new URL("../bin/tiny-engineer.js", import.meta.url));

function tempDir() {
  const root = existsSync("/tmp") ? "/tmp" : tmpdir();
  return mkdtempSync(join(root, "te-sup-"));
}

/**
 * @param {string | null | undefined} port
 */
function fakeSession(port) {
  return {
    port: port || "/dev/fake",
    client: {
      request: async () => ({ ok: true }),
      play: async () => ({ ok: true }),
      setEventHandler() {},
      setLogHandler() {},
      setCloseHandler() {},
      close: async () => {},
    },
  };
}

test("daemon status shows install on or off", async () => {
  const dir = tempDir();
  try {
    const off = await daemonStatusText({ configDir: dir, listPorts: async () => [] });
    assert.match(off, /daemon is not running/);
    assert.match(off, /install: off/);
    assert.match(off, /yield: off/);
    assert.match(off, /ports: none/);
    writeFileSync(daemonPaths(dir).supervisor, "{}\n");
    const on = await daemonStatusText({ configDir: dir, listPorts: async () => [] });
    assert.match(on, /daemon is not running/);
    assert.match(on, /install: on/);
    const serving = serveDaemon({
      configDir: dir,
      exitProcess: false,
      connect: async ({ port }) => fakeSession(port),
    });
    try {
      for (let i = 0; i < 50 && !existsSync(daemonPaths(dir).sock); i += 1) await delay(20);
      const running = await daemonStatusText({ configDir: dir, listPorts: async () => [] });
      assert.match(running, /daemon running/);
      assert.match(running, /install: on/);
    } finally {
      await disableDaemon({ configDir: dir });
      await serving;
    }
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("daemon status lists ports and marks the open one", async () => {
  const dir = tempDir();
  const serving = serveDaemon({
    configDir: dir,
    exitProcess: false,
    preferredPort: "/dev/cu.usbmodem142201",
    connect: async ({ port }) => fakeSession(port),
  });
  try {
    await daemonRobotRequest("health", {}, { configDir: dir });
    const text = await daemonStatusText({
      configDir: dir,
      listPorts: async () => [
        { path: "/dev/tty.usbmodem142201" },
        { path: "/dev/tty.Bluetooth-Incoming-Port" },
      ],
    });
    assert.match(text, /port: \/dev\/cu\.usbmodem142201/);
    assert.match(text, / {2}\/dev\/tty\.usbmodem142201 connected/);
    assert.match(text, / {2}\/dev\/tty\.Bluetooth-Incoming-Port\n/);
    assert.doesNotMatch(text, /Bluetooth-Incoming-Port connected/);
  } finally {
    await disableDaemon({ configDir: dir }).catch(() => {});
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("daemon status treats a stalled port list as unavailable", async () => {
  const dir = tempDir();
  const started = Date.now();
  const text = await daemonStatusText({
    configDir: dir,
    listPorts: () => new Promise(() => {}),
  });
  const elapsed = Date.now() - started;
  assert.match(text, /daemon is not running/);
  assert.match(text, /ports: unavailable/);
  assert.ok(elapsed >= 1400);
  assert.ok(elapsed < 4000);
});

test("daemon status treats a port list error as unavailable", async () => {
  const dir = tempDir();
  const started = Date.now();
  const text = await daemonStatusText({
    configDir: dir,
    listPorts() {
      throw new Error("enum");
    },
  });
  assert.match(text, /daemon is not running/);
  assert.match(text, /ports: unavailable/);
  assert.ok(Date.now() - started < 1000);
});

test("daemon status shows remaining yield seconds", async () => {
  const dir = tempDir();
  try {
    writeFileSync(daemonPaths(dir).yieldFile, JSON.stringify({ until: 10_000 + 90_100 }));
    const text = await daemonStatusText({
      configDir: dir,
      now: 10_000,
      listPorts: async () => [],
    });
    assert.match(text, /yield: 91s/);
    const expired = await daemonStatusText({
      configDir: dir,
      now: 10_000 + 90_100,
      listPorts: async () => [],
    });
    assert.match(expired, /yield: off/);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("wrapper source passes the config directory", () => {
  const configDir = "C:\\Users\\ada\\AppData\\Roaming\\tiny-engineer";
  const source = wrapperSource("file:///pkg/wrapper.js", configDir);
  assert.match(source, /runWrapper\(\{ configDir: /);
  assert.ok(source.includes(JSON.stringify(configDir)));
});

test("login units escape spaces and special characters in paths", () => {
  const opts = {
    nodePath: "/opt/a\"b\\c & d/%h/node",
    wrapperPath: "/home/Mary Ann/.config/tiny-engineer/daemon-wrapper.js",
    configDir: "/home/Mary Ann/%h/.config/tiny-engineer",
  };
  const plist = launchdPlist(opts);
  assert.ok(plist.includes("<string>/opt/a\"b\\c &amp; d/%h/node</string>"));
  assert.ok(plist.includes("<string>/home/Mary Ann/.config/tiny-engineer/daemon-wrapper.js</string>"));
  assert.ok(plist.includes("<string>/home/Mary Ann/%h/.config/tiny-engineer</string>"));
  const unit = systemdUnit(opts);
  assert.ok(unit.includes('ExecStart="/opt/a\\"b\\\\c & d/%%h/node" "/home/Mary Ann/.config/tiny-engineer/daemon-wrapper.js"'));
  assert.ok(unit.includes('Environment="TINY_ENGINEER_CONFIG_DIR=/home/Mary Ann/%%h/.config/tiny-engineer"'));
});

test("login units do not restart and use absolute paths", () => {
  const opts = {
    nodePath: "/usr/local/bin/node",
    wrapperPath: "/tmp/te/daemon-wrapper.js",
    configDir: "/tmp/te",
  };
  const plist = launchdPlist(opts);
  assert.match(plist, /<key>RunAtLoad<\/key>/);
  assert.doesNotMatch(plist, /KeepAlive/);
  assert.match(plist, /\/usr\/local\/bin\/node/);
  assert.match(plist, /\/tmp\/te\/daemon-wrapper.js/);
  const unit = systemdUnit(opts);
  assert.doesNotMatch(unit, /Restart=/);
  assert.match(unit, /WantedBy=default.target/);
  assert.doesNotMatch(unit, /multi-user\.target/);
  const args = windowsTaskArgs(opts);
  assert.ok(args.includes("ONLOGON"));
  assert.equal(args.some((part) => /restart/i.test(part)), false);
  assert.equal(unitSpec("win32", opts).kind, "schtasks");
  assert.equal(unitSpec("linux", opts).kind, "systemd");
  assert.equal(unitSpec("darwin", opts).kind, "plist");
});

test("install during play replaces the daemon and registers the supervisor", async () => {
  const dir = tempDir();
  const wav = join(dir, "clip.wav");
  writeFileSync(wav, Buffer.alloc(4));
  /** @type {(value: Record<string, unknown>) => void} */
  let release = () => {};
  const serving = serveDaemon({
    configDir: dir,
    exitProcess: false,
    connect: async ({ port }) => ({
      port: port || "/dev/fake",
      client: {
        request: async () => ({ ok: true }),
        play: () => new Promise((resolve) => { release = resolve; }),
        setEventHandler() {},
        setLogHandler() {},
        setCloseHandler() {},
        close: async () => { release({ ok: false, error: "closed" }); },
      },
    }),
  });
  /** @type {Awaited<ReturnType<typeof tryConnect>>} */
  let player = null;
  let called = false;
  let starts = 0;
  try {
    for (let i = 0; i < 50 && !existsSync(daemonPaths(dir).sock); i += 1) await delay(20);
    player = await tryConnect(daemonPaths(dir).sock, 500);
    assert.ok(player);
    void player.peer.request("play", { path: wav, name: "talking" }, 60_000).catch(() => {});
    await delay(30);
    await installSupervisor({
      configDir: dir,
      scriptPath: "/opt/cli/bin/tiny-engineer.js",
      registrar: { install() { called = true; } },
      startWrapper() { starts += 1; },
    });
    assert.equal(called, true);
    assert.equal(existsSync(daemonPaths(dir).supervisor), true);
    assert.equal(starts, 1);
    await serving.catch(() => {});
  } finally {
    player?.socket.destroy();
    await disableDaemon({ configDir: dir }).catch(() => {});
    await uninstallSupervisor({
      configDir: dir,
      registrar: { uninstall() {} },
      shutdown: async () => {},
      wrapperAlive: () => false,
    }).catch(() => {});
    rmSync(dir, { recursive: true, force: true });
  }
});

test("install refuses when the daemon socket stays open", async () => {
  const dir = tempDir();
  /** @type {import("node:net").Socket[]} */
  const sockets = [];
  const server = createServer((socket) => {
    sockets.push(socket);
    let buf = "";
    socket.write('{"method":"hello","params":{"pid":1}}\n');
    socket.on("data", (chunk) => {
      buf += chunk.toString("utf8");
      let nl = buf.indexOf("\n");
      while (nl >= 0) {
        const line = buf.slice(0, nl);
        buf = buf.slice(nl + 1);
        nl = buf.indexOf("\n");
        if (!line.trim()) continue;
        try {
          const msg = JSON.parse(line);
          if (msg.method === "daemon.stop" && msg.id != null) {
            socket.write(`${JSON.stringify({ id: msg.id, result: { ok: true } })}\n`);
          }
        } catch {
          // ignore a broken line
        }
      }
    });
  });
  await new Promise((resolve) => {
    server.listen(daemonPaths(dir).sock, () => resolve(undefined));
  });
  let called = false;
  try {
    await assert.rejects(
      () => installSupervisor({
        configDir: dir,
        scriptPath: "/opt/cli/bin/tiny-engineer.js",
        registrar: { install() { called = true; } },
      }),
      /daemon socket still open/,
    );
    assert.equal(called, false);
    assert.equal(existsSync(daemonPaths(dir).supervisor), false);
  } finally {
    for (const socket of sockets) socket.destroy();
    await new Promise((resolve) => {
      server.close(() => resolve(undefined));
    });
    rmSync(dir, { recursive: true, force: true });
  }
});

test("second install refreshes the target and does not start another wrapper", async () => {
  const dir = tempDir();
  /** @type {string[]} */
  const bodies = [];
  let starts = 0;
  const registrar = {
    /**
     * @param {ReturnType<typeof unitSpec>} unit
     */
    install(unit) {
      if (unit.kind === "systemd") bodies.push(unit.body);
    },
  };
  /** @type {import("node:net").Server | undefined} */
  let holder;
  const startWrapper = () => new Promise((resolve, reject) => {
    starts += 1;
    holder = createServer((socket) => {
      socket.end();
    });
    holder.once("error", reject);
    holder.listen(daemonPaths(dir).wrapperLock, () => resolve(undefined));
  });
  try {
    await installSupervisor({
      configDir: dir,
      platform: "linux",
      nodePath: "/usr/bin/node-a",
      scriptPath: "/opt/old/bin/tiny-engineer.js",
      registrar,
      startWrapper,
    });
    await installSupervisor({
      configDir: dir,
      platform: "linux",
      nodePath: "/usr/bin/node-b",
      scriptPath: "/opt/new/bin/tiny-engineer.js",
      registrar,
      startWrapper,
    });
    assert.equal(starts, 1);
    assert.deepEqual(readDaemonTarget(dir), {
      execPath: "/usr/bin/node-b",
      script: "/opt/new/bin/tiny-engineer.js",
    });
    assert.match(bodies[1], /\/usr\/bin\/node-b/);
    assert.match(bodies[1], /daemon-wrapper\.js/);
    assert.doesNotMatch(bodies[1], /Restart=/);
    assert.equal(existsSync(daemonPaths(dir).disabled), false);
  } finally {
    if (holder) await new Promise((resolve) => holder.close(() => resolve(undefined)));
    rmSync(dir, { recursive: true, force: true });
  }
});

test("failed registration removes a new supervisor marker", async () => {
  const dir = tempDir();
  let starts = 0;
  try {
    await assert.rejects(
      () => installSupervisor({
        configDir: dir,
        scriptPath: cli,
        registrar: { install() { throw new Error("launchctl bootstrap failed"); } },
        startWrapper() { starts += 1; },
      }),
      /launchctl bootstrap failed/,
    );
    assert.equal(starts, 0);
    assert.equal(existsSync(daemonPaths(dir).supervisor), false);
    assert.equal(existsSync(daemonPaths(dir).target), false);
    assert.equal(existsSync(daemonPaths(dir).wrapper), false);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("failed refresh keeps an existing supervisor marker", async () => {
  const dir = tempDir();
  writeFileSync(daemonPaths(dir).supervisor, "{}\n");
  writeFileSync(daemonPaths(dir).target, "{\"execPath\":\"/old\",\"script\":\"/old.js\"}\n");
  let starts = 0;
  try {
    await assert.rejects(
      () => installSupervisor({
        configDir: dir,
        scriptPath: cli,
        registrar: { install() { throw new Error("systemctl --user enable failed"); } },
        startWrapper() { starts += 1; },
      }),
      /systemctl --user enable failed/,
    );
    assert.equal(starts, 0);
    assert.equal(existsSync(daemonPaths(dir).supervisor), true);
    assert.equal(readDaemonTarget(dir)?.script, cli);
    assert.equal(existsSync(daemonPaths(dir).wrapper), true);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("install hands an orphan daemon to one supervisor child", async () => {
  const dir = tempDir();
  const first = serveDaemon({
    configDir: dir,
    exitProcess: false,
    connect: async ({ port }) => fakeSession(port),
  });
  /** @type {Promise<void> | undefined} */
  let second;
  try {
    for (let i = 0; i < 50 && !existsSync(daemonPaths(dir).sock); i += 1) await delay(20);
    let starts = 0;
    await installSupervisor({
      configDir: dir,
      platform: "linux",
      nodePath: process.execPath,
      scriptPath: cli,
      registrar: { install() {} },
      startWrapper() {
        starts += 1;
        second = serveDaemon({
          configDir: dir,
          exitProcess: false,
          connect: async ({ port }) => fakeSession(port),
        });
      },
    });
    await first;
    assert.equal(starts, 1);
    const conn = await tryConnect(daemonPaths(dir).sock, 1000);
    assert.ok(conn);
    conn.socket.destroy();
    assert.equal(existsSync(daemonPaths(dir).disabled), false);
  } finally {
    await disableDaemon({ configDir: dir }).catch(() => {});
    await second;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("installed replace writes the target and does not call spawnDaemonProcess", async () => {
  const dir = tempDir();
  const paths = daemonPaths(dir);
  writeFileSync(paths.supervisor, "{}\n");
  const identity = loadIdentity(packageRootFromScript(cli));
  const dummy = spawn(process.execPath, ["-e", "setInterval(() => {}, 1000)"], { stdio: "ignore" });
  /** @type {import("node:net").Server} */
  const server = createServer((socket) => {
    socket.write(`${JSON.stringify({
      method: "hello",
      params: { ...identity, sourceHash: "old", pid: dummy.pid, port: null },
    })}\n`);
    socket.on("data", (buf) => {
      for (const line of String(buf).split("\n")) {
        if (!line.trim()) continue;
        const msg = JSON.parse(line);
        if (msg.method !== "daemon.stop") continue;
        socket.write(`${JSON.stringify({ id: msg.id, result: { ok: true } })}\n`);
        socket.end();
        server.close(() => dummy.kill());
      }
    });
  });
  /** @type {Promise<void> | undefined} */
  let next;
  try {
    if (process.platform !== "win32") rmSync(paths.sock, { force: true });
    await new Promise((resolve) => server.listen(paths.sock, () => resolve(undefined)));
    let spawns = 0;
    let starts = 0;
    const conn = await ensureDaemon({
      configDir: dir,
      script: cli,
      spawnFn() { spawns += 1; },
      wrapperStart() {
        starts += 1;
        next = (async () => {
          for (let i = 0; i < 50 && existsSync(paths.sock); i += 1) await delay(20);
          await serveDaemon({
            configDir: dir,
            exitProcess: false,
            connect: async ({ port }) => fakeSession(port),
          });
        })();
      },
    });
    assert.equal(spawns, 0);
    assert.equal(starts, 1);
    assert.equal(readDaemonTarget(dir)?.script, cli);
    assert.equal(readDaemonTarget(dir)?.execPath, process.execPath);
    conn.socket.end();
  } finally {
    dummy.kill();
    server.close();
    if (next) {
      for (let i = 0; i < 50 && !existsSync(paths.sock); i += 1) await delay(20);
      await disableDaemon({ configDir: dir }).catch(() => {});
      await Promise.race([next, delay(2000)]);
    }
    rmSync(dir, { recursive: true, force: true });
  }
});

test("uninstall without a supervisor leaves the daemon running", async () => {
  const dir = tempDir();
  const serving = serveDaemon({
    configDir: dir,
    exitProcess: false,
    connect: async ({ port }) => fakeSession(port),
  });
  let stops = 0;
  let removed = false;
  try {
    for (let i = 0; i < 50 && !existsSync(daemonPaths(dir).state); i += 1) await delay(20);
    assert.equal(existsSync(daemonPaths(dir).state), true);
    await uninstallSupervisor({
      configDir: dir,
      registrar: { uninstall() { removed = true; } },
      shutdown: async () => { stops += 1; },
    });
    assert.equal(stops, 0);
    assert.equal(removed, true);
    const conn = await tryConnect(daemonPaths(dir).sock, 500);
    assert.ok(conn);
    conn.socket.destroy();
  } finally {
    await disableDaemon({ configDir: dir }).catch(() => {});
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("uninstall retries stop while the wrapper is alive", async () => {
  const dir = tempDir();
  writeFileSync(daemonPaths(dir).supervisor, "{}\n");
  let stops = 0;
  let ticks = 0;
  try {
    await uninstallSupervisor({
      configDir: dir,
      registrar: { uninstall() {} },
      shutdown: async () => { stops += 1; },
      wrapperAlive: () => stops < 2,
      sleep: async () => { ticks += 1000; },
      now: () => ticks,
    });
    assert.ok(stops > 1);
    assert.equal(existsSync(daemonPaths(dir).disabled), false);
    assert.equal(existsSync(daemonPaths(dir).supervisor), false);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("uninstalled config spawns detached instead of the wrapper", async () => {
  const dir = tempDir();
  let spawns = 0;
  let wrappers = 0;
  try {
    await assert.rejects(
      () => ensureDaemon({
        configDir: dir,
        spawnFn() { spawns += 1; },
        wrapperStart() { wrappers += 1; },
      }),
      /daemon failed to start/,
    );
    assert.equal(spawns, 1);
    assert.equal(wrappers, 0);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});
