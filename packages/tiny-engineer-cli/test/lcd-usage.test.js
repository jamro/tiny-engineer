import test from "node:test";
import assert from "node:assert/strict";

import {
  AI_USAGE_LCD_KEY,
  maybeShowCursorUsageOnLcd,
} from "../src/integrations/cursor/lcd-usage.js";
import {
  cookieFromToken,
  jwtClaims,
  cookieIdFromClaims,
  resolveCursorSessionCookie,
  selectUsableStateDbToken,
} from "../src/integrations/cursor/usage-session.js";

test("maybeShowCursorUsageOnLcd skips when unset (opt-in default off)", async () => {
  let posted = 0;
  await maybeShowCursorUsageOnLcd(
    {},
    {
      env: {},
      resolveCookie: async () => "id::jwt",
      fetchLimits: async () => ({ planUsage: { autoPercentUsed: 10 } }),
      postLcd: async () => {
        posted += 1;
      },
    },
  );
  assert.equal(posted, 0);
});

test("maybeShowCursorUsageOnLcd skips when set to 0", async () => {
  let posted = 0;
  await maybeShowCursorUsageOnLcd(
    {},
    {
      env: { [AI_USAGE_LCD_KEY]: "0" },
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
      env: { [AI_USAGE_LCD_KEY]: "1" },
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
      env: { [AI_USAGE_LCD_KEY]: "1" },
      resolveCookie: async () => "id::token",
      fetchLimits: async () => ({ planUsage: {} }),
      postLcd: async () => {
        posted += 1;
      },
    },
  );
  assert.equal(posted, 0);
});

test("maybeShowCursorUsageOnLcd posts remaining percent when enabled", async () => {
  /** @type {{ pct: number, transport: object } | null} */
  let seen = null;
  await maybeShowCursorUsageOnLcd(
    { url: "http://robot.local" },
    {
      env: { [AI_USAGE_LCD_KEY]: "1" },
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
      env: { [AI_USAGE_LCD_KEY]: "1" },
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
  /** @type {AbortSignal | undefined} */
  let seenSignal;
  const started = Date.now();
  await maybeShowCursorUsageOnLcd(
    {},
    {
      env: { [AI_USAGE_LCD_KEY]: "1" },
      resolveCookie: (opts) =>
        new Promise((resolve) => {
          seenSignal = opts?.signal;
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
  assert.equal(seenSignal?.aborted, true);
  // Let the late cookie resolve; cancel flag must still block LCD.
  await new Promise((r) => setTimeout(r, 100));
  assert.equal(posted, 0);
});

test("resolveCursorSessionCookie returns immediately when already aborted", async () => {
  const prev = process.env.CURSOR_SESSION_TOKEN;
  delete process.env.CURSOR_SESSION_TOKEN;
  try {
    const ac = new AbortController();
    ac.abort();
    const started = Date.now();
    assert.equal(await resolveCursorSessionCookie({ signal: ac.signal }), null);
    assert.ok(Date.now() - started < 100);
  } finally {
    if (prev === undefined) delete process.env.CURSOR_SESSION_TOKEN;
    else process.env.CURSOR_SESSION_TOKEN = prev;
  }
});

test("resolveCursorSessionCookie stops lookup when aborted mid-flight", async () => {
  const prev = process.env.CURSOR_SESSION_TOKEN;
  delete process.env.CURSOR_SESSION_TOKEN;
  try {
    const ac = new AbortController();
    let secondStarted = 0;
    /** @param {AbortSignal | undefined} signal */
    const hangUntilAbort = (signal) =>
      new Promise((resolve) => {
        const done = () => resolve(null);
        if (signal?.aborted) {
          done();
          return;
        }
        signal?.addEventListener("abort", done, { once: true });
      });
    /** @param {AbortSignal | undefined} _signal */
    const second = async (_signal) => {
      secondStarted += 1;
      return "should-not-run";
    };

    const started = Date.now();
    const pending = resolveCursorSessionCookie({
      signal: ac.signal,
      sources: [hangUntilAbort, second],
    });
    setTimeout(() => ac.abort(), 40);
    assert.equal(await pending, null);
    assert.equal(secondStarted, 0);
    assert.ok(Date.now() - started < 500);
  } finally {
    if (prev === undefined) delete process.env.CURSOR_SESSION_TOKEN;
    else process.env.CURSOR_SESSION_TOKEN = prev;
  }
});

test("resolveCursorSessionCookie uses injected source when not aborted", async () => {
  const prev = process.env.CURSOR_SESSION_TOKEN;
  delete process.env.CURSOR_SESSION_TOKEN;
  try {
    const sessionPayload = Buffer.from(
      JSON.stringify({ sub: "github|user_01ABC", type: "session" }),
    ).toString("base64url");
    const sessionJwt = `hdr.${sessionPayload}.sig`;
    let calls = 0;
    const cookie = await resolveCursorSessionCookie({
      sources: [
        async () => {
          calls += 1;
          return sessionJwt;
        },
        async () => {
          calls += 1;
          return "unused";
        },
      ],
    });
    assert.equal(cookie, "user_01ABC::" + sessionJwt);
    assert.equal(calls, 1);
  } finally {
    if (prev === undefined) delete process.env.CURSOR_SESSION_TOKEN;
    else process.env.CURSOR_SESSION_TOKEN = prev;
  }
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

test("selectUsableStateDbToken skips api_key then keeps session", () => {
  const sessionPayload = Buffer.from(
    JSON.stringify({ sub: "github|user_01ABC", type: "session" }),
  ).toString("base64url");
  const sessionJwt = `hdr.${sessionPayload}.sig`;
  const apiPayload = Buffer.from(
    JSON.stringify({ sub: "github|user_01ABC", type: "api_key_token" }),
  ).toString("base64url");
  const apiJwt = `hdr.${apiPayload}.sig`;

  assert.equal(selectUsableStateDbToken([apiJwt, sessionJwt]), sessionJwt);
  assert.equal(selectUsableStateDbToken([apiJwt]), null);
});

test("jwtClaims and cookieIdFromClaims parse sub", () => {
  const payload = Buffer.from(JSON.stringify({ sub: "auth0|user_xyz" })).toString("base64url");
  const claims = jwtClaims(`x.${payload}.y`);
  assert.equal(cookieIdFromClaims(claims), "user_xyz");
  assert.deepEqual(jwtClaims("not-a-jwt"), {});
  // payload "null" — must not throw; empty claims so env token can fall through
  assert.deepEqual(jwtClaims("x.bnVsbA.y"), {});
  assert.equal(cookieFromToken("x.bnVsbA.y"), null);
});
