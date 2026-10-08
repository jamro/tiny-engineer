import test from "node:test";
import assert from "node:assert/strict";
import { createOutput } from "../src/util/output.js";
import { createStyle } from "../src/util/style.js";

test("ok prints with checkmark; quiet suppresses", () => {
  /** @type {string[]} */
  const logs = [];
  /** @type {string[]} */
  const errs = [];
  const style = createStyle({ color: false });
  const out = createOutput({
    quiet: false,
    style,
    log: (m) => logs.push(String(m)),
    error: (m) => errs.push(String(m)),
  });

  out.ok("animation ring → http://x");
  assert.deepEqual(logs, ["✓  animation ring → http://x"]);

  const quiet = createOutput({
    quiet: true,
    style,
    log: (m) => logs.push(String(m)),
    error: (m) => errs.push(String(m)),
  });
  quiet.ok("hidden");
  quiet.info("also hidden");
  assert.deepEqual(logs, ["✓  animation ring → http://x"]);
  assert.deepEqual(errs, []);
});

test("error always prints with icon and prefix", () => {
  /** @type {string[]} */
  const errs = [];
  const out = createOutput({
    quiet: true,
    style: createStyle({ color: false }),
    log: () => {},
    error: (m) => errs.push(String(m)),
  });
  out.error("unknown animation: foobar");
  assert.deepEqual(errs, ["✗  error: unknown animation: foobar"]);
});

test("info goes to stderr writer and is quiet-sensitive", () => {
  /** @type {string[]} */
  const errs = [];
  const style = createStyle({ color: false });
  const out = createOutput({
    quiet: false,
    style,
    log: () => {},
    error: (m) => errs.push(String(m)),
  });
  out.info("✎  modified: a.js (1 edit)");
  assert.deepEqual(errs, ["✎  modified: a.js (1 edit)"]);
});
