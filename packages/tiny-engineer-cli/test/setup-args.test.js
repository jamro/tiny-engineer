import test from "node:test";
import assert from "node:assert/strict";
import {
  chmodSync,
  existsSync,
  mkdtempSync,
  mkdirSync,
  readFileSync,
  rmSync,
  statSync,
  writeFileSync,
} from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { createOutput } from "../src/util/output.js";
import { createStyle } from "../src/util/style.js";
import { mergeEnvKey, mergeEnvUrl, readEnvKey } from "../src/wizard/merge.js";
import { parseSetupArgs, runSetup, warnProjectOverrides } from "../src/wizard/setup.js";

test("parseSetupArgs --yes without ide fails at run time; parser ok", () => {
  const args = parseSetupArgs(["--yes"]);
  assert.equal(args.yes, true);
  assert.equal(args.ides.length, 0);
});

test("parseSetupArgs reads flags without force", () => {
  const args = parseSetupArgs([
    "cursor",
    "--yes",
    "--dry-run",
    "--url",
    "http://x",
    "--transport",
    "serial",
    "--port",
    "/dev/ttyUSB0",
  ]);
  assert.deepEqual(args.ides, ["cursor"]);
  assert.equal(args.yes, true);
  assert.equal(args.dryRun, true);
  assert.equal(args.url, "http://x");
  assert.equal(args.transport, "serial");
  assert.equal(args.port, "/dev/ttyUSB0");
});

test("parseSetupArgs rejects removed force flags", () => {
  const a = parseSetupArgs(["--force-url"]);
  assert.ok(a.error?.includes("--force-url"));
  const b = parseSetupArgs(["--force-transport"]);
  assert.ok(b.error?.includes("--force-transport"));
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
    await runSetup(out, ["cursor", "--yes", "--dry-run"], { projectRoot: dir });
    assert.equal(process.exitCode, 0);
    assert.throws(() => readFileSync(join(dir, ".cursor", "hooks.json")));
    assert.ok(infos.some((l) => l.includes(".cursor/hooks.json")));
    assert.ok(infos.some((l) => l.includes("User config") || l.includes("user config")));
    assert.equal(existsSync(join(dir, ".env")), false);
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("runSetup --yes writes cursor hooks and user env not project .env", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-setup-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
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
    assert.equal(
      hooks.hooks.afterShellExecution[0].command,
      "npx -y tiny-engineer hook cursor",
    );
    assert.equal(
      hooks.hooks.beforeMCPExecution[0].command,
      "npx -y tiny-engineer hook cursor",
    );
    assert.equal(
      hooks.hooks.afterMCPExecution[0].command,
      "npx -y tiny-engineer hook cursor",
    );
    assert.equal(
      hooks.hooks.postToolUseFailure[0].command,
      "npx -y tiny-engineer hook cursor",
    );
    assert.equal(existsSync(join(dir, ".env")), false);
    const env = readFileSync(join(cfg, "env"), "utf8");
    assert.ok(env.includes("TINY_ENGINEER_URL=http://robot.test"));
    assert.ok(env.includes("TINY_ENGINEER_TRANSPORT=http"));
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("runSetup --yes --transport serial writes transport without URL", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-setup-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
  mkdirSync(join(dir, ".cursor"));
  const out = createOutput({
    quiet: true,
    style: createStyle({ color: false }),
    log: () => {},
    error: () => {},
  });
  try {
    await runSetup(out, ["cursor", "--yes", "--transport", "serial", "--port", "/dev/cu.usb"], {
      projectRoot: dir,
    });
    assert.equal(process.exitCode, 0);
    const env = readFileSync(join(cfg, "env"), "utf8");
    assert.ok(env.includes("TINY_ENGINEER_TRANSPORT=serial"));
    assert.ok(env.includes("TINY_ENGINEER_PORT=/dev/cu.usb"));
    assert.ok(!env.includes("TINY_ENGINEER_URL="));
    assert.equal(existsSync(join(dir, ".env")), false);
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("runSetup --yes keeps conflicting user env values", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-setup-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
  mkdirSync(join(dir, ".cursor"));
  writeFileSync(join(cfg, "env"), "TINY_ENGINEER_TRANSPORT=serial\n", "utf8");
  /** @type {string[]} */
  const infos = [];
  const out = createOutput({
    quiet: false,
    style: createStyle({ color: false }),
    log: () => {},
    error: (m) => infos.push(String(m)),
  });
  try {
    await runSetup(out, ["cursor", "--yes", "--transport", "http", "--url", "http://n"], {
      projectRoot: dir,
    });
    assert.equal(readEnvKey(join(cfg, "env"), "TINY_ENGINEER_TRANSPORT"), "serial");
    assert.ok(infos.some((l) => l.includes("kept existing")));
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("warnProjectOverrides names keys without token value", () => {
  const dir = mkdtempSync(join(tmpdir(), "te-setup-"));
  writeFileSync(
    join(dir, ".env"),
    "TINY_ENGINEER_URL=http://x\nTINY_ENGINEER_TOKEN=supersecret\n",
    "utf8",
  );
  /** @type {string[]} */
  const infos = [];
  const out = createOutput({
    quiet: false,
    style: createStyle({ color: false }),
    log: () => {},
    error: (m) => infos.push(String(m)),
  });
  try {
    warnProjectOverrides(out, dir);
    const line = infos.find((l) => l.includes("overrides"));
    assert.ok(line);
    assert.ok(line.includes("TINY_ENGINEER_URL"));
    assert.ok(line.includes("TINY_ENGINEER_TOKEN (set)"));
    assert.ok(!line.includes("supersecret"));
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("runSetup aborts when hooks.json is invalid JSON", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-setup-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
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
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
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

test("interactive setup preserves --token when Enter would clear prompt", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-setup-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
  mkdirSync(join(dir, ".cursor"));
  /** @type {string[]} */
  const tokenPrompts = [];
  const out = createOutput({
    quiet: true,
    style: createStyle({ color: false }),
    log: () => {},
    error: () => {},
  });
  try {
    await runSetup(out, ["cursor", "--token", "from-flag"], {
      projectRoot: dir,
      ask: async (q, def = "") => {
        if (String(q).includes("Access token")) {
          tokenPrompts.push(String(q));
          return "";
        }
        return def || "http";
      },
      confirm: async () => true,
    });
    assert.equal(process.exitCode, 0);
    assert.equal(tokenPrompts.length, 0);
    assert.equal(readEnvKey(join(cfg, "env"), "TINY_ENGINEER_TOKEN"), "from-flag");
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("readEnvKey first assignment wins on duplicates", () => {
  const dir = mkdtempSync(join(tmpdir(), "te-env-"));
  const envPath = join(dir, "env");
  try {
    writeFileSync(
      envPath,
      "TINY_ENGINEER_TRANSPORT=serial\nTINY_ENGINEER_TRANSPORT=http\n",
      "utf8",
    );
    assert.equal(readEnvKey(envPath, "TINY_ENGINEER_TRANSPORT"), "serial");
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("mergeEnvKey private mode sets owner-only perms on POSIX", () => {
  if (process.platform === "win32") return;
  const dir = mkdtempSync(join(tmpdir(), "te-env-"));
  const envPath = join(dir, "env");
  try {
    writeFileSync(envPath, "FOO=1\n", { mode: 0o644 });
    chmodSync(envPath, 0o644);
    assert.equal(
      mergeEnvKey(envPath, "TINY_ENGINEER_URL", "http://x", { force: true, private: true }),
      "updated",
    );
    const mode = statSync(envPath).mode & 0o777;
    assert.equal(mode, 0o600);
    const dirMode = statSync(dir).mode & 0o777;
    assert.equal(dirMode, 0o700);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("interactive setup confirms replace of differing user env URL", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-setup-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
  mkdirSync(join(dir, ".cursor"));
  writeFileSync(join(cfg, "env"), "TINY_ENGINEER_URL=http://old\nTINY_ENGINEER_TRANSPORT=http\n", "utf8");
  /** @type {string[]} */
  const confirms = [];
  const out = createOutput({
    quiet: true,
    style: createStyle({ color: false }),
    log: () => {},
    error: () => {},
  });
  try {
    await runSetup(out, ["cursor", "--url", "http://new"], {
      projectRoot: dir,
      ask: async (q, def = "") => {
        if (String(q).includes("Access token")) return "";
        return def || "http";
      },
      confirm: async (q) => {
        confirms.push(String(q));
        return true;
      },
    });
    assert.equal(process.exitCode, 0);
    assert.ok(confirms.some((c) => c.includes("http://old") && c.includes("http://new")));
    assert.equal(readEnvKey(join(cfg, "env"), "TINY_ENGINEER_URL"), "http://new");
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("interactive setup declines replace keeps stored URL", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-setup-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
  mkdirSync(join(dir, ".cursor"));
  writeFileSync(join(cfg, "env"), "TINY_ENGINEER_URL=http://old\nTINY_ENGINEER_TRANSPORT=http\n", "utf8");
  const out = createOutput({
    quiet: true,
    style: createStyle({ color: false }),
    log: () => {},
    error: () => {},
  });
  try {
    await runSetup(out, ["cursor", "--url", "http://new"], {
      projectRoot: dir,
      ask: async (q, def = "") => {
        if (String(q).includes("Access token")) return "";
        return def || "http";
      },
      confirm: async (q) => {
        // Approve hook write; decline URL replace
        if (String(q).includes("Replace user config")) return false;
        return true;
      },
    });
    assert.equal(process.exitCode, 0);
    assert.equal(readEnvKey(join(cfg, "env"), "TINY_ENGINEER_URL"), "http://old");
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("interactive HTTP blank token does not write TINY_ENGINEER_TOKEN", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-setup-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
  mkdirSync(join(dir, ".cursor"));
  const out = createOutput({
    quiet: true,
    style: createStyle({ color: false }),
    log: () => {},
    error: () => {},
  });
  try {
    await runSetup(out, ["cursor", "--url", "http://robot.test"], {
      projectRoot: dir,
      ask: async (q, def = "") => {
        if (String(q).includes("Access token")) return "";
        return def || "http";
      },
      confirm: async () => true,
    });
    assert.equal(process.exitCode, 0);
    assert.equal(existsSync(join(cfg, "env")), true);
    assert.equal(readEnvKey(join(cfg, "env"), "TINY_ENGINEER_TOKEN"), null);
    assert.ok(!readFileSync(join(cfg, "env"), "utf8").includes("TINY_ENGINEER_TOKEN="));
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
    process.exitCode = 0;
  }
});

test("setup leaves existing project .env byte-identical", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-setup-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
  mkdirSync(join(dir, ".cursor"));
  const projectEnv = join(dir, ".env");
  const before = "# keep me\nFOO=bar\nTINY_ENGINEER_URL=http://project\n";
  writeFileSync(projectEnv, before, "utf8");
  const out = createOutput({
    quiet: true,
    style: createStyle({ color: false }),
    log: () => {},
    error: () => {},
  });
  try {
    await runSetup(out, ["cursor", "--yes", "--url", "http://user.test"], {
      projectRoot: dir,
    });
    assert.equal(process.exitCode, 0);
    assert.equal(readFileSync(projectEnv, "utf8"), before);
    assert.equal(readEnvKey(join(cfg, "env"), "TINY_ENGINEER_URL"), "http://user.test");
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
    process.exitCode = 0;
  }
});
