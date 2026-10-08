import test from "node:test";
import assert from "node:assert/strict";
import { formatFileModifications } from "../src/integrations/cursor/files.js";
import { createStyle } from "../src/util/style.js";

const plain = createStyle({ color: false });

test("afterFileEdit formats path and edits", () => {
  const lines = formatFileModifications(
    {
      hook_event_name: "afterFileEdit",
      file_path: "/proj/src/a.js",
      edits: [{ old_string: "x", new_string: "y" }],
    },
    plain,
  );
  assert.equal(lines[0], "✎  modified: /proj/src/a.js (1 edit)");
  assert.equal(lines[1], '   − "x" → + "y"');
});

test("afterFileEdit with no edits still headers", () => {
  const lines = formatFileModifications(
    { hook_event_name: "afterFileEdit", file_path: "/f.ts", edits: [] },
    plain,
  );
  assert.deepEqual(lines, ["✎  modified: /f.ts (0 edits)"]);
});

test("missing file_path yields nothing", () => {
  assert.deepEqual(
    formatFileModifications({ hook_event_name: "afterFileEdit" }, plain),
    [],
  );
});

test("truncates long edit sides", () => {
  const long = "a".repeat(80);
  const lines = formatFileModifications(
    {
      hook_event_name: "afterTabFileEdit",
      file_path: "/x",
      edits: [{ old_string: long, new_string: "b" }],
    },
    plain,
  );
  assert.ok(lines[1].includes("…"));
  assert.ok(lines[1].length < 200);
});

test("collapses newlines in edits", () => {
  const lines = formatFileModifications(
    {
      hook_event_name: "afterFileEdit",
      file_path: "/x",
      edits: [{ old_string: "a\nb", new_string: "c\nd" }],
    },
    plain,
  );
  assert.equal(lines[1], '   − "a\\nb" → + "c\\nd"');
});

test("preToolUse Write reports touching path", () => {
  const lines = formatFileModifications(
    {
      hook_event_name: "preToolUse",
      tool_name: "Write",
      tool_input: { file_path: "/proj/new.js" },
    },
    plain,
  );
  assert.deepEqual(lines, ["✎  touching: /proj/new.js"]);
});

test("preToolUse Read reports nothing", () => {
  assert.deepEqual(
    formatFileModifications(
      {
        hook_event_name: "preToolUse",
        tool_name: "Read",
        tool_input: { file_path: "/proj/a.js" },
      },
      plain,
    ),
    [],
  );
});

test("color style wraps icons", () => {
  const style = createStyle({ color: true });
  const lines = formatFileModifications(
    {
      hook_event_name: "afterFileEdit",
      file_path: "/a",
      edits: [{ old_string: "1", new_string: "2" }],
    },
    style,
  );
  assert.ok(lines[0].includes("\x1b[36m"));
  assert.ok(lines[1].includes("\x1b[31m"));
  assert.ok(lines[1].includes("\x1b[32m"));
});
