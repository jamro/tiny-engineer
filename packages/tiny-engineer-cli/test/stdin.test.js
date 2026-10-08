import test from "node:test";
import assert from "node:assert/strict";
import { PassThrough } from "node:stream";
import { readStdin } from "../src/util/stdin.js";

test("readStdin resolves with chunks when pipe never ends", async () => {
  const stream = new PassThrough();
  const pending = readStdin({ timeoutMs: 50, stream });
  stream.write('{"a":1}');
  const text = await pending;
  assert.equal(text, '{"a":1}');
  stream.destroy();
});

test("readStdin resolves on end before timeout", async () => {
  const stream = new PassThrough();
  const pending = readStdin({ timeoutMs: 5000, stream });
  stream.write('{"hook_event_name":"stop"}');
  stream.end();
  const text = await pending;
  assert.equal(text, '{"hook_event_name":"stop"}');
});

test("readStdin returns empty string for TTY", async () => {
  const stream = new PassThrough();
  stream.isTTY = true;
  const text = await readStdin({ timeoutMs: 50, stream });
  assert.equal(text, "");
});
