import { mkdirSync, readFileSync, writeFileSync } from "node:fs";

import { daemonPaths } from "./paths.js";

/**
 * @param {string} configDir
 * @param {{ execPath: string, script: string }} target
 */
export function writeDaemonTarget(configDir, target) {
  const paths = daemonPaths(configDir);
  mkdirSync(paths.dir, { recursive: true, mode: 0o700 });
  writeFileSync(paths.target, `${JSON.stringify(target)}\n`, { mode: 0o600 });
}

/**
 * @param {string} configDir
 * @returns {{ execPath: string, script: string } | null}
 */
export function readDaemonTarget(configDir) {
  try {
    const body = JSON.parse(readFileSync(daemonPaths(configDir).target, "utf8"));
    if (typeof body.execPath !== "string" || typeof body.script !== "string") return null;
    return { execPath: body.execPath, script: body.script };
  } catch {
    return null;
  }
}
