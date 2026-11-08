import { createHash } from "node:crypto";
import { readdirSync, readFileSync, statSync } from "node:fs";
import { basename, dirname, join, relative, resolve } from "node:path";
import { fileURLToPath } from "node:url";

export const DAEMON_PROTO = 1;
export const YIELD_MS = 180_000;
export const YIELD_ERROR =
  "serial yielded for upload; run tiny-engineer daemon start to clear it";
export const DISABLE_ERROR =
  "daemon disabled; run tiny-engineer daemon start to clear it";

/**
 * @param {string} value
 * @returns {boolean}
 */
export function isGitInstallUrl(value) {
  return /^(git\+|git@|github:|https?:\/\/github\.com\/)/.test(value) || value.includes("git@github.com");
}

/**
 * @param {string} packageRoot
 */
export function assertInstallPath(packageRoot) {
  if (isGitInstallUrl(packageRoot)) {
    throw new Error("git URL is not a CLI install path");
  }
}

/**
 * @param {string} packageRoot
 * @returns {"npm" | "dev"}
 */
export function classifyChannel(packageRoot) {
  assertInstallPath(packageRoot);
  const norm = packageRoot.replaceAll("\\", "/");
  if (norm.includes("/node_modules/tiny-engineer")) return "npm";
  return "dev";
}

/**
 * @param {string} packageRoot
 * @returns {string}
 */
export function readInstallVersion(packageRoot) {
  assertInstallPath(packageRoot);
  const pkg = JSON.parse(readFileSync(join(packageRoot, "package.json"), "utf8"));
  if (typeof pkg.version !== "string" || !pkg.version) {
    throw new Error(`missing version in ${packageRoot}`);
  }
  return pkg.version;
}

/**
 * @param {string} version
 * @returns {{ nums: number[], pre: string }}
 */
function parseSemver(version) {
  const [main, pre = ""] = String(version).split("-", 2);
  const nums = main.split(".").map((part) => Number(part) || 0);
  while (nums.length < 3) nums.push(0);
  return { nums: nums.slice(0, 3), pre };
}

/**
 * @param {string} a
 * @param {string} b
 * @returns {-1 | 0 | 1}
 */
export function compareSemver(a, b) {
  const pa = parseSemver(a);
  const pb = parseSemver(b);
  for (let i = 0; i < 3; i++) {
    if (pa.nums[i] !== pb.nums[i]) return pa.nums[i] > pb.nums[i] ? 1 : -1;
  }
  if (pa.pre === pb.pre) return 0;
  if (pa.pre && !pb.pre) return -1;
  if (!pa.pre && pb.pre) return 1;
  return pa.pre > pb.pre ? 1 : -1;
}

/**
 * @param {string} dir
 * @param {string[]} acc
 */
function walkJs(dir, acc) {
  for (const name of readdirSync(dir).sort()) {
    const path = join(dir, name);
    if (statSync(path).isDirectory()) walkJs(path, acc);
    else if (name.endsWith(".js")) acc.push(path);
  }
}

/** @type {Map<string, string>} */
const sourceHashCache = new Map();

/** Clear the in-process sourceHash memo (tests that mutate package files). */
export function clearSourceHashCache() {
  sourceHashCache.clear();
}

/**
 * Hash bin/tiny-engineer.js and src js files. Same bytes => same hash.
 * Results are memoized per resolved package root until {@link clearSourceHashCache}.
 * @param {string} packageRoot
 * @returns {string}
 */
export function sourceHash(packageRoot) {
  assertInstallPath(packageRoot);
  const key = resolve(packageRoot);
  const cached = sourceHashCache.get(key);
  if (cached !== undefined) return cached;
  /** @type {string[]} */
  const files = [];
  walkJs(join(key, "src"), files);
  files.push(join(key, "bin", "tiny-engineer.js"));
  files.sort();
  const hash = createHash("sha256");
  for (const file of files) {
    hash.update(relative(key, file).replaceAll("\\", "/"));
    hash.update("\0");
    hash.update(readFileSync(file));
    hash.update("\0");
  }
  const digest = hash.digest("hex");
  sourceHashCache.set(key, digest);
  return digest;
}

/**
 * @param {string} packageRoot
 */
export function loadIdentity(packageRoot) {
  const channel = classifyChannel(packageRoot);
  return {
    proto: DAEMON_PROTO,
    version: readInstallVersion(packageRoot),
    channel,
    packageRoot,
    // npm replace uses SemVer/channel only; skip the content walk.
    sourceHash: channel === "npm" ? "" : sourceHash(packageRoot),
  };
}

/**
 * @param {string} scriptPath
 * @returns {string}
 */
export function packageRootFromScript(scriptPath) {
  assertInstallPath(scriptPath);
  const resolved = resolve(scriptPath);
  const parent = dirname(resolved);
  if (basename(parent) !== "bin") {
    throw new Error(`not a tiny-engineer CLI entry: ${scriptPath}`);
  }
  return dirname(parent);
}

/**
 * @returns {string}
 */
export function resolveCliScript() {
  const argv1 = process.argv[1] ? resolve(process.argv[1]) : "";
  if (basename(argv1) === "tiny-engineer.js") return argv1;
  return fileURLToPath(new URL("../../bin/tiny-engineer.js", import.meta.url));
}

/**
 * @typedef {{
 *   proto: number,
 *   version: string,
 *   channel: "npm" | "dev",
 *   packageRoot: string,
 *   sourceHash: string,
 * }} DaemonIdentity
 */

/**
 * @param {DaemonIdentity} client
 * @param {DaemonIdentity} daemon
 * @returns {"replace" | "keep" | "reject-proto"}
 */
export function decideDaemonAction(client, daemon) {
  if (!Number.isFinite(client.proto) || !Number.isFinite(daemon.proto)) return "reject-proto";
  if (client.proto < daemon.proto) return "reject-proto";
  if (client.proto > daemon.proto) return "replace";

  if (client.channel === "dev" && daemon.channel === "npm") return "replace";
  if (client.channel === "npm" && daemon.channel === "dev") return "keep";

  const cmp = compareSemver(client.version, daemon.version);
  if (client.channel === "npm" && daemon.channel === "npm") {
    return cmp > 0 ? "replace" : "keep";
  }

  if (client.packageRoot !== daemon.packageRoot) return "keep";
  if (cmp > 0) return "replace";
  if (client.sourceHash !== daemon.sourceHash) return "replace";
  return "keep";
}
