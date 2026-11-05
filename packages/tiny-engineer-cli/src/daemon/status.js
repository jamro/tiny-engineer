import { readFileSync } from "node:fs";
import { SerialPort } from "serialport";

import { userConfigDir } from "../config/paths.js";
import { tryConnect } from "./connect.js";
import { supervisorInstalled } from "./flags.js";
import { daemonPaths } from "./paths.js";

/**
 * @param {string} configDir
 */
function installStatusLine(configDir) {
  return `install: ${supervisorInstalled(configDir) ? "on" : "off"}`;
}

/**
 * @param {string} configDir
 * @param {number} [now]
 */
function yieldStatusLine(configDir, now = Date.now()) {
  try {
    const until = JSON.parse(readFileSync(daemonPaths(configDir).yieldFile, "utf8")).until;
    if (typeof until !== "number" || until <= now) return "yield: off";
    const left = Math.ceil((until - now) / 1000);
    return `yield: ${left}s`;
  } catch {
    return "yield: off";
  }
}

/**
 * macOS lists `tty.*` while the daemon may hold the matching `cu.*` node.
 * @param {string} path
 */
function serialPortKey(path) {
  return path.replace(/^\/dev\/(?:tty|cu)\./, "");
}

/**
 * @param {string | null} openPort
 * @param {Array<{ path?: string }> | null} listed null when enumeration failed
 * @returns {string[]}
 */
function portStatusLines(openPort, listed) {
  const open = openPort && openPort !== "down" ? openPort : null;
  /** @type {string[]} */
  const lines = [`port: ${open ?? "down"}`];
  if (!listed) {
    lines.push("ports: unavailable");
    return lines;
  }
  const paths = listed.map((entry) => entry.path).filter((path) => typeof path === "string" && path);
  if (paths.length === 0) {
    lines.push("ports: none");
    return lines;
  }
  const openKey = open ? serialPortKey(open) : null;
  lines.push("ports:");
  for (const path of paths) {
    const mark = openKey && serialPortKey(path) === openKey ? " connected" : "";
    lines.push(`  ${path}${mark}`);
  }
  return lines;
}

/**
 * @param {unknown} value
 */
function formatLastAction(value) {
  if (!value || typeof value !== "object") return "none";
  const action = /** @type {Record<string, unknown>} */ (value);
  if (action.kind === "none" || typeof action.kind !== "string") return "none";
  const type = typeof action.type === "string" ? action.type : "";
  const program = typeof action.program === "string" ? action.program : "";
  if (action.kind === "exit") return `exit ${type} ${program} ${action.code}`;
  return `${action.kind} ${type} ${program}`;
}

/**
 * @param {Record<string, unknown>} body
 * @returns {string[]}
 */
function reactionStatusLines(body) {
  const link = typeof body.reactionLink === "string" ? body.reactionLink : "off";
  const detail = typeof body.reactionDetail === "string" && body.reactionDetail
    ? ` ${body.reactionDetail}`
    : "";
  const count = typeof body.rulesCount === "number" ? body.rulesCount : 0;
  const file = body.rulesError ? " file error" : " ok";
  return [
    `reaction: ${link}${detail}`,
    `rules: ${count}${file}`,
    `last: ${formatLastAction(body.lastAction)}`,
  ];
}

/**
 * @param {{
 *   configDir?: string,
 *   now?: number,
 *   listPorts?: () => Promise<Array<{ path?: string }>>,
 * }} [opts]
 */
export async function daemonStatusText(opts = {}) {
  const configDir = opts.configDir ?? userConfigDir();
  const install = installStatusLine(configDir);
  const yieldLine = yieldStatusLine(configDir, opts.now ?? Date.now());
  const listPorts = opts.listPorts ?? (() => SerialPort.list());
  /** @type {ReturnType<typeof setTimeout> | undefined} */
  let timer;
  /** @type {Array<{ path?: string }> | null} */
  const listed = await Promise.race([
    (async () => listPorts())().catch(() => null),
    new Promise((resolve) => {
      timer = setTimeout(() => resolve(null), 1500);
    }),
  ]).finally(() => clearTimeout(timer));
  const conn = await tryConnect(daemonPaths(configDir).sock, 300);
  if (!conn) {
    return [`daemon is not running`, install, yieldLine, ...portStatusLines(null, listed), ""].join("\n");
  }
  try {
    const res = await conn.peer.request("daemon.status", {}, 1000);
    const body = /** @type {Record<string, unknown>} */ (res.result ?? {});
    const port = body.port ? String(body.port) : null;
    return [
      "daemon running",
      install,
      yieldLine,
      `pid: ${body.pid}`,
      `version: ${body.version}`,
      `channel: ${body.channel}`,
      `packageRoot: ${body.packageRoot}`,
      `proto: ${body.proto}`,
      ...portStatusLines(port, listed),
      ...reactionStatusLines(body),
      "",
    ].join("\n");
  } finally {
    conn.socket.end();
  }
}
