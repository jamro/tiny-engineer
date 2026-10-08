import test from "node:test";
import assert from "node:assert/strict";
import { antigravity } from "../src/integrations/antigravity/index.js";
import { defaultResponseForHook } from "../src/integrations/antigravity/map.js";

test("defaultResponseForHook allows PreToolUse and Stop", () => {
  assert.deepEqual(defaultResponseForHook("PreToolUse"), { decision: "allow" });
  assert.deepEqual(defaultResponseForHook("Stop"), { decision: "allow" });
  assert.deepEqual(defaultResponseForHook("PreInvocation"), {});
});

test("antigravity.parseHook never returns null", () => {
  const input = antigravity.parseHook(["PreToolUse"], "{");
  assert.ok(input);
  assert.equal(input.event, "PreToolUse");
});

test("antigravity.respond prints decision JSON", () => {
  /** @type {string[]} */
  const lines = [];
  const original = console.log;
  console.log = (msg) => lines.push(String(msg));
  try {
    const input = antigravity.parseHook(["Stop"], "{}");
    antigravity.respond({ animPosted: true, anim: "ring", input });
    assert.deepEqual(lines, ['{"decision":"allow"}']);
  } finally {
    console.log = original;
  }
});
