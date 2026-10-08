import test from "node:test";
import assert from "node:assert/strict";
import { mkdtempSync, mkdirSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
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
    await runDoctor(out, [], { projectRoot: dir });
    assert.ok(lines.some((l) => l.includes("url:")));
    assert.ok(lines.some((l) => l.includes("cursor:")));
  } finally {
    globalThis.fetch = original;
    rmSync(dir, { recursive: true, force: true });
    process.exitCode = 0;
  }
});
