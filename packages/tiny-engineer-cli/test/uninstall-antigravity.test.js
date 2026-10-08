import test from "node:test";
import assert from "node:assert/strict";
import { applyUninstall } from "../src/integrations/antigravity/install.js";

test("applyUninstall removes tiny-engineer key only", () => {
  const existing = {
    other: { keep: true },
    "tiny-engineer": {
      Stop: [{ command: "node x hook antigravity Stop", timeout: 5 }],
    },
  };
  const once = applyUninstall(existing);
  assert.deepEqual(once.other, { keep: true });
  assert.equal(once["tiny-engineer"], undefined);

  const twice = applyUninstall(once);
  assert.deepEqual(twice.other, { keep: true });
});
