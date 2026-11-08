import assert from "node:assert/strict";
import test from "node:test";

import { createRuleStore, matchRules, parseRulesDocument } from "../src/daemon/rules.js";

test("minimal rule loads and extra fields are ignored", () => {
  const parsed = parseRulesDocument({
    v: 1,
    rules: [{ on: "button.click", run: ["/usr/bin/open", "-a", "Music"], note: "hi" }],
  });
  assert.equal(parsed.ok, true);
  if (!parsed.ok) return;
  assert.equal(parsed.rules.length, 1);
  assert.equal(parsed.rules[0].on, "button.click");
  assert.deepEqual(parsed.rules[0].run, ["/usr/bin/open", "-a", "Music"]);
  assert.equal("note" in parsed.rules[0], false);
});

test("optional source and timeoutMs are kept", () => {
  const parsed = parseRulesDocument({
    v: 1,
    rules: [{ on: "button.click", source: "push-button", timeoutMs: 1000, run: ["/bin/true"] }],
  });
  assert.equal(parsed.ok, true);
  if (!parsed.ok) return;
  assert.equal(parsed.rules[0].source, "push-button");
  assert.equal(parsed.rules[0].timeoutMs, 1000);
});

test("bad shape is rejected", () => {
  assert.equal(parseRulesDocument({ v: 2, rules: [] }).ok, false);
  assert.equal(parseRulesDocument({ v: 1, rules: [{ on: "x", run: [] }] }).ok, false);
  assert.equal(parseRulesDocument({ v: 1, rules: [{ on: "x", run: ["/bin/true"], timeoutMs: 1.5 }] }).ok, false);
  assert.equal(parseRulesDocument({ v: 1, rules: [{ on: "x", run: ["/bin/true"], source: 1 }] }).ok, false);
});

test("invalid text keeps the last good set and empty list clears the error", () => {
  const store = createRuleStore();
  store.loadText(JSON.stringify({
    v: 1,
    rules: [
      { on: "button.click", run: ["/bin/a"] },
      { on: "button.up", run: ["/bin/b"] },
    ],
  }));
  assert.equal(store.rules.length, 2);
  store.loadText("{");
  assert.equal(store.rules.length, 2);
  assert.equal(store.fileError, "invalid rules.json");
  store.loadText(JSON.stringify({ v: 1, rules: [] }));
  assert.equal(store.rules.length, 0);
  assert.equal(store.fileError, null);
});

test("first invalid load is zero rules with an error", () => {
  const store = createRuleStore();
  store.loadText("nope");
  assert.equal(store.rules.length, 0);
  assert.equal(store.fileError, "invalid rules.json");
});

test("missing file is zero rules and no error", () => {
  const store = createRuleStore();
  store.loadMissing();
  assert.deepEqual(store.rules, []);
  assert.equal(store.fileError, null);
});

test("match is file order, source, and envelope v", () => {
  const parsed = parseRulesDocument({
    v: 1,
    rules: [
      { on: "button.click", run: ["/bin/a"] },
      { on: "button.click", source: "push-button", run: ["/bin/b"] },
      { on: "button.up", run: ["/bin/c"] },
    ],
  });
  assert.equal(parsed.ok, true);
  if (!parsed.ok) return;
  const both = matchRules(parsed.rules, { v: 1, type: "button.click", source: "push-button" });
  assert.deepEqual(both.map((rule) => rule.run[0]), ["/bin/a", "/bin/b"]);
  const filtered = matchRules(parsed.rules, { v: 1, type: "button.click", source: "other" });
  assert.deepEqual(filtered.map((rule) => rule.run[0]), ["/bin/a"]);
  assert.deepEqual(matchRules(parsed.rules, { v: 2, type: "button.click" }), []);
});
