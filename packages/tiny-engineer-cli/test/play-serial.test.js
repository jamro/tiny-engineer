import test from "node:test";
import assert from "node:assert/strict";
import { mkdtempSync, writeFileSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { run } from "../src/cli.js";

test("play rejects serial transport", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-play-"));
  const wav = join(dir, "clip.wav");
  writeFileSync(wav, Buffer.alloc(44));
  const prev = process.env.TINY_ENGINEER_SERIAL;
  delete process.env.TINY_ENGINEER_SERIAL;

  /** @type {string[]} */
  const errors = [];
  const originalError = console.error;
  console.error = (...args) => errors.push(args.map(String).join(" "));

  try {
    process.exitCode = 0;
    await run(["play", wav, "--serial", "/dev/cu.test"]);
    assert.equal(process.exitCode, 1);
    assert.ok(errors.some((e) => e.includes("play requires HTTP")));
  } finally {
    console.error = originalError;
    if (prev === undefined) delete process.env.TINY_ENGINEER_SERIAL;
    else process.env.TINY_ENGINEER_SERIAL = prev;
    rmSync(dir, { recursive: true, force: true });
    process.exitCode = 0;
  }
});
