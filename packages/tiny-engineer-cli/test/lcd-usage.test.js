import test from "node:test";
import assert from "node:assert/strict";

import { maybeShowCursorUsageOnLcd } from "../src/integrations/cursor/lcd-usage.js";
import { cookieFromToken, jwtClaims, cookieIdFromClaims } from "../src/integrations/cursor/usage-session.js";

test("maybeShowCursorUsageOnLcd skips when kill switch is 0", async () => {
  let posted = 0;
  await maybeShowCursorUsageOnLcd(
    {},
    {
      env: { TINY_ENGINEER_AI_USAGE_LCD: "0" },
      resolveCookie: async () => "id::jwt",
      fetchLimits: async () => ({ planUsage: { autoPercentUsed: 10 } }),
      postLcd: async () => {
        posted += 1;
      },
    },
  );
  assert.equal(posted, 0);
});

test("maybeShowCursorUsageOnLcd skips LCD when no cookie", async () => {
  let posted = 0;
  await maybeShowCursorUsageOnLcd(
    {},
    {
      env: {},
      resolveCookie: async () => null,
      fetchLimits: async () => ({ planUsage: { autoPercentUsed: 10 } }),
      postLcd: async () => {
        posted += 1;
      },
    },
  );
  assert.equal(posted, 0);
});

test("maybeShowCursorUsageOnLcd skips LCD when remaining is null", async () => {
  let posted = 0;
  await maybeShowCursorUsageOnLcd(
    {},
    {
      env: {},
      resolveCookie: async () => "id::token",
      fetchLimits: async () => ({ planUsage: {} }),
      postLcd: async () => {
        posted += 1;
      },
    },
  );
  assert.equal(posted, 0);
});

test("maybeShowCursorUsageOnLcd posts remaining percent", async () => {
  /** @type {{ pct: number, transport: object } | null} */
  let seen = null;
  await maybeShowCursorUsageOnLcd(
    { url: "http://robot.local" },
    {
      env: {},
      resolveCookie: async () => "id::token",
      fetchLimits: async () => ({
        planUsage: { autoPercentUsed: 40, apiPercentUsed: 25 },
      }),
      postLcd: async (pct, transport) => {
        seen = { pct, transport };
      },
    },
  );
  assert.deepEqual(seen, { pct: 60, transport: { url: "http://robot.local" } });
});

test("maybeShowCursorUsageOnLcd swallows postLcd errors", async () => {
  await maybeShowCursorUsageOnLcd(
    {},
    {
      env: {},
      resolveCookie: async () => "id::token",
      fetchLimits: async () => ({ planUsage: { autoPercentUsed: 5 } }),
      postLcd: async () => {
        throw new Error("lcd down");
      },
      overallTimeoutMs: 500,
    },
  );
});

test("maybeShowCursorUsageOnLcd respects overall timeout", async () => {
  let posted = 0;
  const started = Date.now();
  await maybeShowCursorUsageOnLcd(
    {},
    {
      env: {},
      resolveCookie: () =>
        new Promise((resolve) => {
          // Resolves after the overall cap so cancelled=true skips postLcd.
          setTimeout(() => resolve("id::token"), 80);
        }),
      fetchLimits: async () => ({ planUsage: { autoPercentUsed: 1 } }),
      postLcd: async () => {
        posted += 1;
      },
      overallTimeoutMs: 30,
    },
  );
  assert.ok(Date.now() - started < 200);
  // Let the late cookie resolve; cancel flag must still block LCD.
  await new Promise((r) => setTimeout(r, 100));
  assert.equal(posted, 0);
});

test("cookieFromToken builds id::jwt and skips api_key_token", () => {
  // header.payload.sig — payload {"sub":"github|user_01ABC","type":"session"}
  const sessionPayload = Buffer.from(
    JSON.stringify({ sub: "github|user_01ABC", type: "session" }),
  ).toString("base64url");
  const sessionJwt = `hdr.${sessionPayload}.sig`;
  assert.equal(cookieFromToken(sessionJwt), "user_01ABC::" + sessionJwt);

  const apiPayload = Buffer.from(
    JSON.stringify({ sub: "github|user_01ABC", type: "api_key_token" }),
  ).toString("base64url");
  assert.equal(cookieFromToken(`hdr.${apiPayload}.sig`), null);

  assert.equal(cookieFromToken("already::full"), "already::full");
});

test("jwtClaims and cookieIdFromClaims parse sub", () => {
  const payload = Buffer.from(JSON.stringify({ sub: "auth0|user_xyz" })).toString("base64url");
  const claims = jwtClaims(`x.${payload}.y`);
  assert.equal(cookieIdFromClaims(claims), "user_xyz");
  assert.deepEqual(jwtClaims("not-a-jwt"), {});
});
