import test from "node:test";
import assert from "node:assert/strict";
import { existsSync, mkdtempSync, mkdirSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { stopDaemon } from "../src/daemon/client.js";
import { daemonPaths } from "../src/daemon/paths.js";
import { delay } from "../src/daemon/proc.js";
import { serveDaemon } from "../src/daemon/server.js";
import { createOutput } from "../src/util/output.js";
import { createStyle } from "../src/util/style.js";
import { checkHealth, configHasHook, runDoctor } from "../src/wizard/doctor.js";

test("configHasHook finds hook cursor command", () => {
  const dir = mkdtempSync(join(tmpdir(), "te-doc-"));
  try {
    mkdirSync(join(dir, ".cursor"), { recursive: true });
    writeFileSync(
      join(dir, ".cursor", "hooks.json"),
      JSON.stringify({
        version: 1,
        hooks: { stop: [{ command: 'node "/x" hook cursor', timeout: 5 }] },
      }),
      "utf8",
    );
    assert.equal(configHasHook(dir, "cursor", ".cursor/hooks.json"), true);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("checkHealth returns ok on 200", async () => {
  const original = globalThis.fetch;
  globalThis.fetch = async () =>
    /** @type {Response} */ ({
      ok: true,
      status: 200,
      text: async () => '{"ok":true}',
    });
  try {
    const result = await checkHealth("http://robot.test");
    assert.equal(result.ok, true);
  } finally {
    globalThis.fetch = original;
  }
});

test("checkHealth returns fail on throw", async () => {
  const original = globalThis.fetch;
  globalThis.fetch = async () => {
    throw new Error("offline");
  };
  try {
    const result = await checkHealth("http://robot.test");
    assert.equal(result.ok, false);
    assert.ok(result.detail.includes("offline"));
  } finally {
    globalThis.fetch = original;
  }
});

test("runDoctor prints without throwing", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-doc-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
  const original = globalThis.fetch;
  globalThis.fetch = async () => {
    throw new Error("offline");
  };
  /** @type {string[]} */
  const lines = [];
  const out = createOutput({
    quiet: false,
    style: createStyle({ color: false }),
    log: (m) => lines.push(String(m)),
    error: (m) => lines.push(String(m)),
  });
  try {
    writeFileSync(join(dir, ".env"), "TINY_ENGINEER_URL=http://override\nTINY_ENGINEER_TOKEN=sekrit\n", "utf8");
    await runDoctor(out, [], { projectRoot: dir });
    assert.ok(lines.some((l) => l.includes("url:")));
    assert.ok(lines.some((l) => l.includes("cursor:")));
    assert.ok(lines.some((l) => l.includes("user config:")));
    assert.ok(lines.some((l) => l.includes("overrides") && l.includes("TINY_ENGINEER_URL")));
    assert.ok(lines.some((l) => l.includes("TINY_ENGINEER_TOKEN (set)")));
    assert.ok(!lines.some((l) => l.includes("sekrit")));
  } finally {
    globalThis.fetch = original;
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("runDoctor serial uses the daemon and does not need HTTP", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-doc-"));
  const cfg = mkdtempSync(join(existsSync("/tmp") ? "/tmp" : tmpdir(), "te-doc-cfg-"));
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
  const original = globalThis.fetch;
  let fetches = 0;
  globalThis.fetch = async () => {
    fetches += 1;
    throw new Error("http");
  };
  /** @type {string[]} */
  const lines = [];
  const out = createOutput({
    quiet: false,
    style: createStyle({ color: false }),
    log: (m) => lines.push(String(m)),
    error: (m) => lines.push(String(m)),
  });
  const serving = serveDaemon({
    configDir: cfg,
    exitProcess: false,
    connect: async () => ({
      port: "/dev/fake",
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
    for (let i = 0; i < 50 && !existsSync(daemonPaths(cfg).sock); i += 1) await delay(20);
    await runDoctor(out, ["--transport", "serial"], { projectRoot: dir });
    assert.equal(fetches, 0);
    assert.ok(lines.some((line) => line.includes("transport: serial")));
    assert.ok(lines.some((line) => line.includes("health: ok")));
    assert.equal(lines.some((line) => line.includes("url:")), false);
  } finally {
    globalThis.fetch = original;
    await stopDaemon({ configDir: cfg }).catch(() => {});
    await serving;
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
    process.exitCode = 0;
  }
});
