import assert from "node:assert/strict";
import { existsSync, mkdirSync, mkdtempSync, rmSync, writeFileSync } from "node:fs";
import { createServer } from "node:net";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";

import { daemonPaths } from "../src/daemon/paths.js";
import { delay } from "../src/daemon/proc.js";
import { writeDaemonTarget } from "../src/daemon/target.js";
import { runWrapper, wrapperAlive } from "../src/daemon/wrapper.js";

function tempDir() {
  const root = existsSync("/tmp") ? "/tmp" : tmpdir();
  return mkdtempSync(join(root, "te-wrap-"));
}

test("wrapper restarts after hello and stops when disabled", async () => {
  const dir = tempDir();
  writeFileSync(daemonPaths(dir).supervisor, "{}\n");
  writeDaemonTarget(dir, { execPath: process.execPath, script: "/bin/tiny-engineer.js" });
  let n = 0;
  try {
    const code = await runWrapper({
      configDir: dir,
      spawnChild() {
        n += 1;
        if (n === 2) writeFileSync(daemonPaths(dir).disabled, "1\n");
        return { exited: Promise.resolve(1) };
      },
      accepted: async () => true,
      sleep: async () => {},
      now: () => 0,
    });
    assert.equal(n, 2);
    assert.equal(code, 0);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("wrapper keeps retrying exits before hello until disabled", async () => {
  const dir = tempDir();
  writeFileSync(daemonPaths(dir).supervisor, "{}\n");
  writeDaemonTarget(dir, { execPath: process.execPath, script: "/bin/tiny-engineer.js" });
  let n = 0;
  try {
    const code = await runWrapper({
      configDir: dir,
      spawnChild() {
        n += 1;
        if (n === 5) writeFileSync(daemonPaths(dir).disabled, "1\n");
        return { exited: Promise.resolve(1) };
      },
      accepted: async () => false,
      sleep: async () => {},
      now: () => 1000,
    });
    assert.equal(n, 5);
    assert.equal(code, 0);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("missing supervisor file does not spawn", async () => {
  const dir = tempDir();
  writeDaemonTarget(dir, { execPath: process.execPath, script: "/bin/tiny-engineer.js" });
  let n = 0;
  try {
    const code = await runWrapper({
      configDir: dir,
      spawnChild() {
        n += 1;
        return { exited: Promise.resolve(1) };
      },
    });
    assert.equal(n, 0);
    assert.equal(code, 0);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("wrapper stops when the supervisor file is removed", async () => {
  const dir = tempDir();
  writeFileSync(daemonPaths(dir).supervisor, "{}\n");
  writeDaemonTarget(dir, { execPath: process.execPath, script: "/bin/tiny-engineer.js" });
  let n = 0;
  try {
    const code = await runWrapper({
      configDir: dir,
      spawnChild() {
        n += 1;
        rmSync(daemonPaths(dir).supervisor, { force: true });
        return { exited: Promise.resolve(1) };
      },
      accepted: async () => true,
      sleep: async () => {},
      now: () => 0,
    });
    assert.equal(n, 1);
    assert.equal(code, 0);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("second wrapper exits while the lock is held", async () => {
  const dir = tempDir();
  writeFileSync(daemonPaths(dir).supervisor, "{}\n");
  writeDaemonTarget(dir, { execPath: process.execPath, script: "/bin/tiny-engineer.js" });
  /** @type {(code: number | null) => void} */
  let release = () => {};
  const exited = new Promise((resolve) => {
    release = resolve;
  });
  let firstSpawns = 0;
  const first = runWrapper({
    configDir: dir,
    spawnChild() {
      firstSpawns += 1;
      return { exited };
    },
    accepted: async () => true,
    sleep: async () => {},
    now: () => 0,
  });
  try {
    for (let i = 0; i < 50 && !(await wrapperAlive(dir)); i += 1) await delay(20);
    assert.equal(await wrapperAlive(dir), true);
    let secondSpawns = 0;
    const code = await runWrapper({
      configDir: dir,
      spawnChild() {
        secondSpawns += 1;
        return { exited: Promise.resolve(0) };
      },
    });
    assert.equal(code, 0);
    assert.equal(secondSpawns, 0);
    assert.equal(firstSpawns, 1);
  } finally {
    rmSync(daemonPaths(dir).supervisor, { force: true });
    release(0);
    await first;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("stale wrapper lock file does not block startup", { skip: process.platform === "win32" }, async () => {
  const dir = tempDir();
  writeFileSync(daemonPaths(dir).supervisor, "{}\n");
  writeDaemonTarget(dir, { execPath: process.execPath, script: "/bin/tiny-engineer.js" });
  writeFileSync(daemonPaths(dir).wrapperLock, "stale");
  let n = 0;
  try {
    const code = await runWrapper({
      configDir: dir,
      spawnChild() {
        n += 1;
        rmSync(daemonPaths(dir).supervisor, { force: true });
        return { exited: Promise.resolve(1) };
      },
      accepted: async () => true,
      sleep: async () => {},
      now: () => 0,
    });
    assert.equal(n, 1);
    assert.equal(code, 0);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("stale socket hello does not count as the child", async () => {
  const dir = tempDir();
  writeFileSync(daemonPaths(dir).supervisor, "{}\n");
  writeDaemonTarget(dir, { execPath: process.execPath, script: "/bin/tiny-engineer.js" });
  /** @type {import("node:net").Socket[]} */
  const sockets = [];
  const server = createServer((socket) => {
    sockets.push(socket);
    socket.write('{"method":"hello","params":{"pid":1}}\n');
  });
  await new Promise((resolve) => {
    server.listen(daemonPaths(dir).sock, () => resolve(undefined));
  });
  let n = 0;
  try {
    const code = await runWrapper({
      configDir: dir,
      spawnChild() {
        n += 1;
        if (n === 3) writeFileSync(daemonPaths(dir).disabled, "1\n");
        return { exited: Promise.resolve(1), pid: 424242 };
      },
      sleep: async () => {},
      now: () => 1000,
    });
    assert.equal(n, 3);
    assert.equal(code, 0);
  } finally {
    for (const socket of sockets) socket.destroy();
    await new Promise((resolve) => {
      server.close(() => resolve(undefined));
    });
    rmSync(dir, { recursive: true, force: true });
  }
});

test("disabled file does not spawn", async () => {
  const dir = tempDir();
  mkdirSync(dir, { recursive: true });
  writeFileSync(daemonPaths(dir).supervisor, "{}\n");
  writeFileSync(daemonPaths(dir).disabled, "1\n");
  let n = 0;
  try {
    const code = await runWrapper({
      configDir: dir,
      spawnChild() {
        n += 1;
        return { exited: Promise.resolve(1) };
      },
    });
    assert.equal(n, 0);
    assert.equal(code, 0);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});
