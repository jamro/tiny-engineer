import test from "node:test";
import assert from "node:assert/strict";
import { mkdtempSync, readFileSync, rmSync, writeFileSync, mkdirSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { createOutput } from "../src/util/output.js";
import { createStyle } from "../src/util/style.js";
import { mergeEnvUrl } from "../src/wizard/merge.js";
import { parseSetupArgs, runSetup } from "../src/wizard/setup.js";

test("parseSetupArgs --yes without ide fails at run time; parser ok", () => {
  const args = parseSetupArgs(["--yes"]);
  assert.equal(args.yes, true);
  assert.equal(args.ides.length, 0);
});

test("parseSetupArgs reads flags", () => {
  const args = parseSetupArgs([
    "cursor",
    "--yes",
    "--dry-run",
    "--url",
    "http://x",
    "--force-url",
  ]);
  assert.deepEqual(args.ides, ["cursor"]);
  assert.equal(args.yes, true);
  assert.equal(args.dryRun, true);
  assert.equal(args.url, "http://x");
  assert.equal(args.forceUrl, true);
});

test("runSetup --yes without ide exits 1", async () => {
  /** @type {string[]} */
  const errs = [];
  const out = createOutput({
    quiet: false,
    style: createStyle({ color: false }),
    log: () => {},
    error: (m) => errs.push(String(m)),
  });
  await runSetup(out, ["--yes"]);
  assert.equal(process.exitCode, 1);
  assert.ok(errs.some((e) => e.includes("--yes requires")));
  process.exitCode = 0;
});

test("runSetup --dry-run does not write files", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-setup-"));
  /** @type {string[]} */
  const infos = [];
  const out = createOutput({
    quiet: false,
    style: createStyle({ color: false }),
    log: () => {},
    error: (m) => infos.push(String(m)),
  });
  try {
    await runSetup(out, ["cursor", "--yes", "--dry-run"], { projectRoot: dir });
    assert.equal(process.exitCode, 0);
    assert.throws(() => readFileSync(join(dir, ".cursor", "hooks.json")));
    assert.ok(infos.some((l) => l.includes(".cursor/hooks.json")));
  } finally {
    rmSync(dir, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("runSetup --yes writes cursor hooks", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-setup-"));
  mkdirSync(join(dir, ".cursor"));
  const out = createOutput({
    quiet: true,
    style: createStyle({ color: false }),
    log: () => {},
    error: () => {},
  });
  try {
    await runSetup(out, ["cursor", "--yes", "--url", "http://robot.test"], {
      projectRoot: dir,
    });
    const hooks = JSON.parse(readFileSync(join(dir, ".cursor", "hooks.json"), "utf8"));
    assert.equal(hooks.hooks.stop[0].command, "npx -y tiny-engineer hook cursor");
    assert.equal(hooks.hooks.stop[0].timeout, 30);
    const env = readFileSync(join(dir, ".env"), "utf8");
    assert.ok(env.includes("TINY_ENGINEER_URL=http://robot.test"));
  } finally {
    rmSync(dir, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("runSetup aborts when hooks.json is invalid JSON", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-setup-"));
  mkdirSync(join(dir, ".cursor"), { recursive: true });
  const hooksPath = join(dir, ".cursor", "hooks.json");
  writeFileSync(hooksPath, "{", "utf8");
  /** @type {string[]} */
  const errs = [];
  const out = createOutput({
    quiet: false,
    style: createStyle({ color: false }),
    log: () => {},
    error: (m) => errs.push(String(m)),
  });
  try {
    await runSetup(out, ["cursor", "--yes"], { projectRoot: dir });
    assert.equal(process.exitCode, 1);
    assert.equal(readFileSync(hooksPath, "utf8"), "{");
    assert.ok(errs.some((e) => e.includes("not valid JSON")));
  } finally {
    rmSync(dir, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("mergeEnvUrl respects force flag", () => {
  const dir = mkdtempSync(join(tmpdir(), "te-env-"));
  const envPath = join(dir, ".env");
  try {
    writeFileSync(envPath, "TINY_ENGINEER_URL=http://old\n", "utf8");
    assert.equal(mergeEnvUrl(envPath, "http://new"), "skipped");
    assert.ok(readFileSync(envPath, "utf8").includes("http://old"));
    assert.equal(mergeEnvUrl(envPath, "http://new", { force: true }), "updated");
    assert.ok(readFileSync(envPath, "utf8").includes("http://new"));
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});
