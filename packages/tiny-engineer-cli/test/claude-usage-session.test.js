import test from "node:test";
import assert from "node:assert/strict";

import {
  accessTokenFromCredentials,
  resolveClaudeAccessToken,
} from "../src/integrations/claude-code/usage-session.js";

const BLOB = JSON.stringify({
  claudeAiOauth: { accessToken: "access-token", refreshToken: "do-not-log" },
});

test("accessTokenFromCredentials reads oauth json or a raw token", () => {
  assert.equal(accessTokenFromCredentials(BLOB), "access-token");
  assert.equal(accessTokenFromCredentials("  raw-token  "), "raw-token");
  assert.equal(accessTokenFromCredentials("{"), null);
  assert.equal(accessTokenFromCredentials(""), null);
});

test("resolveClaudeAccessToken uses keychain and does not read the file", async () => {
  let reads = 0;
  const token = await resolveClaudeAccessToken({
    sources: [async () => BLOB],
    readFile: () => {
      reads += 1;
      return null;
    },
  });
  assert.equal(token, "access-token");
  assert.equal(reads, 0);
});

test("resolveClaudeAccessToken falls back to the credentials file", async () => {
  const token = await resolveClaudeAccessToken({
    sources: [async () => null],
    readFile: () => BLOB,
  });
  assert.equal(token, "access-token");
});

test("resolveClaudeAccessToken returns null when no login exists", async () => {
  const token = await resolveClaudeAccessToken({
    sources: [async () => null],
    readFile: () => null,
  });
  assert.equal(token, null);
});
