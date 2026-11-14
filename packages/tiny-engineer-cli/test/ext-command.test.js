import test from "node:test";
import assert from "node:assert/strict";

import { KNOWN_EXTENSION_IDS, runExt } from "../src/commands/ext.js";
import { createOutput } from "../src/util/output.js";
import { createStyle } from "../src/util/style.js";

test("KNOWN_EXTENSION_IDS includes lcd1602 only", () => {
  assert.deepEqual([...KNOWN_EXTENSION_IDS], ["lcd1602"]);
});

test("runExt rejects unknown id without contacting the robot", async () => {
  /** @type {string[]} */
  const errs = [];
  const out = createOutput({
    quiet: false,
    style: createStyle({ color: false }),
    log: () => {},
    error: (m) => errs.push(String(m)),
  });
  process.exitCode = 0;
  await runExt(out, ["nope", "text", "get"], false);
  assert.equal(process.exitCode, 1);
  assert.ok(errs.some((e) => e.includes("unknown extension")));
  process.exitCode = 0;
});

test("runExt --help prints lcd1602 usage", async () => {
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
    await runExt(out, ["--help"], false);
    assert.ok(logs.some((l) => l.includes("lcd1602 text get")));
  } finally {
    console.log = original;
  }
});
