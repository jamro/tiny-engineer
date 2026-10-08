import test from "node:test";
import assert from "node:assert/strict";
import { applyInstall } from "../src/integrations/claude-code/install.js";

const ctx = {
  projectRoot: "/proj",
  cliCommand: 'node "/x/bin/tiny-engineer.js" hook claude-code',
  timeout: 5,
};

test("applyInstall writes async command shape and is idempotent", () => {
  const once = applyInstall(ctx, null);
  const stop = once.hooks.Stop;
  assert.ok(Array.isArray(stop));
  assert.equal(stop[0].hooks[0].type, "command");
  assert.equal(stop[0].hooks[0].async, true);
  assert.ok(stop[0].hooks[0].command.includes("hook claude-code"));

  const twice = applyInstall(ctx, once);
  assert.equal(twice.hooks.Stop.length, 1);
});

test("applyInstall keeps unrelated settings keys", () => {
  const once = applyInstall(ctx, { permissions: { allow: ["Bash"] }, hooks: {} });
  assert.deepEqual(once.permissions, { allow: ["Bash"] });
  assert.ok(once.hooks.SessionStart);
});
