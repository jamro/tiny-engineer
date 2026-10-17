import { spawn } from "node:child_process";

/**
 * Fail-open serial anim via the unified CLI (honors TINY_ENGINEER_PORT / cache).
 * @param {string} animName
 * @returns {Promise<void>}
 */
export function postAnimSerial(animName) {
  return new Promise((resolve) => {
    const args = ["anim", animName, "--transport", "serial", "-q"];
    const port = process.env.TINY_ENGINEER_PORT?.trim();
    if (port) args.push("--port", port);

    const bin = process.env.TINY_ENGINEER_CLI?.trim() || "tiny-engineer";
    const proc = spawn(bin, args, { stdio: "ignore", env: process.env });
    const timer = setTimeout(() => {
      proc.kill("SIGKILL");
      resolve();
    }, 2500);
    proc.on("error", () => {
      clearTimeout(timer);
      resolve();
    });
    proc.on("close", () => {
      clearTimeout(timer);
      resolve();
    });
  });
}

/**
 * POST /anim?name=… with a 2s timeout, or serial via CLI when transport is serial.
 * Errors are swallowed.
 * @param {string} baseUrl
 * @param {string} animName
 * @param {string | null} [token]
 */
export async function postAnim(baseUrl, animName, token = null) {
  const transport = process.env.TINY_ENGINEER_TRANSPORT?.trim().toLowerCase();
  if (transport === "serial") {
    await postAnimSerial(animName);
    return;
  }

  const base = baseUrl.replace(/\/+$/, "");
  const url = `${base}/anim?name=${encodeURIComponent(animName)}`;
  /** @type {RequestInit} */
  const init = {
    method: "POST",
    signal: AbortSignal.timeout(2000),
  };
  if (token) {
    init.headers = { Authorization: `Bearer ${token}` };
  }
  try {
    await fetch(url, init);
  } catch {
    // Robot offline / timeout — never stall the agent.
  }
}
