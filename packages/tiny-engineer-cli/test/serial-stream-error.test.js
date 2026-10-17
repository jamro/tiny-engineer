import test from "node:test";
import assert from "node:assert/strict";

import { SerialRobotClient } from "../src/robot/serial/client.js";
import { cobsEncodeFrame } from "../src/robot/serial/cobs.js";
import {
  packMessage,
  packStreamPayload,
  SerialMsgType,
  SerialStreamKind,
} from "../src/robot/serial/protocol.js";

test("waitCredit rejects on stream Error frame with reason text", async () => {
  const client = new SerialRobotClient("/dev/null");

  const streamId = 2;
  const seq = 7;
  const creditPromise = client.waitCredit(streamId, seq, 5000);

  const payload = packStreamPayload(
    streamId,
    SerialStreamKind.Error,
    seq,
    Buffer.from("over credits"),
  );
  const message = packMessage(SerialMsgType.Stream, 0, payload);
  const frame = cobsEncodeFrame(message);

  client._decoder.feed(frame);

  await assert.rejects(creditPromise, (err) => {
    assert.match(String(err.message), /serial play rejected/);
    assert.match(String(err.message), /over credits/);
    return true;
  });
});
