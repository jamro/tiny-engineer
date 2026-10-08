import test from "node:test";
import assert from "node:assert/strict";
import { all, get, list } from "../src/integrations/registry.js";

test("list includes cursor", () => {
  assert.deepEqual(list(), ["cursor"]);
});

test("get returns cursor integration", () => {
  const integration = get("cursor");
  assert.ok(integration);
  assert.equal(integration.id, "cursor");
  assert.equal(integration.name, "Cursor");
  assert.equal(typeof integration.parseHook, "function");
  assert.equal(typeof integration.mapToAnim, "function");
});

test("get returns undefined for unknown id", () => {
  assert.equal(get("windsurf"), undefined);
});

test("all returns registered integrations", () => {
  const integrations = all();
  assert.equal(integrations.length, 1);
  assert.equal(integrations[0].id, "cursor");
});
