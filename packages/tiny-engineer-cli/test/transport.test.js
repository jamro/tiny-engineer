import test from "node:test";
import assert from "node:assert/strict";
import { resolveTransport } from "../src/robot/transport.js";
import { encodeSerialRequest, parseSerialReply } from "../src/robot/serial.js";

test("resolveTransport prefers --serial over env URL", async () => {
  const prevSerial = process.env.TINY_ENGINEER_SERIAL;
  const prevUrl = process.env.TINY_ENGINEER_URL;
  delete process.env.TINY_ENGINEER_SERIAL;
  process.env.TINY_ENGINEER_URL = "http://from-env";
  try {
    assert.deepEqual(await resolveTransport({ serial: "/dev/cu.test" }), {
      type: "serial",
      path: "/dev/cu.test",
    });
  } finally {
    if (prevSerial === undefined) delete process.env.TINY_ENGINEER_SERIAL;
    else process.env.TINY_ENGINEER_SERIAL = prevSerial;
    if (prevUrl === undefined) delete process.env.TINY_ENGINEER_URL;
    else process.env.TINY_ENGINEER_URL = prevUrl;
  }
});

test("resolveTransport uses TINY_ENGINEER_SERIAL before HTTP", async () => {
  const prevSerial = process.env.TINY_ENGINEER_SERIAL;
  process.env.TINY_ENGINEER_SERIAL = "/dev/cu.env";
  try {
    assert.deepEqual(await resolveTransport({ url: "http://ignored" }), {
      type: "serial",
      path: "/dev/cu.env",
    });
  } finally {
    if (prevSerial === undefined) delete process.env.TINY_ENGINEER_SERIAL;
    else process.env.TINY_ENGINEER_SERIAL = prevSerial;
  }
});

test("resolveTransport falls back to HTTP", async () => {
  const prevSerial = process.env.TINY_ENGINEER_SERIAL;
  delete process.env.TINY_ENGINEER_SERIAL;
  try {
    assert.deepEqual(await resolveTransport({ url: "http://robot.test" }), {
      type: "http",
      baseUrl: "http://robot.test",
    });
  } finally {
    if (prevSerial === undefined) delete process.env.TINY_ENGINEER_SERIAL;
    else process.env.TINY_ENGINEER_SERIAL = prevSerial;
  }
});

test("encodeSerialRequest builds NDJSON op line", () => {
  const { id, line } = encodeSerialRequest({
    id: 7,
    method: "POST",
    path: "/anim",
    query: { name: "thinking" },
  });
  assert.equal(id, 7);
  const parsed = JSON.parse(line);
  assert.equal(parsed.v, 1);
  assert.equal(parsed.id, 7);
  assert.equal(parsed.op, "POST /anim");
  assert.deepEqual(parsed.q, { name: "thinking" });
});

test("parseSerialReply reads R envelope", () => {
  const reply = parseSerialReply(
    'R{"v":1,"id":7,"status":200,"body":{"ok":true,"animation":"thinking"}}\n',
  );
  assert.deepEqual(reply, {
    id: 7,
    status: 200,
    body: { ok: true, animation: "thinking" },
  });
  assert.equal(parseSerialReply("L noise"), null);
});
