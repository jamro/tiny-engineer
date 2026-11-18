import test from "node:test";
import assert from "node:assert/strict";

import { remainingBudgetPercent } from "../src/integrations/cursor/usage-remaining.js";

test("remainingBudgetPercent from planUsage max of auto/api", () => {
  assert.equal(
    remainingBudgetPercent({
      planUsage: { autoPercentUsed: 40, apiPercentUsed: 25 },
    }),
    60,
  );
});

test("remainingBudgetPercent uses individualUsage.plan fallback", () => {
  assert.equal(
    remainingBudgetPercent({
      individualUsage: { plan: { autoPercentUsed: 12.34, apiPercentUsed: 10 } },
    }),
    87.66,
  );
});

test("remainingBudgetPercent prefers planUsage over individualUsage", () => {
  assert.equal(
    remainingBudgetPercent({
      planUsage: { autoPercentUsed: 50 },
      individualUsage: { plan: { autoPercentUsed: 90 } },
    }),
    50,
  );
});

test("remainingBudgetPercent clamps at 0 when over 100", () => {
  assert.equal(
    remainingBudgetPercent({
      planUsage: { autoPercentUsed: 110 },
    }),
    0,
  );
});

test("remainingBudgetPercent rounds to two decimals", () => {
  assert.equal(
    remainingBudgetPercent({
      planUsage: { apiPercentUsed: 33.333 },
    }),
    66.67,
  );
});

test("remainingBudgetPercent returns null when no usable fields", () => {
  assert.equal(remainingBudgetPercent({}), null);
  assert.equal(remainingBudgetPercent({ planUsage: {} }), null);
  assert.equal(remainingBudgetPercent(null), null);
  assert.equal(remainingBudgetPercent(undefined), null);
});

test("remainingBudgetPercent ignores non-finite values", () => {
  assert.equal(
    remainingBudgetPercent({
      planUsage: { autoPercentUsed: "nope", apiPercentUsed: 20 },
    }),
    80,
  );
});
