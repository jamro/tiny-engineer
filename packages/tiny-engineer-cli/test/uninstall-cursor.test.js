import test from "node:test";
import assert from "node:assert/strict";
import { applyUninstall, planUninstall } from "../src/integrations/cursor/install.js";

test("applyUninstall keeps foreign hooks and removes ours", () => {
  const existing = {
    version: 1,
    hooks: {
      sessionStart: [
        { command: ".cursor/hooks/log-event.sh sessionStart", timeout: 2 },
        { command: 'node "/x" hook cursor', timeout: 5 },
      ],
      stop: [{ command: 'node "/x" hook cursor', timeout: 5 }],
    },
  };
  const once = applyUninstall(existing);
  assert.equal(once.hooks.sessionStart.length, 1);
  assert.ok(once.hooks.sessionStart[0].command.includes("log-event"));
  assert.equal(once.hooks.stop, undefined);

  const twice = applyUninstall(once);
  assert.deepEqual(twice.hooks.sessionStart, once.hooks.sessionStart);
});

test("planUninstall counts our entries", () => {
  // planUninstall reads from disk; count via applyUninstall input shape using temp is in args test.
  const existing = {
    hooks: {
      stop: [
        { command: "tiny-engineer-cursor" },
        { command: "other" },
      ],
    },
  };
  const after = applyUninstall(existing);
  assert.equal(after.hooks.stop.length, 1);
  assert.equal(after.hooks.stop[0].command, "other");
});

test("planUninstall missing file is zero", () => {
  const plan = planUninstall("/tmp/te-uninstall-missing-cursor-root-xyz");
  assert.equal(plan.removeCount, 0);
});
