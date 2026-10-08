import test from "node:test";
import assert from "node:assert/strict";
import { applyInstall, commandForEvent } from "../src/integrations/antigravity/install.js";

const ctx = {
  projectRoot: "/proj",
  cliCommand: 'node "/x/bin/tiny-engineer.js" hook antigravity',
  timeout: 5,
};

test("commandForEvent appends hook type", () => {
  assert.equal(
    commandForEvent(ctx.cliCommand, "PreToolUse"),
    `${ctx.cliCommand} PreToolUse`,
  );
});

test("applyInstall writes tiny-engineer block with matcher hooks", () => {
  const once = applyInstall(ctx, { other: true });
  assert.equal(once.other, true);
  const block = once["tiny-engineer"];
  assert.ok(block.PreInvocation[0].command.endsWith("PreInvocation"));
  assert.equal(block.PreToolUse[0].matcher, "*");
  assert.ok(block.PreToolUse[0].hooks[0].command.endsWith("PreToolUse"));
  assert.equal(block.Stop[0].timeout, 5);

  const twice = applyInstall(ctx, once);
  assert.equal(twice["tiny-engineer"].Stop.length, 1);
});
