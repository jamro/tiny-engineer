import test from "node:test";
import assert from "node:assert/strict";

import {
  curlArgs,
  curlConfig,
  fetchClaudeUsage,
} from "../src/integrations/claude-code/usage-api.js";

/**
 * @param {number} status
 * @param {Record<string, unknown>} [body]
 */
function fakeResponse(status, body = {}) {
  return {
    status,
    ok: status >= 200 && status < 300,
    json: async () => body,
  };
}

test("fetchClaudeUsage returns fetch json and skips curl", async () => {
  let curls = 0;
  const body = { five_hour: { utilization: 1 } };
  const json = await fetchClaudeUsage("token", {
    fetchImpl: async () => fakeResponse(200, body),
    spawnCurl: async () => {
      curls += 1;
      return null;
    },
  });
  assert.equal(curls, 0);
  assert.deepEqual(json, body);
});

test("fetchClaudeUsage retries curl on 403 without putting the token in argv", async () => {
  const token = "secret-token";
  /** @type {string[] | null} */
  let seenArgs = null;
  const json = await fetchClaudeUsage(token, {
    fetchImpl: async () => fakeResponse(403),
    spawnCurl: async (got) => {
      seenArgs = curlArgs();
      assert.equal(got, token);
      return { five_hour: { utilization: 4 } };
    },
  });
  assert.deepEqual(json, { five_hour: { utilization: 4 } });
  assert.deepEqual(seenArgs, ["--config", "-"]);
  assert.equal(seenArgs.some((arg) => arg.includes(token)), false);
  const config = curlConfig(token, 1.5);
  assert.ok(config?.includes(`Bearer ${token}`));
  assert.equal(curlArgs().some((arg) => arg.includes(token)), false);
});

test("fetchClaudeUsage retries curl on network error", async () => {
  let curls = 0;
  const json = await fetchClaudeUsage("token", {
    fetchImpl: async () => {
      throw new Error("socket");
    },
    spawnCurl: async () => {
      curls += 1;
      return { seven_day: { utilization: 2 } };
    },
  });
  assert.equal(curls, 1);
  assert.deepEqual(json, { seven_day: { utilization: 2 } });
});

test("fetchClaudeUsage does not curl on other HTTP errors", async () => {
  let curls = 0;
  const json = await fetchClaudeUsage("token", {
    fetchImpl: async () => fakeResponse(500),
    spawnCurl: async () => {
      curls += 1;
      return { five_hour: { utilization: 1 } };
    },
  });
  assert.equal(json, null);
  assert.equal(curls, 0);
});

test("fetchClaudeUsage abort skips curl", async () => {
  const ac = new AbortController();
  ac.abort();
  let curls = 0;
  let fetches = 0;
  const json = await fetchClaudeUsage("token", {
    signal: ac.signal,
    fetchImpl: async () => {
      fetches += 1;
      return fakeResponse(200, {});
    },
    spawnCurl: async () => {
      curls += 1;
      return {};
    },
  });
  assert.equal(json, null);
  assert.equal(fetches, 0);
  assert.equal(curls, 0);
});

test("fetchClaudeUsage aborts an in-flight curl retry", async () => {
  const ac = new AbortController();
  let killed = false;
  const pending = fetchClaudeUsage("token", {
    signal: ac.signal,
    fetchImpl: async () => {
      throw new Error("network");
    },
    spawnCurl: (_token, opts) =>
      new Promise((resolve) => {
        opts.signal?.addEventListener(
          "abort",
          () => {
            killed = true;
            resolve(null);
          },
          { once: true },
        );
      }),
  });
  setTimeout(() => ac.abort(), 20);
  assert.equal(await pending, null);
  assert.equal(killed, true);
});
