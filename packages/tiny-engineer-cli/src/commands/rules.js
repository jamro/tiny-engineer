import { accessSync, chmodSync, constants, mkdirSync, readFileSync, renameSync, writeFileSync } from "node:fs";
import { delimiter, dirname, isAbsolute, join, resolve } from "node:path";

import { userConfigDir } from "../config/paths.js";
import { tryConnect } from "../daemon/client.js";
import { daemonPaths } from "../daemon/paths.js";
import { classifyPid } from "../daemon/proc.js";
import { parseRulesDocument } from "../daemon/rules.js";

/**
 * Resolve a program to a stored path. Absolute paths are kept as given.
 * A relative path that contains a separator is resolved from the current
 * directory. Any other name is searched on PATH.
 * @param {string} name
 * @param {NodeJS.ProcessEnv} [env]
 * @returns {string | null}
 */
export function resolveProgram(name, env = process.env) {
  if (!name || name.includes("\0")) return null;
  if (isAbsolute(name)) return name;
  const mode = process.platform === "win32" ? constants.F_OK : constants.X_OK;
  if (name.includes("/") || (process.platform === "win32" && name.includes("\\"))) {
    const candidate = resolve(name);
    try {
      accessSync(candidate, mode);
      return candidate;
    } catch {
      return null;
    }
  }
  const dirs = (env.PATH ?? "").split(delimiter).filter(Boolean);
  const exts = process.platform === "win32"
    ? (env.PATHEXT || ".EXE;.CMD;.BAT;.COM").split(";").filter(Boolean)
    : [""];
  for (const dir of dirs) {
    for (const ext of exts) {
      const candidate = join(dir, `${name}${ext}`);
      try {
        accessSync(candidate, mode);
        return candidate;
      } catch {
        // next directory
      }
    }
  }
  return null;
}

/**
 * @param {string} configDir
 * @returns {{ rules: Array<{ on: string, run: string[], source?: string, timeoutMs?: number }>, error?: undefined } | { rules?: undefined, error: string }}
 */
function readRules(configDir) {
  const path = daemonPaths(configDir).rules;
  let text;
  try {
    text = readFileSync(path, "utf8");
  } catch (err) {
    if (err && typeof err === "object" && "code" in err && err.code === "ENOENT") {
      return { rules: [] };
    }
    return { error: "invalid rules.json" };
  }
  let value;
  try {
    value = JSON.parse(text);
  } catch {
    return { error: "invalid rules.json" };
  }
  const parsed = parseRulesDocument(value);
  if (!parsed.ok) return { error: parsed.error };
  return {
    rules: parsed.rules.map((rule) => {
      /** @type {{ on: string, run: string[], source?: string, timeoutMs?: number }} */
      const stored = { on: rule.on, run: rule.run };
      if (rule.source !== undefined) stored.source = rule.source;
      if (rule.timeoutMs !== undefined) stored.timeoutMs = rule.timeoutMs;
      return stored;
    }),
  };
}

/**
 * @param {string} path
 * @param {Array<{ on: string, run: string[], source?: string, timeoutMs?: number }>} rules
 */
function writeRules(path, rules) {
  mkdirSync(dirname(path), { recursive: true, mode: 0o700 });
  const tmp = `${path}.${process.pid}.tmp`;
  const body = `${JSON.stringify({ v: 1, rules }, null, 2)}\n`;
  writeFileSync(tmp, body, { mode: 0o600 });
  if (process.platform !== "win32") chmodSync(tmp, 0o600);
  renameSync(tmp, path);
  if (process.platform !== "win32") chmodSync(path, 0o600);
}

/**
 * Old daemons reload on SIGHUP and do not know reactions.sync.
 * The hello pid is signaled only when ps shows this daemon.
 * @param {Record<string, unknown> | undefined} hello
 */
function sighupLegacy(hello) {
  if (process.platform === "win32") return;
  const pid = Number(hello?.pid);
  if (!Number.isFinite(pid) || pid <= 0) return;
  if (classifyPid(pid) !== "ours") return;
  try {
    process.kill(pid, "SIGHUP");
  } catch {
    // daemon exited between the checks
  }
}

/**
 * Ask a running daemon to reload rules. A missing daemon is not an error.
 * @param {string} configDir
 */
async function signalReload(configDir) {
  /** @type {Awaited<ReturnType<typeof tryConnect>>} */
  let conn;
  try {
    conn = await tryConnect(daemonPaths(configDir).sock, 300);
  } catch {
    return;
  }
  if (!conn) return;
  try {
    const res = await conn.peer.request("reactions.sync", {}, 2000);
    if (typeof res.error === "string" && res.error.startsWith("unknown method")) {
      sighupLegacy(conn.hello);
    }
  } catch {
    // daemon gone or timed out
  } finally {
    conn.socket.destroy();
  }
}

export function rulesHelp() {
  return `Usage: tiny-engineer rules <list|add|remove>

Map a robot event to a host program. Rules live in rules.json next to the
user env file. Programs are stored as absolute paths. A relative program
path is resolved from the current directory. The daemon does not
search PATH, and a login agent only has a minimal PATH, so a shell script
must call programs by absolute path too.

  tiny-engineer rules list
  tiny-engineer rules add <type> -- <program> [args...]
  tiny-engineer rules remove <index>    1-based index from list

add and remove reload a running daemon. They do not register daemon install.
HTTP anim and hooks stay direct and do not go through the daemon.
`;
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 * @param {{ configDir?: string, env?: NodeJS.ProcessEnv, signal?: (configDir: string) => void | Promise<void> }} [opts]
 */
export async function runRules(out, argv, opts = {}) {
  const configDir = opts.configDir ?? userConfigDir();
  const env = opts.env ?? process.env;
  const signal = opts.signal ?? signalReload;
  if (argv.includes("-h") || argv.includes("--help") || argv.length === 0) {
    console.log(rulesHelp());
    return;
  }
  const [sub, ...rest] = argv;
  if (sub === "list") {
    const loaded = readRules(configDir);
    if (loaded.error) {
      out.error(loaded.error);
      process.exitCode = 1;
      return;
    }
    loaded.rules.forEach((rule, index) => {
      console.log(`${index + 1} ${rule.on} ${rule.run.join(" ")}`);
    });
    return;
  }
  if (sub === "add") {
    const split = rest.indexOf("--");
    if (split <= 0 || split === rest.length - 1) {
      out.error("usage: tiny-engineer rules add <type> -- <program> [args...]");
      process.exitCode = 1;
      return;
    }
    const type = rest[0];
    const program = resolveProgram(rest[split + 1], env);
    if (!program) {
      out.error(`program not found: ${rest[split + 1]}`);
      process.exitCode = 1;
      return;
    }
    const loaded = readRules(configDir);
    if (loaded.error) {
      out.error(loaded.error);
      process.exitCode = 1;
      return;
    }
    loaded.rules.push({ on: type, run: [program, ...rest.slice(split + 2)] });
    writeRules(daemonPaths(configDir).rules, loaded.rules);
    await signal(configDir);
    return;
  }
  if (sub === "remove") {
    const index = Number(rest[0]);
    const loaded = readRules(configDir);
    if (loaded.error) {
      out.error(loaded.error);
      process.exitCode = 1;
      return;
    }
    if (!Number.isInteger(index) || index < 1 || index > loaded.rules.length) {
      out.error(`rules remove: no rule at index ${rest[0] ?? ""}`);
      process.exitCode = 1;
      return;
    }
    loaded.rules.splice(index - 1, 1);
    writeRules(daemonPaths(configDir).rules, loaded.rules);
    await signal(configDir);
    return;
  }
  out.error(`unknown rules command ${sub ?? ""}`);
  console.log(rulesHelp());
  process.exitCode = 1;
}
