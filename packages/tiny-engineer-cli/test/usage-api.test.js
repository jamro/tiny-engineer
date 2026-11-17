import test from "node:test";
import assert from "node:assert/strict";

import { fetchCursorLimits } from "../src/integrations/cursor/usage-api.js";

test("fetchCursorLimits falls back when primary has no usable percent", async () => {
  /** @type {string[]} */
  const paths = [];
  const fetchImpl = async (url) => {
    paths.push(new URL(String(url)).pathname);
    if (paths.length === 1) {
      return {
        ok: true,
        json: async () => ({ planUsage: {} }),
      };
    }
    return {
      ok: true,
      json: async () => ({ planUsage: { autoPercentUsed: 10 } }),
    };
  };
  const result = await fetchCursorLimits("id::jwt", { fetchImpl: /** @type {typeof fetch} */ (fetchImpl) });
  assert.deepEqual(paths, [
    "/api/dashboard/get-current-period-usage",
    "/api/usage-summary",
  ]);
  assert.deepEqual(result, { planUsage: { autoPercentUsed: 10 } });
});

test("fetchCursorLimits skips fallback when primary has usable percent", async () => {
  /** @type {string[]} */
  const paths = [];
  const fetchImpl = async (url) => {
    paths.push(new URL(String(url)).pathname);
    return {
      ok: true,
      json: async () => ({ planUsage: { autoPercentUsed: 40 } }),
    };
  };
  const result = await fetchCursorLimits("id::jwt", { fetchImpl: /** @type {typeof fetch} */ (fetchImpl) });
  assert.deepEqual(paths, ["/api/dashboard/get-current-period-usage"]);
  assert.deepEqual(result, { planUsage: { autoPercentUsed: 40 } });
});
