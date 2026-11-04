import test from "node:test";
import assert from "node:assert/strict";
import { mkdtempSync, rmSync } from "node:fs";
import { homedir, tmpdir } from "node:os";
import { join } from "node:path";

import { defaultCachePath } from "../src/robot/serial/port-cache.js";
import { userConfigDir, userEnvPath } from "../src/config/paths.js";
import { daemonPaths } from "../src/daemon/paths.js";

test("TINY_ENGINEER_CONFIG_DIR overrides userConfigDir and cache path", () => {
  const dir = mkdtempSync(join(tmpdir(), "te-cfg-"));
  const prev = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_CONFIG_DIR = dir;
  try {
    assert.equal(userConfigDir(), dir);
    assert.equal(userEnvPath(), join(dir, "env"));
    assert.equal(defaultCachePath(), join(dir, "serial-port.json"));
    assert.equal(daemonPaths(dir).rules, join(dir, "rules.json"));
    assert.notEqual(daemonPaths(dir).rules, daemonPaths(join(homedir(), ".config", "tiny-engineer")).rules);
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prev;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("default Unix path uses ~/.config/tiny-engineer", () => {
  if (process.platform === "win32") return;
  const prevC = process.env.TINY_ENGINEER_CONFIG_DIR;
  const prevX = process.env.XDG_CONFIG_HOME;
  delete process.env.TINY_ENGINEER_CONFIG_DIR;
  delete process.env.XDG_CONFIG_HOME;
  try {
    const expected = join(homedir(), ".config", "tiny-engineer");
    assert.equal(userConfigDir(), expected);
    assert.equal(userEnvPath(), join(expected, "env"));
  } finally {
    if (prevC === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prevC;
    if (prevX === undefined) delete process.env.XDG_CONFIG_HOME;
    else process.env.XDG_CONFIG_HOME = prevX;
  }
});
