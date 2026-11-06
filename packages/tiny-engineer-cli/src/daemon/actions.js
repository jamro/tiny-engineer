import { spawn, spawnSync } from "node:child_process";
import { homedir } from "node:os";

export const ACTION_CAP = 8;
export const DEFAULT_TIMEOUT_MS = 30_000;
const STDERR_CAP = 4096;

const ENV_KEYS = ["HOME", "PATH", "USER", "LANG", "TMPDIR", "SystemRoot", "COMSPEC", "PATHEXT"];

/**
 * @typedef {{ kind: "none" } | { kind: "exit" | "timeout" | "skipped", type: string, program: string, code?: number }} LastAction
 */

/**
 * Allowlisted child environment. Does not copy the parent env.
 * @param {Record<string, unknown>} event
 * @param {NodeJS.ProcessEnv} parent
 * @param {string} home
 */
export function actionChildEnv(event, parent, home) {
  /** @type {NodeJS.ProcessEnv} */
  const env = { HOME: home };
  for (const key of ENV_KEYS) {
    if (key === "HOME") continue;
    const value = parent[key];
    if (typeof value === "string") env[key] = value;
  }
  env.TINY_ENGINEER_EVENT_TYPE = typeof event.type === "string" ? event.type : "";
  env.TINY_ENGINEER_EVENT_SOURCE = typeof event.source === "string" ? event.source : "";
  env.TINY_ENGINEER_EVENT_MS = typeof event.ms === "number" ? String(event.ms) : "";
  return env;
}

/**
 * @param {number} pid
 */
export function killProcessTree(pid) {
  if (!Number.isFinite(pid) || pid <= 0) return;
  if (process.platform === "win32") {
    spawnSync("taskkill", ["/pid", String(pid), "/t", "/f"], { windowsHide: true });
    return;
  }
  try {
    process.kill(-pid, "SIGTERM");
  } catch {
    try {
      process.kill(pid, "SIGTERM");
    } catch {
      // already gone
    }
  }
}

/**
 * @param {{
 *   spawnImpl?: typeof spawn,
 *   killTree?: (pid: number) => void,
 *   home?: string,
 *   parentEnv?: NodeJS.ProcessEnv,
 *   setTimer?: typeof setTimeout,
 *   clearTimer?: typeof clearTimeout,
 *   onOutcome?: (action: LastAction) => void,
 * }} [opts]
 */
export function createActionRunner(opts = {}) {
  const spawnImpl = opts.spawnImpl ?? spawn;
  const killTree = opts.killTree ?? killProcessTree;
  const home = opts.home ?? homedir();
  const parentEnv = opts.parentEnv ?? process.env;
  const setTimer = opts.setTimer ?? setTimeout;
  const clearTimer = opts.clearTimer ?? clearTimeout;
  const onOutcome = opts.onOutcome ?? (() => {});
  /** @type {Set<number>} */
  const busy = new Set();
  /** @type {Set<{ pid?: number, kill?: () => void }>} */
  const live = new Set();
  /** @type {LastAction} */
  let last = { kind: "none" };

  /**
   * @param {import("./rules.js").ReactionRule} rule
   * @param {Record<string, unknown>} event
   */
  function start(rule, event) {
    const program = rule.run[0] ?? "";
    if (busy.has(rule.id) || live.size >= ACTION_CAP) {
      last = { kind: "skipped", type: String(event.type ?? ""), program };
      onOutcome(last);
      return "skipped";
    }
    const child = spawnImpl(program, rule.run.slice(1), {
      shell: false,
      cwd: home,
      detached: process.platform !== "win32",
      stdio: ["pipe", "ignore", "pipe"],
      env: actionChildEnv(event, parentEnv, home),
    });
    busy.add(rule.id);
    live.add(child);
    let timedOut = false;
    const limit = rule.timeoutMs ?? DEFAULT_TIMEOUT_MS;
    const timer = setTimer(() => {
      timedOut = true;
      last = { kind: "timeout", type: String(event.type ?? ""), program };
      onOutcome(last);
      if (typeof child.pid === "number") killTree(child.pid);
      else child.kill?.();
    }, limit);
    let stderr = "";
    child.stderr?.on("data", (chunk) => {
      stderr = (stderr + String(chunk)).slice(-STDERR_CAP);
      void stderr;
    });
    const body = `${JSON.stringify(event)}\n`;
    child.stdin?.on("error", () => {});
    child.stdin?.write(body);
    child.stdin?.end();
    child.on("exit", (code) => {
      clearTimer(timer);
      busy.delete(rule.id);
      live.delete(child);
      if (timedOut) return;
      last = {
        kind: "exit",
        type: String(event.type ?? ""),
        program,
        code: typeof code === "number" ? code : 1,
      };
      onOutcome(last);
    });
    child.on("error", () => {
      clearTimer(timer);
      busy.delete(rule.id);
      live.delete(child);
      if (timedOut) return;
      last = { kind: "exit", type: String(event.type ?? ""), program, code: 1 };
      onOutcome(last);
    });
    return "started";
  }

  return {
    start,
    /**
     * Drop per-rule slots after a reload. Live children still count toward the cap.
     */
    retarget() {
      busy.clear();
    },
    killAll() {
      for (const child of live) {
        if (typeof child.pid === "number") killTree(child.pid);
        else child.kill?.();
      }
    },
    get lastAction() {
      return last;
    },
    get liveCount() {
      return live.size;
    },
  };
}
