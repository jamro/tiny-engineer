import test from "node:test";
import assert from "node:assert/strict";
import { join } from "node:path";
import { applyInstall, planInstall, planUninstall } from "../src/integrations/cursor/install.js";

const ctx = {
  projectRoot: "/proj",
  cliCommand: 'node "/proj/bin/tiny-engineer.js" hook cursor',
  timeout: 5,
};

test("planInstall lists cursor events", () => {
  const plan = planInstall(ctx);
  assert.equal(plan.path, join("/proj", ".cursor", "hooks.json"));
  assert.equal(plan.relativePath, ".cursor/hooks.json");
  assert.equal(plan.ide, "cursor");
  assert.ok(plan.events.includes("stop"));
  assert.ok(plan.events.includes("afterFileEdit"));
  assert.ok(plan.events.includes("afterShellExecution"));
  assert.ok(plan.events.includes("beforeMCPExecution"));
  assert.ok(plan.events.includes("afterMCPExecution"));
  assert.ok(plan.events.includes("postToolUseFailure"));
  assert.equal(plan.commandPreview, ctx.cliCommand);
});

test("planInstall user scope targets home cursor hooks", () => {
  const plan = planInstall({ ...ctx, scope: "user", home: "/home/me" });
  const path = join("/home/me", ".cursor", "hooks.json");
  assert.equal(plan.path, path);
  assert.equal(plan.relativePath, path);
});

test("planUninstall user scope reads home cursor hooks", () => {
  const plan = planUninstall("/proj", { scope: "user", home: "/home/me" });
  assert.equal(plan.path, join("/home/me", ".cursor", "hooks.json"));
  assert.equal(plan.removeCount, 0);
});

test("applyInstall preserves foreign hooks and is idempotent", () => {
  const existing = {
    version: 1,
    hooks: {
      sessionStart: [{ command: ".cursor/hooks/log-event.sh sessionStart", timeout: 2 }],
    },
  };
  const once = applyInstall(ctx, existing);
  assert.equal(once.hooks.sessionStart.length, 2);
  assert.ok(once.hooks.sessionStart.some((h) => h.command.includes("log-event")));
  assert.ok(once.hooks.sessionStart.some((h) => h.command.includes("hook cursor")));
  assert.equal(once.hooks.stop[0].timeout, 5);

  const twice = applyInstall(ctx, once);
  assert.equal(twice.hooks.sessionStart.length, 2);
  assert.equal(twice.hooks.stop.length, 1);
});
