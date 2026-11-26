import test from "node:test";
import assert from "node:assert/strict";

import { budgetLcdBody, CLAUDE_BUDGET_LABEL } from "../src/integrations/ai-usage-lcd.js";
import { claudeCode } from "../src/integrations/claude-code/index.js";
import {
  AI_USAGE_LCD_KEY,
  maybeShowClaudeUsageOnLcd,
} from "../src/integrations/claude-code/lcd-usage.js";

const USAGE = {
  five_hour: { utilization: 20 },
  seven_day: { utilization: 40 },
};

test("maybeShowClaudeUsageOnLcd skips when unset", async () => {
  let posted = 0;
  await maybeShowClaudeUsageOnLcd(
    {},
    {
      env: {},
      resolveToken: async () => "token",
      fetchUsage: async () => USAGE,
      postLcd: async () => {
        posted += 1;
      },
    },
  );
  assert.equal(posted, 0);
});

test("maybeShowClaudeUsageOnLcd skips when set to 0", async () => {
  let posted = 0;
  await maybeShowClaudeUsageOnLcd(
    {},
    {
      env: { [AI_USAGE_LCD_KEY]: "0" },
      resolveToken: async () => "token",
      fetchUsage: async () => USAGE,
      postLcd: async () => {
        posted += 1;
      },
    },
  );
  assert.equal(posted, 0);
});

test("maybeShowClaudeUsageOnLcd posts one remaining percent", async () => {
  /** @type {number | null} */
  let pct = null;
  await maybeShowClaudeUsageOnLcd(
    { url: "http://robot.local" },
    {
      env: { [AI_USAGE_LCD_KEY]: "1" },
      resolveToken: async () => "token",
      fetchUsage: async () => USAGE,
      postLcd: async (value) => {
        pct = value;
      },
    },
  );
  assert.equal(pct, 60);
  assert.deepEqual(budgetLcdBody(/** @type {number} */ (pct), CLAUDE_BUDGET_LABEL), {
    line1: { text: "CLAUDE BUDGET" },
    line2: { text: "60.00%", align: "right" },
  });
});

test("maybeShowClaudeUsageOnLcd skips when remaining is null", async () => {
  let posted = 0;
  await maybeShowClaudeUsageOnLcd(
    {},
    {
      env: { [AI_USAGE_LCD_KEY]: "1" },
      resolveToken: async () => "token",
      fetchUsage: async () => ({}),
      postLcd: async () => {
        posted += 1;
      },
    },
  );
  assert.equal(posted, 0);
});

test("maybeShowClaudeUsageOnLcd skips when no login", async () => {
  let posted = 0;
  await maybeShowClaudeUsageOnLcd(
    {},
    {
      env: { [AI_USAGE_LCD_KEY]: "1" },
      resolveToken: async () => null,
      fetchUsage: async () => USAGE,
      postLcd: async () => {
        posted += 1;
      },
    },
  );
  assert.equal(posted, 0);
});

test("maybeShowClaudeUsageOnLcd swallows post errors", async () => {
  await maybeShowClaudeUsageOnLcd(
    {},
    {
      env: { [AI_USAGE_LCD_KEY]: "1" },
      resolveToken: async () => "token",
      fetchUsage: async () => USAGE,
      postLcd: async () => {
        throw new Error("lcd down");
      },
    },
  );
});

test("maybeShowClaudeUsageOnLcd respects overall timeout", async () => {
  let posted = 0;
  const started = Date.now();
  await maybeShowClaudeUsageOnLcd(
    {},
    {
      env: { [AI_USAGE_LCD_KEY]: "1" },
      resolveToken: () => new Promise((resolve) => setTimeout(() => resolve("token"), 80)),
      fetchUsage: async () => USAGE,
      postLcd: async () => {
        posted += 1;
      },
      overallTimeoutMs: 30,
    },
  );
  assert.ok(Date.now() - started < 200);
  await new Promise((r) => setTimeout(r, 100));
  assert.equal(posted, 0);
});

test("afterAnim skips budget when the animation is not ring", async () => {
  const prev = process.env[AI_USAGE_LCD_KEY];
  process.env[AI_USAGE_LCD_KEY] = "1";
  try {
    const started = Date.now();
    await claudeCode.afterAnim({ anim: "error" });
    assert.ok(Date.now() - started < 100);
  } finally {
    if (prev === undefined) delete process.env[AI_USAGE_LCD_KEY];
    else process.env[AI_USAGE_LCD_KEY] = prev;
  }
});
