import test from "node:test";
import assert from "node:assert/strict";
import { mkdtempSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

import { loadDotEnv, resolveTransport } from "../src/config/env.js";

test("resolveTransport defaults to http", () => {
  const prev = process.env.TINY_ENGINEER_TRANSPORT;
  delete process.env.TINY_ENGINEER_TRANSPORT;
  try {
    assert.deepEqual(resolveTransport(undefined), { transport: "http" });
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_TRANSPORT;
    else process.env.TINY_ENGINEER_TRANSPORT = prev;
  }
});

test("resolveTransport prefers flag over env", () => {
  const prev = process.env.TINY_ENGINEER_TRANSPORT;
  process.env.TINY_ENGINEER_TRANSPORT = "serial";
  try {
    assert.deepEqual(resolveTransport("http"), { transport: "http" });
    assert.deepEqual(resolveTransport(undefined), { transport: "serial" });
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_TRANSPORT;
    else process.env.TINY_ENGINEER_TRANSPORT = prev;
  }
});

test("resolveTransport rejects invalid values", () => {
  assert.equal(resolveTransport("ble").error?.includes("invalid --transport"), true);
  const prev = process.env.TINY_ENGINEER_TRANSPORT;
  process.env.TINY_ENGINEER_TRANSPORT = "ble";
  try {
    assert.equal(resolveTransport(undefined).error?.includes("TINY_ENGINEER_TRANSPORT"), true);
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_TRANSPORT;
    else process.env.TINY_ENGINEER_TRANSPORT = prev;
  }
});

test("setup-written .env transport is loaded for later commands", () => {
  const dir = mkdtempSync(join(tmpdir(), "te-transport-"));
  const prev = process.env.TINY_ENGINEER_TRANSPORT;
  delete process.env.TINY_ENGINEER_TRANSPORT;
  try {
    writeFileSync(join(dir, ".env"), "TINY_ENGINEER_TRANSPORT=serial\n", "utf8");
    loadDotEnv(dir);
    assert.deepEqual(resolveTransport(undefined), { transport: "serial" });
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_TRANSPORT;
    else process.env.TINY_ENGINEER_TRANSPORT = prev;
    rmSync(dir, { recursive: true, force: true });
  }
});
