import test from "node:test";
import assert from "node:assert/strict";

import {
  eventsWebSocketUrl,
  handleWatchSerialMessage,
  redactEventsUrl,
} from "../src/commands/watch.js";
import { packMessage, SerialMsgType } from "../src/robot/serial/protocol.js";

test("eventsWebSocketUrl rewrites http to ws port 81 /events", () => {
  assert.equal(
    eventsWebSocketUrl("http://tiny-engineer.local"),
    "ws://tiny-engineer.local:81/events",
  );
  assert.equal(
    eventsWebSocketUrl("http://robot.example/path", "secret"),
    "ws://robot.example:81/events?token=secret",
  );
});

test("eventsWebSocketUrl rejects https bases", () => {
  assert.throws(
    () => eventsWebSocketUrl("https://robot.example"),
    /http: robot URL/,
  );
  assert.throws(
    () => eventsWebSocketUrl("https://robot.example", "secret"),
    /no TLS on port 81/,
  );
});

test("redactEventsUrl hides token", () => {
  const redacted = redactEventsUrl("ws://host:81/events?token=super-secret");
  assert.match(redacted, /token=\*\*\*/);
  assert.doesNotMatch(redacted, /super-secret/);
});

test("handleWatchSerialMessage prints Evt frames only", () => {
  /** @type {string[]} */
  const lines = [];
  const evt = packMessage(SerialMsgType.Evt, 0, '{"v":1,"type":"button.down"}');
  assert.equal(handleWatchSerialMessage(evt, (l) => lines.push(l)), true);
  assert.deepEqual(lines, ['{"v":1,"type":"button.down"}']);

  const log = packMessage(SerialMsgType.Log, 0, '{"t":"noise"}');
  assert.equal(handleWatchSerialMessage(log, (l) => lines.push(l)), false);
  assert.deepEqual(lines, ['{"v":1,"type":"button.down"}']);
});
