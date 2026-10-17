import test from "node:test";
import assert from "node:assert/strict";

import { handleMonitorMessage, logTextFromPayload } from "../src/commands/monitor.js";
import { packMessage, SerialMsgType } from "../src/robot/serial/protocol.js";

test("logTextFromPayload reads t field", () => {
  assert.equal(logTextFromPayload(Buffer.from('{"t":"hello"}')), "hello");
  assert.equal(logTextFromPayload(Buffer.from("{}")), null);
});

test("handleMonitorMessage prints LOG frames only", () => {
  /** @type {string[]} */
  const lines = [];
  const logMsg = packMessage(SerialMsgType.Log, 0, '{"t":"boot ok"}');
  assert.equal(handleMonitorMessage(logMsg, (l) => lines.push(l)), true);
  assert.deepEqual(lines, ["boot ok"]);

  const resMsg = packMessage(SerialMsgType.Res, 1, '{"ok":true}');
  assert.equal(handleMonitorMessage(resMsg, (l) => lines.push(l)), false);
  assert.deepEqual(lines, ["boot ok"]);
});
