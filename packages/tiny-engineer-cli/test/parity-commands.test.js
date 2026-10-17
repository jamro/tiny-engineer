import test from "node:test";
import assert from "node:assert/strict";

import { createOutput } from "../src/util/output.js";
import { createStyle } from "../src/util/style.js";
import { runReset, runTest } from "../src/commands/parity.js";

test("runTest rejects unknown test name", async () => {
  /** @type {string[]} */
  const errs = [];
  const out = createOutput({
    quiet: false,
    style: createStyle({ color: false }),
    log: () => {},
    error: (m) => errs.push(String(m)),
  });
  await runTest(out, ["nope"], false);
  assert.equal(process.exitCode, 1);
  assert.ok(errs.some((e) => e.includes("audio")));
  process.exitCode = 0;
});

test("runReset --help prints usage", async () => {
  /** @type {string[]} */
  const logs = [];
  const original = console.log;
  console.log = (...args) => logs.push(args.join(" "));
  const out = createOutput({
    quiet: false,
    style: createStyle({ color: false }),
    log: () => {},
    error: () => {},
  });
  try {
    await runReset(out, ["--help"], false);
    assert.ok(logs.some((l) => l.includes("Factory-reset")));
  } finally {
    console.log = original;
  }
});
