import test from "node:test";
import assert from "node:assert/strict";
import { mkdtempSync, writeFileSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { run } from "../src/cli.js";
import { parsePlayArgs } from "../src/util/args.js";
import { encodeSerialPlayBegin, writeSerialPlayBody } from "../src/robot/serial.js";

test("encodeSerialPlayBegin includes op and len", () => {
  const { id, line } = encodeSerialPlayBegin({ id: 7, len: 123456 });
  assert.equal(id, 7);
  const json = JSON.parse(line);
  assert.equal(json.v, 1);
  assert.equal(json.id, 7);
  assert.equal(json.op, "POST /play");
  assert.equal(json.len, 123456);
});

test("encodeSerialPlayBegin with name uses /play/<name>", () => {
  const { line } = encodeSerialPlayBegin({ id: 1, name: "thinking", len: 44 });
  const json = JSON.parse(line);
  assert.equal(json.op, "POST /play/thinking");
  assert.equal(json.len, 44);
});

test("writeSerialPlayBody writes paced chunks then finishes", async () => {
  /** @type {Buffer[]} */
  const written = [];
  const fakePort = {
    write(chunk, cb) {
      written.push(Buffer.from(chunk));
      queueMicrotask(() => cb(null));
    },
    drain(cb) {
      queueMicrotask(() => cb(null));
    },
  };
  const wav = Buffer.alloc(5000, 1);
  await writeSerialPlayBody(fakePort, wav);
  const total = written.reduce((n, b) => n + b.length, 0);
  assert.equal(total, wav.length);
  assert.ok(written.length >= 2);
});

test("parsePlayArgs accepts --serial", () => {
  const opts = parsePlayArgs(["clip.wav", "--serial", "/dev/cu.test"]);
  assert.equal(opts.wavPath, "clip.wav");
  assert.equal(opts.serial, "/dev/cu.test");
  assert.equal(opts.error, undefined);
});

test("play with --serial does not reject as HTTP-only", async () => {
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
    // Port does not exist → unreachable, not "play requires HTTP".
    await run(["play", wav, "--serial", "/dev/cu.tiny-engineer-missing-port"]);
    assert.equal(process.exitCode, 1);
    const joined = errors.join("\n");
    assert.ok(!joined.includes("play requires HTTP"));
    assert.ok(joined.includes("robot unreachable") || joined.includes("cannot open") || joined.length > 0);
  } finally {
    console.error = originalError;
    if (prev === undefined) delete process.env.TINY_ENGINEER_SERIAL;
    else process.env.TINY_ENGINEER_SERIAL = prev;
    rmSync(dir, { recursive: true, force: true });
    process.exitCode = 0;
  }
});
