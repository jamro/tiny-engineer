import test from "node:test";
import assert from "node:assert/strict";

import { cobsDecodeFrame, cobsEncodeFrame, CobsFrameDecoder } from "../src/robot/serial/cobs.js";
import { packMessage, parseMessage, SerialMsgType } from "../src/robot/serial/protocol.js";

test("cobs round-trips empty, zeros, and sparse payloads", () => {
  const samples = [Buffer.alloc(0), Buffer.from([0]), Buffer.from([1, 0, 2, 0xff]), Buffer.from([7, 8, 9])];
  for (const sample of samples) {
    const encoded = cobsEncodeFrame(sample);
    assert.equal(encoded[encoded.length - 1], 0);
    const decoded = cobsDecodeFrame(encoded);
    assert.ok(decoded);
    assert.deepEqual([...decoded], [...sample]);
  }
});

test("frame decoder resyncs after malformed input", () => {
  const good = cobsEncodeFrame(Buffer.from([0xaa, 0xbb, 0xcc]));
  /** @type {Buffer[]} */
  const frames = [];
  const decoder = new CobsFrameDecoder();
  decoder.setHandler((p) => frames.push(p));

  decoder.feed(Buffer.from([0x05, 0x00]));
  assert.equal(frames.length, 0);
  assert.equal(decoder.resyncing(), true);

  decoder.feed(good);
  assert.equal(frames.length, 1);
  assert.deepEqual([...frames[0]], [0xaa, 0xbb, 0xcc]);
  assert.equal(decoder.resyncing(), false);
});

test("pack/parse message header", () => {
  const packed = packMessage(SerialMsgType.Req, 42, '{"op":"ping"}');
  const parsed = parseMessage(packed);
  assert.ok(parsed);
  assert.equal(parsed.type, SerialMsgType.Req);
  assert.equal(parsed.id, 42);
  assert.equal(parsed.payload.toString("utf8"), '{"op":"ping"}');
});
