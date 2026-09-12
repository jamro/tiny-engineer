import test from "node:test";
import assert from "node:assert/strict";
import { animationForEvent } from "../src/map.js";

test("EVENT_ANIM maps reading events to reading", () => {
  assert.equal(
    animationForEvent({ hook_event_name: "sessionStart" }),
    "reading"
  );
  assert.equal(
    animationForEvent({ hook_event_name: "beforeSubmitPrompt" }),
    "reading"
  );
  assert.equal(
    animationForEvent({ hook_event_name: "beforeReadFile" }),
    "reading"
  );
});

test("EVENT_ANIM maps thinking events to thinking", () => {
  assert.equal(
    animationForEvent({ hook_event_name: "afterAgentThought" }),
    "thinking"
  );
  assert.equal(
    animationForEvent({ hook_event_name: "preCompact" }),
    "thinking"
  );
});

test("EVENT_ANIM maps typing events to typing", () => {
  assert.equal(
    animationForEvent({ hook_event_name: "beforeShellExecution" }),
    "typing"
  );
  assert.equal(
    animationForEvent({ hook_event_name: "subagentStart" }),
    "typing"
  );
  assert.equal(
    animationForEvent({ hook_event_name: "afterFileEdit" }),
    "typing"
  );
});

test("preToolUse maps reading tools to reading", () => {
  assert.equal(
    animationForEvent({ hook_event_name: "preToolUse", tool_name: "Read" }),
    "reading"
  );
  assert.equal(
    animationForEvent({ hook_event_name: "preToolUse", tool_name: "Grep" }),
    "reading"
  );
});

test("preToolUse maps typing tools to typing", () => {
  assert.equal(
    animationForEvent({ hook_event_name: "preToolUse", tool_name: "Write" }),
    "typing"
  );
  assert.equal(
    animationForEvent({ hook_event_name: "preToolUse", tool_name: "StrReplace" }),
    "typing"
  );
  assert.equal(
    animationForEvent({ hook_event_name: "preToolUse", tool_name: "Shell" }),
    "typing"
  );
});

test("preToolUse skips unknown tools", () => {
  assert.equal(
    animationForEvent({
      hook_event_name: "preToolUse",
      tool_name: "AskQuestion",
    }),
    null
  );
});

test("stop maps aborted to abort", () => {
  assert.equal(
    animationForEvent({ hook_event_name: "stop", status: "aborted" }),
    "abort"
  );
});

test("stop maps error to error", () => {
  assert.equal(
    animationForEvent({ hook_event_name: "stop", status: "error" }),
    "error"
  );
});

test("stop maps completed or missing status to ring", () => {
  assert.equal(
    animationForEvent({ hook_event_name: "stop", status: "completed" }),
    "ring"
  );
  assert.equal(animationForEvent({ hook_event_name: "stop" }), "ring");
});

test("skips missing or unknown hook_event_name", () => {
  assert.equal(animationForEvent(undefined), null);
  assert.equal(animationForEvent({}), null);
  assert.equal(animationForEvent({ hook_event_name: "" }), null);
  assert.equal(
    animationForEvent({ hook_event_name: "afterFileEditFoo" }),
    null
  );
});
