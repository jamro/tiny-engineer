import test from "node:test";
import assert from "node:assert/strict";
import { createStyle } from "../src/util/style.js";

test("createStyle color false leaves strings plain", () => {
  const style = createStyle({ color: false });
  assert.equal(style.enabled, false);
  assert.equal(style.green("ok"), "ok");
  assert.equal(style.red("no"), "no");
  assert.equal(style.cyan("x"), "x");
  assert.equal(style.dim("d"), "d");
});

test("createStyle color true wraps with ANSI", () => {
  const style = createStyle({ color: true });
  assert.equal(style.enabled, true);
  assert.equal(style.green("ok"), "\x1b[32mok\x1b[0m");
  assert.equal(style.red("no"), "\x1b[31mno\x1b[0m");
  assert.equal(style.cyan("x"), "\x1b[36mx\x1b[0m");
  assert.ok(style.dim("d").includes("\x1b[2m"));
});
