import test from "node:test";
import assert from "node:assert/strict";
import {
  ANIM_NAMES,
  formatRobotError,
  PLAY_ANIM_NAMES,
  postAnim,
  postPlay,
  validateAnimName,
  validatePlayAnimName,
} from "../src/robot/client.js";

/** @type {typeof fetch | undefined} */
let originalFetch;

test.beforeEach(() => {
  originalFetch = globalThis.fetch;
});

test.afterEach(() => {
  globalThis.fetch = originalFetch;
});

/**
 * @param {number} status
 * @param {string} body
 */
function mockFetchResponse(status, body) {
  globalThis.fetch = async () =>
    /** @type {Response} */ ({
      ok: status >= 200 && status < 300,
      status,
      text: async () => body,
    });
}

test("validateAnimName accepts firmware names", () => {
  assert.equal(validateAnimName("ring"), null);
  assert.equal(validateAnimName("typing"), null);
  assert.ok(ANIM_NAMES.has("sleep"));
});

test("validateAnimName rejects unknown names", () => {
  const err = validateAnimName("foobar");
  assert.ok(err?.startsWith("unknown animation: foobar"));
  assert.ok(err?.includes("ring"));
});

test("validatePlayAnimName accepts play path names", () => {
  assert.equal(validatePlayAnimName("talking"), null);
  assert.equal(validatePlayAnimName("none"), null);
  assert.ok(PLAY_ANIM_NAMES.has("thinking"));
});

test("validatePlayAnimName rejects ring", () => {
  const err = validatePlayAnimName("ring");
  assert.ok(err?.startsWith("unknown play animation: ring"));
});

test("postAnim 200 → ok", async () => {
  mockFetchResponse(200, '{"ok":true,"animation":"ring"}');
  const result = await postAnim("http://robot.test", "ring");
  assert.equal(result.ok, true);
  assert.equal(result.status, 200);
  assert.equal(result.body, '{"ok":true,"animation":"ring"}');
});

test("postAnim 400 unknown animation → ok false with parsed error", async () => {
  mockFetchResponse(400, '{"ok":false,"error":"unknown animation"}');
  const result = await postAnim("http://robot.test", "nope");
  assert.equal(result.ok, false);
  assert.equal(result.status, 400);
  assert.equal(result.error, "unknown animation");
  assert.equal(formatRobotError(result), "unknown animation");
});

test("postAnim fetch throw → unreachable", async () => {
  globalThis.fetch = async () => {
    throw new Error("connect ECONNREFUSED");
  };
  const result = await postAnim("http://robot.test", "ring");
  assert.equal(result.ok, false);
  assert.ok(result.error?.startsWith("robot unreachable:"));
  assert.ok(result.error?.includes("ECONNREFUSED"));
});

test("postPlay 415 → ok false with body error", async () => {
  mockFetchResponse(415, '{"ok":false,"error":"expected 16-bit mono PCM at 22050 Hz"}');
  const result = await postPlay({
    baseUrl: "http://robot.test",
    wav: new Uint8Array([1, 2, 3]),
  });
  assert.equal(result.ok, false);
  assert.equal(result.error, "expected 16-bit mono PCM at 22050 Hz");
});

test("postPlay network error → unreachable", async () => {
  globalThis.fetch = async () => {
    throw new Error("timed out");
  };
  const result = await postPlay({
    baseUrl: "http://robot.test",
    wav: new Uint8Array([1]),
    name: "talking",
  });
  assert.equal(result.ok, false);
  assert.equal(formatRobotError(result), "robot unreachable: timed out");
});

test("formatRobotError falls back to HTTP status", () => {
  assert.equal(formatRobotError({ ok: false, status: 503 }), "HTTP 503");
  assert.equal(formatRobotError({ ok: false }), "robot request failed");
});
