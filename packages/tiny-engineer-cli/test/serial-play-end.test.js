import test from "node:test";
import assert from "node:assert/strict";

import { SerialRobotClient } from "../src/robot/serial/client.js";

test("play attempts play_end after stream credit failure", async () => {
  const client = new SerialRobotClient("/dev/null");
  /** @type {{ op: string, fields: Record<string, unknown> }[]} */
  const ops = [];

  client._port = {
    isOpen: true,
    write(_frame, cb) {
      cb(null);
    },
  };

  client.request = async (op, fields = {}) => {
    ops.push({ op, fields });
    if (op === "play") {
      return { ok: true, stream_id: 3, credits: 64 };
    }
    if (op === "play_end") {
      return { ok: true };
    }
    throw new Error(`unexpected op ${op}`);
  };

  client.waitCredit = async () => {
    throw new Error("serial play rejected: over credits");
  };

  await assert.rejects(() => client.play(Buffer.alloc(32), "talking"), /over credits/);

  assert.equal(ops[0]?.op, "play");
  assert.equal(ops.some((entry) => entry.op === "play_end"), true);
  const end = ops.find((entry) => entry.op === "play_end");
  assert.equal(end?.fields.stream_id, 3);
});

test("play cancels credit waiter when port write fails", async () => {
  const client = new SerialRobotClient("/dev/null");
  const writeErr = new Error("write failed");

  client._port = {
    isOpen: true,
    write(_frame, cb) {
      cb(writeErr);
    },
  };

  client.request = async (op) => {
    if (op === "play") {
      return { ok: true, stream_id: 5, credits: 64 };
    }
    if (op === "play_end") {
      return { ok: true };
    }
    throw new Error(`unexpected op ${op}`);
  };

  await assert.rejects(() => client.play(Buffer.alloc(16), "talking"), (err) => {
    assert.equal(err, writeErr);
    return true;
  });

  assert.equal(client._creditWaiters.length, 0);
});

test("play does not double play_end on success", async () => {
  const client = new SerialRobotClient("/dev/null");
  /** @type {string[]} */
  const ops = [];

  client._port = {
    isOpen: true,
    write(_frame, cb) {
      cb(null);
    },
  };

  client.request = async (op) => {
    ops.push(op);
    if (op === "play") {
      return { ok: true, stream_id: 1, credits: 1024 };
    }
    if (op === "play_end") {
      return { ok: true, played_ms: 1 };
    }
    throw new Error(`unexpected op ${op}`);
  };

  client.waitCredit = async () => 1024;

  const result = await client.play(Buffer.alloc(8), "talking");
  assert.equal(result.ok, true);
  assert.deepEqual(
    ops.filter((op) => op === "play_end"),
    ["play_end"],
  );
});
