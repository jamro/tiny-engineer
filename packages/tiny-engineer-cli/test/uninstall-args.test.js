import test from "node:test";
import assert from "node:assert/strict";
import { mkdtempSync, mkdirSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { createOutput } from "../src/util/output.js";
import { createStyle } from "../src/util/style.js";
import { parseUninstallArgs, runUninstall } from "../src/wizard/uninstall.js";

test("parseUninstallArgs reads flags", () => {
  const args = parseUninstallArgs(["cursor", "--yes", "--dry-run", "--global"]);
  assert.deepEqual(args.ides, ["cursor"]);
  assert.equal(args.yes, true);
  assert.equal(args.dryRun, true);
  assert.equal(args.global, true);
});

test("parseUninstallArgs rejects --purge-env", () => {
  const args = parseUninstallArgs(["--purge-env"]);
  assert.ok(args.error?.includes("--purge-env"));
});

test("runUninstall --yes without ide exits 1", async () => {
  /** @type {string[]} */
  const errs = [];
  const out = createOutput({
    quiet: false,
    style: createStyle({ color: false }),
    log: () => {},
    error: (m) => errs.push(String(m)),
  });
  await runUninstall(out, ["--yes"]);
  assert.equal(process.exitCode, 1);
  assert.ok(errs.some((e) => e.includes("--yes requires")));
  process.exitCode = 0;
});

test("runUninstall --dry-run does not write", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-un-"));
  mkdirSync(join(dir, ".cursor"), { recursive: true });
  const hooksPath = join(dir, ".cursor", "hooks.json");
  writeFileSync(
    hooksPath,
    JSON.stringify({
      version: 1,
      hooks: { stop: [{ command: 'node "/x" hook cursor', timeout: 5 }] },
    }),
    "utf8",
  );
  const before = readFileSync(hooksPath, "utf8");
  const out = createOutput({
    quiet: true,
    style: createStyle({ color: false }),
    log: () => {},
    error: () => {},
  });
  try {
    await runUninstall(out, ["cursor", "--yes", "--dry-run"], { projectRoot: dir });
    assert.equal(process.exitCode, 0);
    assert.equal(readFileSync(hooksPath, "utf8"), before);
  } finally {
    rmSync(dir, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("runUninstall --yes removes cursor hooks", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-un-"));
  mkdirSync(join(dir, ".cursor"), { recursive: true });
  const hooksPath = join(dir, ".cursor", "hooks.json");
  writeFileSync(
    hooksPath,
    JSON.stringify({
      version: 1,
      hooks: {
        sessionStart: [
          { command: "log.sh", timeout: 2 },
          { command: 'node "/x" hook cursor', timeout: 5 },
        ],
      },
    }),
    "utf8",
  );
  const out = createOutput({
    quiet: true,
    style: createStyle({ color: false }),
    log: () => {},
    error: () => {},
  });
  try {
    await runUninstall(out, ["cursor", "--yes"], { projectRoot: dir });
    const hooks = JSON.parse(readFileSync(hooksPath, "utf8"));
    assert.equal(hooks.hooks.sessionStart.length, 1);
    assert.equal(hooks.hooks.sessionStart[0].command, "log.sh");
  } finally {
    rmSync(dir, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("runUninstall --global removes cursor user hooks only", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-un-"));
  const home = mkdtempSync(join(tmpdir(), "te-home-"));
  mkdirSync(join(home, ".cursor"), { recursive: true });
  mkdirSync(join(dir, ".cursor"), { recursive: true });
  const userHooks = join(home, ".cursor", "hooks.json");
  const projectHooks = join(dir, ".cursor", "hooks.json");
  const body = JSON.stringify({
    version: 1,
    hooks: {
      stop: [
        { command: "log.sh", timeout: 2 },
        { command: 'node "/x" hook cursor', timeout: 5 },
      ],
    },
  });
  writeFileSync(userHooks, body, "utf8");
  writeFileSync(projectHooks, body, "utf8");
  const out = createOutput({
    quiet: true,
    style: createStyle({ color: false }),
    log: () => {},
    error: () => {},
  });
  try {
    await runUninstall(out, ["cursor", "--yes", "--global"], { projectRoot: dir, home });
    const user = JSON.parse(readFileSync(userHooks, "utf8"));
    const project = JSON.parse(readFileSync(projectHooks, "utf8"));
    assert.equal(user.hooks.stop.length, 1);
    assert.equal(user.hooks.stop[0].command, "log.sh");
    assert.equal(project.hooks.stop.length, 2);
  } finally {
    rmSync(dir, { recursive: true, force: true });
    rmSync(home, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("runUninstall --global removes claude user hooks only", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-un-"));
  const home = mkdtempSync(join(tmpdir(), "te-home-"));
  mkdirSync(join(home, ".claude"), { recursive: true });
  mkdirSync(join(dir, ".claude"), { recursive: true });
  const userSettings = join(home, ".claude", "settings.json");
  const projectSettings = join(dir, ".claude", "settings.json");
  const body = JSON.stringify({
    hooks: {
      Stop: [
        { hooks: [{ type: "command", command: "log.sh" }] },
        { hooks: [{ type: "command", command: 'node "/x" hook claude-code', async: true }] },
      ],
    },
  });
  writeFileSync(userSettings, body, "utf8");
  writeFileSync(projectSettings, body, "utf8");
  const out = createOutput({
    quiet: true,
    style: createStyle({ color: false }),
    log: () => {},
    error: () => {},
  });
  try {
    await runUninstall(out, ["claude-code", "--yes", "--global"], { projectRoot: dir, home });
    const user = JSON.parse(readFileSync(userSettings, "utf8"));
    const project = JSON.parse(readFileSync(projectSettings, "utf8"));
    assert.equal(user.hooks.Stop.length, 1);
    assert.equal(user.hooks.Stop[0].hooks[0].command, "log.sh");
    assert.equal(project.hooks.Stop.length, 2);
  } finally {
    rmSync(dir, { recursive: true, force: true });
    rmSync(home, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("runUninstall --global rejects antigravity and --all", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-un-"));
  const home = mkdtempSync(join(tmpdir(), "te-home-"));
  /** @type {string[]} */
  const errs = [];
  const out = createOutput({
    quiet: false,
    style: createStyle({ color: false }),
    log: () => {},
    error: (m) => errs.push(String(m)),
  });
  try {
    await runUninstall(out, ["antigravity", "--global"], { projectRoot: dir, home });
    assert.equal(process.exitCode, 1);
    assert.ok(errs.some((e) => e.includes("not supported") && e.includes("antigravity")));
    errs.length = 0;
    process.exitCode = 0;
    await runUninstall(out, ["--all", "--global"], { projectRoot: dir, home });
    assert.equal(process.exitCode, 1);
    assert.ok(
      errs.some(
        (e) => e.includes("not supported") && e.includes("antigravity") && !e.includes("claude-code"),
      ),
    );
  } finally {
    rmSync(dir, { recursive: true, force: true });
    rmSync(home, { recursive: true, force: true });
    process.exitCode = 0;
  }
});
