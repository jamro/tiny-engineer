import { execFileSync } from "node:child_process";

/**
 * @param {number} pid
 * @returns {"dead" | "alive"}
 */
export function pidLiveness(pid) {
  if (!pid) return "dead";
  try {
    process.kill(pid, 0);
    return "alive";
  } catch (err) {
    const code = err && typeof err === "object" && "code" in err ? err.code : "";
    if (code === "ESRCH") return "dead";
    return "alive";
  }
}

/**
 * @param {number} pid
 * @returns {string | null} null when the command line cannot be read
 */
export function readProcessCommand(pid) {
  if (process.platform === "win32") return null;
  try {
    return execFileSync("ps", ["-p", String(pid), "-o", "command="], {
      encoding: "utf8",
      timeout: 1000,
    }).trim();
  } catch {
    return null;
  }
}

/**
 * @param {number | undefined} pid
 * @param {(pid: number) => string | null} [readCommand]
 * @returns {"none" | "dead" | "ours" | "foreign" | "unknown"}
 */
export function classifyPid(pid, readCommand = readProcessCommand) {
  if (!pid) return "none";
  if (pidLiveness(pid) === "dead") return "dead";
  const command = readCommand(pid);
  if (command == null) return "unknown";
  if (command.includes("tiny-engineer") && command.includes("daemon")) return "ours";
  return "foreign";
}

/**
 * @param {number} ms
 */
export function delay(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

/**
 * SIGTERM, then SIGKILL if the pid is still alive.
 * @param {number} pid
 * @param {{ signal?: (pid: number, sig: NodeJS.Signals | 0) => void, alive?: (pid: number) => boolean, waitMs?: number }} [opts]
 */
export async function terminatePid(pid, opts = {}) {
  const signal = opts.signal ?? ((target, sig) => process.kill(target, sig));
  const alive = opts.alive ?? ((target) => pidLiveness(target) === "alive");
  const waitMs = opts.waitMs ?? 500;
  try {
    signal(pid, "SIGTERM");
  } catch {
    return;
  }
  const start = Date.now();
  while (Date.now() - start < waitMs) {
    if (!alive(pid)) return;
    await delay(20);
  }
  try {
    signal(pid, "SIGKILL");
  } catch {
    // already gone
  }
}
