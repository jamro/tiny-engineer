import test from "node:test";
import assert from "node:assert/strict";

import { budgetLcdBody, CLAUDE_BUDGET_LABEL, CURSOR_BUDGET_LABEL, OVERALL_TIMEOUT_MS, STEP_TIMEOUT_MS } from "../src/integrations/ai-usage-lcd.js";

test("budgetLcdBody uses the source label", () => {
  assert.deepEqual(budgetLcdBody(60, CURSOR_BUDGET_LABEL), {
    line1: { text: "CURSOR BUDGET" },
    line2: { text: "60.00%", align: "right" },
  });
  assert.deepEqual(budgetLcdBody(80.23, CLAUDE_BUDGET_LABEL), {
    line1: { text: "CLAUDE BUDGET" },
    line2: { text: "80.23%", align: "right" },
  });
  assert.equal(CURSOR_BUDGET_LABEL.length <= 16, true);
  assert.equal(CLAUDE_BUDGET_LABEL.length <= 16, true);
  assert.equal(OVERALL_TIMEOUT_MS, 2000);
  assert.equal(STEP_TIMEOUT_MS, 1500);
});
