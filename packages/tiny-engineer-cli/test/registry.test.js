import test from "node:test";
import assert from "node:assert/strict";
import { all, get, list } from "../src/integrations/registry.js";

test("list includes cursor, claude-code, antigravity", () => {
  assert.deepEqual(list(), ["cursor", "claude-code", "antigravity"]);
});

test("get returns each registered integration", () => {
  for (const id of ["cursor", "claude-code", "antigravity"]) {
    const integration = get(id);
    assert.ok(integration, id);
    assert.equal(integration.id, id);
    assert.equal(typeof integration.parseHook, "function");
    assert.equal(typeof integration.mapToAnim, "function");
  }
});

test("claude-code sets hardExitAfterPost", () => {
  assert.equal(get("claude-code")?.hardExitAfterPost, true);
});

test("get returns undefined for unknown id", () => {
  assert.equal(get("windsurf"), undefined);
});

test("all returns registered integrations", () => {
  const integrations = all();
  assert.equal(integrations.length, 3);
  assert.deepEqual(
    integrations.map((i) => i.id),
    ["cursor", "claude-code", "antigravity"],
  );
});
