import test from "node:test";
import assert from "node:assert/strict";
import { mkdtempSync, mkdirSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { createOutput } from "../src/util/output.js";
import { createStyle } from "../src/util/style.js";
import { stripEnvTinyEngineer } from "../src/wizard/merge.js";
import { parseUninstallArgs, runUninstall } from "../src/wizard/uninstall.js";

test("parseUninstallArgs reads flags", () => {
  const args = parseUninstallArgs(["cursor", "--yes", "--dry-run", "--purge-env"]);
  assert.deepEqual(args.ides, ["cursor"]);
  assert.equal(args.yes, true);
  assert.equal(args.dryRun, true);
  assert.equal(args.purgeEnv, true);
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

test("stripEnvTinyEngineer removes only TE keys", () => {
  const dir = mkdtempSync(join(tmpdir(), "te-env-"));
  const envPath = join(dir, ".env");
  try {
    writeFileSync(
      envPath,
      "FOO=1\nTINY_ENGINEER_URL=http://x\nTINY_ENGINEER_TOKEN=secret\nBAR=2\n",
      "utf8",
    );
    assert.equal(stripEnvTinyEngineer(envPath), "removed");
    const next = readFileSync(envPath, "utf8");
    assert.ok(next.includes("FOO=1"));
    assert.ok(next.includes("BAR=2"));
    assert.ok(!next.includes("TINY_ENGINEER"));
    assert.equal(stripEnvTinyEngineer(envPath), "unchanged");
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});
