import assert from "node:assert/strict";
import { chmodSync, existsSync, mkdirSync, mkdtempSync, readFileSync, realpathSync, rmSync, statSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { isAbsolute, join } from "node:path";
import { spawn, spawnSync } from "node:child_process";
import { createServer } from "node:net";
import test from "node:test";
import { fileURLToPath } from "node:url";

import { rulesHelp, runRules } from "../src/commands/rules.js";
import { daemonPaths } from "../src/daemon/paths.js";
import { delay } from "../src/daemon/proc.js";
import { serveDaemon } from "../src/daemon/server.js";
import { disableDaemon } from "../src/daemon/client.js";
import { createOutput } from "../src/util/output.js";

const cli = fileURLToPath(new URL("../bin/tiny-engineer.js", import.meta.url));

function freshDir() {
  return mkdtempSync(join(existsSync("/tmp") ? "/tmp" : tmpdir(), "te-rules-"));
}

function quietOut() {
  return createOutput({ quiet: true, log: () => {}, error: () => {} });
}

test("documented add syntax matches the parser", () => {
  const help = rulesHelp();
  assert.match(help, /rules add <type> -- <program> \[args\.\.\.\]/);
  assert.match(help, /absolute path/);
  assert.match(help, /relative program\npath is resolved from the current directory/);
  assert.match(help, /login agent/);
  const top = spawnSync(process.execPath, [cli, "--help"], { encoding: "utf8" });
  assert.equal(top.status, 0);
  assert.match(top.stdout, /rules list\|add\|remove/);
  assert.match(top.stdout, /rules add <type> -- <program> \[args\.\.\.\]/);
  assert.match(top.stdout, /absolute program path/);
  assert.match(top.stdout, /relative program path/);
  assert.match(top.stdout, /sh -c scripts see the login agent's PATH/);
  assert.match(top.stdout, /HTTP anim and hooks do not go through the daemon/);
  const integration = readFileSync(fileURLToPath(new URL("../../../docs/integration.md", import.meta.url)), "utf8");
  assert.match(integration, /tiny-engineer rules add button\.click -- \/usr\/bin\/open -a Music/);
  assert.match(integration, /sh -c/);
  assert.match(integration, /HTTP `anim` and hooks do not go through the daemon/);
});

test("add stores an absolute path, refuses a missing program, and does not install", async () => {
  const dir = freshDir();
  const bin = join(dir, "bin");
  mkdirSync(bin);
  const program = join(bin, "my-tool");
  writeFileSync(program, "#!/bin/sh\nexit 0\n");
  chmodSync(program, 0o755);
  const out = quietOut();
  try {
    await runRules(out, ["add", "button.click", "--", "missing-tool"], {
      configDir: dir,
      env: { PATH: bin },
      signal: () => {},
    });
    assert.equal(process.exitCode, 1);
    process.exitCode = 0;
    assert.equal(existsSync(daemonPaths(dir).rules), false);
    assert.equal(existsSync(daemonPaths(dir).supervisor), false);
    await runRules(out, ["add", "button.click", "--", "my-tool", "--flag"], {
      configDir: dir,
      env: { PATH: bin },
      signal: () => {},
    });
    assert.equal(process.exitCode ?? 0, 0);
    const saved = JSON.parse(readFileSync(daemonPaths(dir).rules, "utf8"));
    assert.deepEqual(saved.rules[0].run, [program, "--flag"]);
    if (process.platform !== "win32") {
      assert.equal(statSync(daemonPaths(dir).rules).mode & 0o777, 0o600);
    }
    assert.equal(existsSync(daemonPaths(dir).supervisor), false);
    const before = readFileSync(daemonPaths(dir).rules, "utf8");
    await runRules(out, ["remove", "9"], { configDir: dir, signal: () => {} });
    assert.equal(process.exitCode, 1);
    process.exitCode = 0;
    assert.equal(readFileSync(daemonPaths(dir).rules, "utf8"), before);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("add resolves a relative program path from the current directory", () => {
  const dir = freshDir();
  const bin = join(dir, "bin");
  mkdirSync(bin);
  const program = join(dir, "script.sh");
  writeFileSync(program, "#!/bin/sh\nexit 0\n");
  chmodSync(program, 0o755);
  const decoy = join(bin, "script.sh");
  writeFileSync(decoy, "#!/bin/sh\nexit 0\n");
  chmodSync(decoy, 0o755);
  const env = { ...process.env, PATH: bin, TINY_ENGINEER_CONFIG_DIR: dir };
  const run = (programArg) => spawnSync(
    process.execPath,
    [cli, "rules", "add", "button.click", "--", programArg],
    { cwd: dir, env, encoding: "utf8" },
  );
  try {
    const added = run("./script.sh");
    assert.equal(added.status, 0, added.stderr);
    const saved = JSON.parse(readFileSync(daemonPaths(dir).rules, "utf8"));
    assert.equal(saved.rules.length, 1);
    assert.equal(isAbsolute(saved.rules[0].run[0]), true);
    assert.equal(realpathSync(saved.rules[0].run[0]), realpathSync(program));
    assert.notEqual(realpathSync(saved.rules[0].run[0]), realpathSync(decoy));

    const missing = run("./missing.sh");
    assert.equal(missing.status, 1);
    assert.match(missing.stderr, /program not found: \.\/missing\.sh/);
    assert.equal(JSON.parse(readFileSync(daemonPaths(dir).rules, "utf8")).rules.length, 1);

    if (process.platform !== "win32") {
      const plain = join(dir, "plain.sh");
      writeFileSync(plain, "#!/bin/sh\nexit 0\n");
      chmodSync(plain, 0o644);
      const denied = run("./plain.sh");
      assert.equal(denied.status, 1);
      assert.equal(JSON.parse(readFileSync(daemonPaths(dir).rules, "utf8")).rules.length, 1);
    }
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("add reloads a live daemon", async () => {
  const dir = freshDir();
  const hits = join(dir, "hits");
  writeFileSync(join(dir, "env"), "TINY_ENGINEER_TRANSPORT=serial\n");
  /** @type {ReturnType<typeof fakeClient> | null} */
  let client = null;
  const serving = serveDaemon({
    configDir: dir,
    exitProcess: false,
    connect: async () => {
      client = fakeClient();
      return { port: "/dev/fake", client };
    },
  });
  try {
    const startState = Date.now();
    while (!existsSync(daemonPaths(dir).state) && Date.now() - startState < 2000) await delay(20);
    const script = `require('fs').appendFileSync(${JSON.stringify(hits)}, 'hit\\n')`;
    await runRules(quietOut(), ["add", "button.click", "--", process.execPath, "-e", script], {
      configDir: dir,
    });
    const start = Date.now();
    while (!client && Date.now() - start < 2000) await delay(20);
    client?.emit(JSON.stringify({ v: 1, type: "button.click", source: "push-button", ms: 1 }));
    const wait = Date.now();
    while (!existsSync(hits) && Date.now() - wait < 2000) await delay(20);
    assert.equal(readFileSync(hits, "utf8").trim(), "hit");
  } finally {
    await disableDaemon({ configDir: dir });
    await serving;
    rmSync(dir, { recursive: true, force: true });
  }
});

/**
 * @param {string[]} [extraArgs]
 */
function sleeper(extraArgs = []) {
  return spawn(process.execPath, ["-e", "setInterval(() => {}, 1000)", ...extraArgs], {
    stdio: "ignore",
  });
}

/**
 * @param {string} sockPath
 * @param {number} pid
 */
function legacyDaemon(sockPath, pid) {
  const server = createServer((socket) => {
    socket.write(`${JSON.stringify({ method: "hello", params: { pid } })}\n`);
    let buf = "";
    socket.on("data", (chunk) => {
      buf += chunk.toString("utf8");
      const nl = buf.indexOf("\n");
      if (nl < 0) return;
      /** @type {{ id?: number }} */
      let msg;
      try {
        msg = JSON.parse(buf.slice(0, nl));
      } catch {
        return;
      }
      socket.write(`${JSON.stringify({ id: msg.id, error: "unknown method reactions.sync" })}\n`);
    });
  });
  return new Promise((resolve, reject) => {
    server.once("error", reject);
    server.listen(sockPath, () => resolve(server));
  });
}

test("stale daemon pid is not signaled when the socket is down", async () => {
  const dir = freshDir();
  const child = sleeper();
  writeFileSync(daemonPaths(dir).state, JSON.stringify({ pid: child.pid }), { mode: 0o600 });
  try {
    await runRules(quietOut(), ["add", "button.click", "--", process.execPath], { configDir: dir });
    assert.equal(process.exitCode ?? 0, 0);
    await delay(100);
    assert.equal(child.exitCode, null);
    assert.equal(child.signalCode, null);
  } finally {
    child.kill("SIGKILL");
    rmSync(dir, { recursive: true, force: true });
  }
});

test("unknown method does not signal a foreign pid", async () => {
  const dir = freshDir();
  const child = sleeper();
  assert.equal(typeof child.pid, "number");
  const server = await legacyDaemon(daemonPaths(dir).sock, /** @type {number} */ (child.pid));
  try {
    await runRules(quietOut(), ["add", "button.click", "--", process.execPath], { configDir: dir });
    assert.equal(process.exitCode ?? 0, 0);
    await delay(100);
    assert.equal(child.exitCode, null);
    assert.equal(child.signalCode, null);
  } finally {
    child.kill("SIGKILL");
    await new Promise((resolve) => server.close(() => resolve(undefined)));
    rmSync(dir, { recursive: true, force: true });
  }
});

test("unknown method reloads a legacy daemon with SIGHUP", { skip: process.platform === "win32" }, async () => {
  const dir = freshDir();
  const child = sleeper(["tiny-engineer", "daemon"]);
  assert.equal(typeof child.pid, "number");
  const server = await legacyDaemon(daemonPaths(dir).sock, /** @type {number} */ (child.pid));
  try {
    await runRules(quietOut(), ["add", "button.click", "--", process.execPath], { configDir: dir });
    assert.equal(process.exitCode ?? 0, 0);
    const start = Date.now();
    while (child.exitCode === null && child.signalCode === null && Date.now() - start < 2000) {
      await delay(20);
    }
    assert.equal(child.signalCode, "SIGHUP");
  } finally {
    child.kill("SIGKILL");
    await new Promise((resolve) => server.close(() => resolve(undefined)));
    rmSync(dir, { recursive: true, force: true });
  }
});

/**
 * @returns {{ emit: (line: string) => void, setEventHandler: (fn: (payload: Buffer) => void) => void }}
 */
function fakeClient() {
  /** @type {(payload: Buffer) => void} */
  let onEvent = () => {};
  return {
    request: async () => ({ ok: true }),
    play: async () => ({ ok: true }),
    /**
     * @param {(payload: Buffer) => void} fn
     */
    setEventHandler(fn) {
      onEvent = fn;
    },
    setLogHandler() {},
    setCloseHandler() {},
    close: async () => {},
    /**
     * @param {string} line
     */
    emit(line) {
      onEvent(Buffer.from(line));
    },
  };
}
