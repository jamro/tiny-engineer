import test from "node:test";
import assert from "node:assert/strict";

import { remainingBudgetPercent } from "../src/integrations/claude-code/usage-remaining.js";

test("remaining percent uses the fuller of session and weekly", () => {
  assert.equal(
    remainingBudgetPercent({
      five_hour: { utilization: 20 },
      seven_day: { utilization: 40 },
    }),
    60,
  );
});

test("remaining percent uses a fuller per-model window", () => {
  assert.equal(
    remainingBudgetPercent({
      five_hour: { utilization: 10 },
      seven_day: { utilization: 20 },
      seven_day_sonnet: { utilization: 90 },
    }),
    10,
  );
});

test("remaining percent reads limits percent", () => {
  assert.equal(
    remainingBudgetPercent({
      five_hour: null,
      limits: [{ kind: "session", percent: 10 }, { kind: "weekly_all", percent: 40 }],
    }),
    60,
  );
});

test("remaining percent clamps at zero", () => {
  assert.equal(remainingBudgetPercent({ five_hour: { utilization: 100 } }), 0);
  assert.equal(remainingBudgetPercent({ seven_day: { utilization: 140 } }), 0);
});

test("remaining percent returns null without windows", () => {
  assert.equal(remainingBudgetPercent({}), null);
  assert.equal(remainingBudgetPercent({ five_hour: null, limits: [] }), null);
  assert.equal(remainingBudgetPercent(null), null);
  assert.equal(
    remainingBudgetPercent({ extra_usage: { used_credits: 1977, monthly_limit: 10000 } }),
    null,
  );
});

test("remaining percent uses enterprise extra usage when plan windows are null", () => {
  assert.equal(
    remainingBudgetPercent({
      five_hour: null,
      seven_day: null,
      extra_usage: { utilization: 19.77, used_credits: 1977, monthly_limit: 10000 },
      spend: { percent: 20 },
      limits: [],
    }),
    80.23,
  );
});

test("remaining percent falls back to rounded spend percent", () => {
  assert.equal(remainingBudgetPercent({ five_hour: null, spend: { percent: 20 } }), 80);
});
