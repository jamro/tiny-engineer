import test from "node:test";
import assert from "node:assert/strict";
import { applyInstall, planInstall } from "../src/integrations/cursor/install.js";

const ctx = {
  projectRoot: "/proj",
  cliCommand: 'node "/proj/bin/tiny-engineer.js" hook cursor',
  timeout: 5,
};

test("planInstall lists cursor events", () => {
  const plan = planInstall(ctx);
  assert.equal(plan.ide, "cursor");
  assert.ok(plan.events.includes("stop"));
  assert.ok(plan.events.includes("afterFileEdit"));
  assert.equal(plan.commandPreview, ctx.cliCommand);
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
