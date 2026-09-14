import test from "node:test";
import assert from "node:assert/strict";
import { mkdtempSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { DEFAULT_URL, getBaseUrl, getToken, loadDotEnv } from "../src/env.js";

const KEYS = ["TINY_ENGINEER_TOKEN", "TINY_ENGINEER_URL"];

function withEnv(values, fn) {
  const saved = Object.fromEntries(KEYS.map((key) => [key, process.env[key]]));
  for (const key of KEYS) delete process.env[key];
  Object.assign(process.env, values);
  try {
    fn();
  } finally {
    for (const key of KEYS) {
      if (saved[key] === undefined) delete process.env[key];
      else process.env[key] = saved[key];
    }
  }
}

function projectWithDotEnv(contents) {
  const dir = mkdtempSync(join(tmpdir(), "te-claude-code-env-"));
  writeFileSync(join(dir, ".env"), contents);
  return dir;
}

test("a project .env can set both the token and the URL", () => {
  const dir = projectWithDotEnv(
    "TINY_ENGINEER_TOKEN=projtok\nTINY_ENGINEER_URL=http://10.0.0.5/\n"
  );
  withEnv({}, () => {
    loadDotEnv(dir);
    assert.equal(getToken(), "projtok");
    assert.equal(getBaseUrl(), "http://10.0.0.5");
  });
});

test("a project .env cannot redirect a token inherited from the environment", () => {
  const dir = projectWithDotEnv("TINY_ENGINEER_URL=http://10.0.0.5\n");
  withEnv({ TINY_ENGINEER_TOKEN: "envtok" }, () => {
    loadDotEnv(dir);
    assert.equal(getToken(), "envtok");
    assert.equal(getBaseUrl(), DEFAULT_URL);
  });
});

test("an inherited URL still wins over the project .env", () => {
  const dir = projectWithDotEnv("TINY_ENGINEER_URL=http://10.0.0.5\n");
  withEnv({ TINY_ENGINEER_TOKEN: "envtok", TINY_ENGINEER_URL: "http://10.0.0.9" }, () => {
    loadDotEnv(dir);
    assert.equal(getBaseUrl(), "http://10.0.0.9");
  });
});
