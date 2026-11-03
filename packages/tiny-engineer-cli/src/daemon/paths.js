import { createHash } from "node:crypto";
import { join } from "node:path";

import { userConfigDir } from "../config/paths.js";

/**
 * POSIX keeps a socket file in the config directory. Windows named pipes
 * live in a global namespace, so the name is a hash of that directory.
 * @param {string} configDir
 * @param {NodeJS.Platform} [platform]
 */
export function daemonIpcPath(configDir, platform = process.platform) {
  if (platform === "win32") {
    const id = createHash("sha256").update(configDir).digest("hex").slice(0, 16);
    return `\\\\.\\pipe\\tiny-engineer-${id}`;
  }
  return join(configDir, "daemon.sock");
}

/**
 * Lock held by the login wrapper. Distinct from the daemon IPC socket.
 * @param {string} configDir
 * @param {NodeJS.Platform} [platform]
 */
export function wrapperLockPath(configDir, platform = process.platform) {
  if (platform === "win32") {
    const id = createHash("sha256").update(configDir).digest("hex").slice(0, 16);
    return `\\\\.\\pipe\\tiny-engineer-wrapper-${id}`;
  }
  return join(configDir, "daemon.wrapper.sock");
}

/**
 * @param {string} [configDir]
 */
export function daemonPaths(configDir = userConfigDir()) {
  return {
    dir: configDir,
    sock: daemonIpcPath(configDir),
    state: join(configDir, "daemon.json"),
    yieldFile: join(configDir, "daemon.yield"),
    disabled: join(configDir, "daemon.disabled"),
    target: join(configDir, "daemon.target"),
    supervisor: join(configDir, "daemon.supervisor"),
    wrapper: join(configDir, "daemon-wrapper.js"),
    wrapperPid: join(configDir, "daemon.wrapper.pid"),
    wrapperLock: wrapperLockPath(configDir),
    lock: join(configDir, "daemon.lock"),
    log: join(configDir, "daemon.log"),
  };
}
