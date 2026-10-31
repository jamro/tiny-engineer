import assert from "node:assert/strict";
import { mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

import {
  classifyChannel,
  decideDaemonAction,
  isGitInstallUrl,
  loadIdentity,
  readInstallVersion,
  sourceHash,
} from "../src/daemon/policy.js";

const repoRoot = fileURLToPath(new URL("../../..", import.meta.url));
const packageRoot = fileURLToPath(new URL("..", import.meta.url));

/**
 * @param {string} root
 * @param {string} version
 * @param {string} source
 */
function writePackage(root, version, source) {
  mkdirSync(join(root, "bin"), { recursive: true });
  mkdirSync(join(root, "src"), { recursive: true });
  writeFileSync(join(root, "package.json"), JSON.stringify({ version }));
  writeFileSync(join(root, "bin", "tiny-engineer.js"), "#!/usr/bin/env node\n");
  writeFileSync(join(root, "src", "main.js"), source);
}

test("channel follows npm install path vs packages checkout", () => {
  const npmPath = join(tmpdir(), "node_modules", "tiny-engineer");
  const devPath = join(repoRoot, "packages", "tiny-engineer-cli");
  assert.equal(classifyChannel(npmPath), "npm");
  assert.equal(classifyChannel(devPath), "dev");
  assert.equal(readInstallVersion(packageRoot), JSON.parse(readFileSync(join(packageRoot, "package.json"), "utf8")).version);
  assert.equal(isGitInstallUrl("git+https://github.com/jamro/tiny-engineer.git"), true);
  assert.throws(() => readInstallVersion("git@github.com:jamro/tiny-engineer.git"), /git URL/);
  assert.throws(() => classifyChannel("github:jamro/tiny-engineer"), /git URL/);
});

test("source hash changes when src bytes change", () => {
  const dir = mkdtempSync(join(tmpdir(), "te-hash-"));
  try {
    writePackage(dir, "0.3.0", "a");
    const first = sourceHash(dir);
    writeFileSync(join(dir, "src", "main.js"), "b");
    assert.notEqual(sourceHash(dir), first);
    assert.equal(sourceHash(dir), sourceHash(dir));
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

/**
 * @param {Partial<import("../src/daemon/policy.js").DaemonIdentity>} over
 */
function id(over) {
  return {
    proto: 1,
    version: "0.3.0",
    channel: /** @type {"npm" | "dev"} */ ("npm"),
    packageRoot: "/cache/a/node_modules/tiny-engineer",
    sourceHash: "h",
    ...over,
  };
}

test("decideDaemonAction matches npm and dev replace rules", () => {
  const npmOld = id({ version: "0.3.0", packageRoot: "/npx/a/node_modules/tiny-engineer" });
  const npmNew = id({ version: "0.4.0", packageRoot: "/npx/b/node_modules/tiny-engineer" });
  const npmSameOtherCache = id({ version: "0.3.0", packageRoot: "/npx/b/node_modules/tiny-engineer", sourceHash: "other" });
  const dev = id({
    channel: "dev",
    version: "0.3.0",
    packageRoot: "/src/packages/tiny-engineer-cli",
    sourceHash: "dev",
  });
  const devLower = id({ ...dev, version: "0.2.0" });
  const devEdited = id({ ...dev, sourceHash: "edited" });
  const devOther = id({ ...dev, packageRoot: "/other/packages/tiny-engineer-cli" });

  assert.equal(decideDaemonAction(npmNew, npmOld), "replace");
  assert.equal(decideDaemonAction(npmSameOtherCache, npmOld), "keep");
  assert.equal(decideDaemonAction(npmOld, npmNew), "keep");
  assert.equal(decideDaemonAction(devLower, npmOld), "replace");
  assert.equal(decideDaemonAction(npmNew, dev), "keep");
  assert.equal(decideDaemonAction(devEdited, dev), "replace");
  assert.equal(decideDaemonAction(devOther, dev), "keep");
  assert.equal(decideDaemonAction(id({ proto: 1 }), id({ proto: 2 })), "reject-proto");
  assert.equal(decideDaemonAction(id({ proto: 2 }), id({ proto: 1 })), "replace");
  assert.equal(loadIdentity(packageRoot).channel, "dev");
});
