import test from "node:test";
import assert from "node:assert/strict";
import { mkdtempSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

import { AI_USAGE_LCD_KEY } from "../src/integrations/cursor/lcd-usage.js";
import { createOutput } from "../src/util/output.js";
import { createStyle } from "../src/util/style.js";
import {
  aiUsageLcdGuide,
  aiUsageLcdPrompt,
  budgetIdeIds,
} from "../src/wizard/ai-usage-lcd.js";
import { readEnvKey } from "../src/wizard/merge.js";
import { runSetup } from "../src/wizard/setup.js";

test("budgetIdeIds drops IDEs without a cost monitor", () => {
  assert.deepEqual(budgetIdeIds(["antigravity"]), []);
  assert.deepEqual(budgetIdeIds(["cursor", "antigravity", "claude-code"]), ["cursor", "claude-code"]);
});

test("aiUsageLcdPrompt names only the selected IDEs", () => {
  assert.match(aiUsageLcdPrompt(["cursor"]), /Cursor/);
  assert.doesNotMatch(aiUsageLcdPrompt(["cursor"]), /Claude/);
  assert.match(aiUsageLcdPrompt(["claude-code"]), /Claude Code/);
  assert.doesNotMatch(aiUsageLcdPrompt(["claude-code"]), /Cursor/);
  assert.match(aiUsageLcdPrompt(["cursor", "claude-code"]), /Cursor and Claude Code/);
});

test("aiUsageLcdGuide is IDE-specific", () => {
  const cursor = aiUsageLcdGuide(["cursor"]).join("\n");
  assert.match(cursor, /cursor-access-token/);
  assert.doesNotMatch(cursor, /\/login/);

  const claude = aiUsageLcdGuide(["claude-code"]).join("\n");
  assert.match(claude, /\/login/);
  assert.match(claude, /API key/);
  assert.doesNotMatch(claude, /cursor-access-token/);
});

test("runSetup antigravity --ai-usage-lcd does not ask or write the flag", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-setup-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
  /** @type {string[]} */
  const infos = [];
  const out = createOutput({
    quiet: false,
    style: createStyle({ color: false }),
    log: () => {},
    error: (m) => infos.push(String(m)),
  });
  try {
    await runSetup(out, ["antigravity", "--yes", "--url", "http://robot.test", "--ai-usage-lcd"], {
      projectRoot: dir,
    });
    assert.equal(process.exitCode, 0);
    assert.equal(readEnvKey(join(cfg, "env"), AI_USAGE_LCD_KEY), null);
    assert.ok(infos.some((line) => line.includes("not available") && line.includes("antigravity")));
    assert.equal(infos.some((line) => line.includes("/login")), false);
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("interactive antigravity setup never asks about AI budget", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-setup-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
  const out = createOutput({
    quiet: true,
    style: createStyle({ color: false }),
    log: () => {},
    error: () => {},
  });
  /** @type {string[]} */
  const questions = [];
  try {
    await runSetup(out, ["antigravity", "--url", "http://robot.test"], {
      projectRoot: dir,
      ask: async (q, def = "") => {
        if (String(q).includes("Access token")) return "";
        if (String(q).includes("Hook scope")) return "project";
        return def || "http";
      },
      confirm: async (q) => {
        questions.push(String(q));
        return true;
      },
    });
    assert.equal(process.exitCode, 0);
    assert.equal(questions.some((q) => q.includes("AI budget")), false);
    assert.equal(readEnvKey(join(cfg, "env"), AI_USAGE_LCD_KEY), null);
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("enabling Cursor budget prints Cursor steps only", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-setup-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
  /** @type {string[]} */
  const infos = [];
  const out = createOutput({
    quiet: false,
    style: createStyle({ color: false }),
    log: () => {},
    error: (m) => infos.push(String(m)),
  });
  try {
    await runSetup(out, ["cursor", "--yes", "--url", "http://robot.test", "--ai-usage-lcd"], {
      projectRoot: dir,
    });
    assert.equal(process.exitCode, 0);
    const text = infos.join("\n");
    assert.match(text, /cursor-access-token/);
    assert.doesNotMatch(text, /\/login/);
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("enabling Claude budget prints /login steps only", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-setup-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
  /** @type {string[]} */
  const infos = [];
  const out = createOutput({
    quiet: false,
    style: createStyle({ color: false }),
    log: () => {},
    error: (m) => infos.push(String(m)),
  });
  try {
    await runSetup(out, ["claude-code", "--yes", "--url", "http://robot.test", "--ai-usage-lcd"], {
      projectRoot: dir,
    });
    assert.equal(process.exitCode, 0);
    const text = infos.join("\n");
    assert.match(text, /\/login/);
    assert.match(text, /API key/);
    assert.doesNotMatch(text, /cursor-access-token/);
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("interactive Cursor prompt does not mention Claude", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-setup-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
  const out = createOutput({
    quiet: true,
    style: createStyle({ color: false }),
    log: () => {},
    error: () => {},
  });
  /** @type {string[]} */
  const questions = [];
  try {
    await runSetup(out, ["cursor", "--url", "http://robot.test"], {
      projectRoot: dir,
      ask: async (q, def = "") => {
        if (String(q).includes("Access token")) return "";
        if (String(q).includes("Hook scope")) return "project";
        return def || "http";
      },
      confirm: async (q) => {
        questions.push(String(q));
        return !String(q).includes("AI budget");
      },
    });
    const budget = questions.find((q) => q.includes("AI budget")) ?? "";
    assert.match(budget, /Cursor/);
    assert.doesNotMatch(budget, /Claude/);
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
    process.exitCode = 0;
  }
});
