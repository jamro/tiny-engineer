import test from "node:test";
import assert from "node:assert/strict";
import { animationForEvent } from "../src/map.js";

test("PreInvocation maps to thinking", () => {
  assert.equal(animationForEvent("PreInvocation"), "thinking");
});

test("PreToolUse maps reading tools to reading", () => {
  assert.equal(
    animationForEvent("PreToolUse", { toolCall: { name: "view_file" } }),
    "reading"
  );
  assert.equal(
    animationForEvent("PreToolUse", { toolCall: { name: "grep_search" } }),
    "reading"
  );
});

test("PreToolUse maps typing tools to typing", () => {
  assert.equal(
    animationForEvent("PreToolUse", { toolCall: { name: "write_to_file" } }),
    "typing"
  );
  assert.equal(
    animationForEvent("PreToolUse", { toolCall: { name: "replace_file_content" } }),
    "typing"
  );
  assert.equal(
    animationForEvent("PreToolUse", { toolCall: { name: "run_command" } }),
    "typing"
  );
});

test("PostToolUse maps error to attention", () => {
  assert.equal(
    animationForEvent("PostToolUse", { error: "command failed" }),
    "attention"
  );
  assert.equal(animationForEvent("PostToolUse", {}), null);
});

test("Stop maps model_stop to ring", () => {
  assert.equal(
    animationForEvent("Stop", { terminationReason: "model_stop" }),
    "ring"
  );
});

test("Stop maps error to error", () => {
  assert.equal(
    animationForEvent("Stop", { terminationReason: "error" }),
    "error"
  );
  assert.equal(
    animationForEvent("Stop", { error: "fatal failure" }),
    "error"
  );
});

test("Stop maps abort to abort", () => {
  assert.equal(
    animationForEvent("Stop", { terminationReason: "aborted" }),
    "abort"
  );
});
