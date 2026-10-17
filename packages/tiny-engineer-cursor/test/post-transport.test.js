import test from "node:test";
import assert from "node:assert/strict";
import { mkdtempSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

import { postAnim } from "../src/post.js";

test("postAnim serial transport shells out fail-open with short timeout", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-post-"));
  const marker = join(dir, "ran");
  const stub = join(dir, "stub.js");
  writeFileSync(
    stub,
    `import { writeFileSync } from "node:fs"; writeFileSync(${JSON.stringify(marker)}, "ok");\n`,
    "utf8",
  );

  const prevTransport = process.env.TINY_ENGINEER_TRANSPORT;
  const prevCli = process.env.TINY_ENGINEER_CLI;
  process.env.TINY_ENGINEER_TRANSPORT = "serial";
  process.env.TINY_ENGINEER_CLI = process.execPath;

  // spawn(bin, args) — bin is node, but args are anim... So set CLI to a wrapper script via env.
  // postAnimSerial uses spawn(bin, args) where bin is TINY_ENGINEER_CLI.
  // Point CLI at node and prepend -e? Better: set CLI to stub via shell script equivalent.
  process.env.TINY_ENGINEER_CLI = stub;

  try {
    // stub is a .js file — spawn needs node. Use a small bash/node launcher.
    writeFileSync(
      join(dir, "launcher"),
      `#!/usr/bin/env node\nimport { writeFileSync } from "node:fs";\nwriteFileSync(${JSON.stringify(marker)}, process.argv.slice(2).join(" "));\n`,
      { encoding: "utf8", mode: 0o755 },
    );
    process.env.TINY_ENGINEER_CLI = join(dir, "launcher");

    await postAnim("http://unused", "ring", null);
    const ran = readFileSync(marker, "utf8");
    assert.ok(ran.includes("anim"));
    assert.ok(ran.includes("serial"));
  } finally {
    if (prevTransport === undefined) delete process.env.TINY_ENGINEER_TRANSPORT;
    else process.env.TINY_ENGINEER_TRANSPORT = prevTransport;
    if (prevCli === undefined) delete process.env.TINY_ENGINEER_CLI;
    else process.env.TINY_ENGINEER_CLI = prevCli;
    rmSync(dir, { recursive: true, force: true });
  }
});
