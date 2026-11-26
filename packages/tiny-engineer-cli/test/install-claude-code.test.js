import test from "node:test";
import assert from "node:assert/strict";
import { join } from "node:path";
import { applyInstall, planInstall, planUninstall } from "../src/integrations/claude-code/install.js";

const ctx = {
  projectRoot: "/proj",
  cliCommand: 'node "/x/bin/tiny-engineer.js" hook claude-code',
  timeout: 5,
};

test("planInstall user scope targets home claude settings", () => {
  const plan = planInstall({ ...ctx, scope: "user", home: "/home/me" });
  const path = join("/home/me", ".claude", "settings.json");
  assert.equal(plan.path, path);
  assert.equal(plan.relativePath, path);
  assert.equal(plan.ide, "claude-code");
});

test("planInstall project scope stays in the project", () => {
  const plan = planInstall(ctx);
  assert.equal(plan.path, join("/proj", ".claude", "settings.json"));
  assert.equal(plan.relativePath, ".claude/settings.json");
});

test("planUninstall user scope reads home claude settings", () => {
  const plan = planUninstall("/proj", { scope: "user", home: "/home/me" });
  assert.equal(plan.path, join("/home/me", ".claude", "settings.json"));
  assert.equal(plan.removeCount, 0);
});

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
