import { existsSync, mkdirSync, readFileSync, rmSync, writeFileSync } from "node:fs";

import { daemonPaths } from "./paths.js";
import { YIELD_MS } from "./policy.js";

/**
 * @param {string} configDir
 * @param {number} [now]
 */
export function writeYield(configDir, now = Date.now()) {
  const paths = daemonPaths(configDir);
  mkdirSync(paths.dir, { recursive: true, mode: 0o700 });
  writeFileSync(paths.yieldFile, JSON.stringify({ until: now + YIELD_MS }), { mode: 0o600 });
}

/**
 * @param {string} configDir
 * @param {number} [now]
 */
export function yieldActive(configDir, now = Date.now()) {
  try {
    const until = JSON.parse(readFileSync(daemonPaths(configDir).yieldFile, "utf8")).until;
    return typeof until === "number" && until > now;
  } catch {
    return false;
  }
}

/**
 * @param {string} configDir
 */
export function clearYield(configDir) {
  rmSync(daemonPaths(configDir).yieldFile, { force: true });
}

/**
 * @param {string} configDir
 */
export function writeDisable(configDir) {
  const paths = daemonPaths(configDir);
  mkdirSync(paths.dir, { recursive: true, mode: 0o700 });
  writeFileSync(paths.disabled, "{}\n", { mode: 0o600 });
}

/**
 * @param {string} configDir
 */
export function disableActive(configDir) {
  return existsSync(daemonPaths(configDir).disabled);
}

/**
 * @param {string} configDir
 */
export function clearDisable(configDir) {
  rmSync(daemonPaths(configDir).disabled, { force: true });
}

/**
 * @param {string} configDir
 */
export function supervisorInstalled(configDir) {
  return existsSync(daemonPaths(configDir).supervisor);
}
