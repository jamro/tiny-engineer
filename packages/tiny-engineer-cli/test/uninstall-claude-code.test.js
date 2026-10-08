import test from "node:test";
import assert from "node:assert/strict";
import { applyUninstall } from "../src/integrations/claude-code/install.js";

test("applyUninstall keeps permissions and strips our Stop hook", () => {
  const existing = {
    permissions: { allow: ["Bash"] },
    hooks: {
      Stop: [
        {
          hooks: [
            {
              type: "command",
              command: 'node "/x" hook claude-code',
              async: true,
            },
            {
              type: "command",
              command: "echo other",
              async: true,
            },
          ],
        },
      ],
    },
  };
  const once = applyUninstall(existing);
  assert.deepEqual(once.permissions, { allow: ["Bash"] });
  assert.equal(once.hooks.Stop.length, 1);
  assert.equal(once.hooks.Stop[0].hooks.length, 1);
  assert.equal(once.hooks.Stop[0].hooks[0].command, "echo other");

  const twice = applyUninstall(once);
  assert.equal(twice.hooks.Stop[0].hooks.length, 1);
});
