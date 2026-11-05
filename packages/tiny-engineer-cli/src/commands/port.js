import { SerialPort } from "serialport";

import { getSerialPort } from "../config/env.js";
import { serialProbe } from "../daemon/robot.js";
import { scoreSerialPort } from "../robot/serial/client.js";
import { readPortCache } from "../robot/serial/port-cache.js";

/**
 * @param {string[]} argv
 * @returns {{ help: boolean, probe: boolean, error?: string }}
 */
export function parsePortArgs(argv) {
  let help = false;
  let probe = false;
  for (const arg of argv) {
    if (arg === "-h" || arg === "--help") {
      help = true;
      continue;
    }
    if (arg === "--probe") {
      probe = true;
      continue;
    }
    return { help, probe, error: `unknown port argument ${arg}` };
  }
  return { help, probe };
}

export function portHelp() {
  return `Usage: tiny-engineer port [--probe]

List host serial ports and the auto-detect choice.
Score uses the same rules as serial auto-detect: Espressif VID 303a,
a tiny-engineer name, a CDC/JTAG name, then a serial-looking path.
env (TINY_ENGINEER_PORT) wins over the cache, then the highest score.

  --probe   Sync candidates through the host daemon. Does not write the port
            cache. May leave the daemon holding a winning port.
`;
}

/**
 * @param {{
 *   ports: Array<{ path?: string, manufacturer?: string, friendlyName?: string, vendorId?: string }>,
 *   envPort?: string | null,
 *   cachedPort?: string | null,
 * }} opts
 * @returns {string}
 */
export function formatPortScan(opts) {
  const scored = opts.ports
    .map((port) => scoreSerialPort(port))
    .filter((row) => row !== null)
    .sort((a, b) => b.score - a.score);
  const winner = scored.find((row) => row.score > 0) ?? null;
  const envPort = opts.envPort ?? null;
  const cachedPort = opts.cachedPort ?? null;
  /** @type {string[]} */
  const lines = [
    `env: ${envPort ?? "(unset)"}`,
    `cache: ${cachedPort ?? "(none)"}`,
  ];
  if (envPort) lines.push(`open: ${envPort} (env)`);
  else if (cachedPort) lines.push(`open: ${cachedPort} (cache)`);
  else if (winner) lines.push(`open: ${winner.path} (autodetect)`);
  else lines.push("open: (none)");
  lines.push(`autodetect: ${winner ? `${winner.path} score ${winner.score}` : "(none)"}`);
  lines.push("ports:");
  if (scored.length === 0) lines.push("  (none)");
  for (const row of scored) {
    const why = row.reasons.length ? row.reasons.join(", ") : "skipped";
    lines.push(`  ${row.path}  score ${row.score}  ${why}`);
  }
  return `${lines.join("\n")}\n`;
}

/**
 * Same candidate order as historical in-process probe: env alone when set;
 * otherwise cache then other positive-score paths.
 * @param {{
 *   ports: Array<{ path?: string, manufacturer?: string, friendlyName?: string, vendorId?: string }>,
 *   envPort?: string | null,
 *   cachedPort?: string | null,
 * }} opts
 * @returns {{ path: string, source: string }[]}
 */
export function probeCandidates(opts) {
  const scored = opts.ports
    .map((port) => scoreSerialPort(port))
    .filter((row) => row !== null && row.score > 0)
    .sort((a, b) => b.score - a.score);
  const envPort = opts.envPort ?? null;
  const cachedPort = opts.cachedPort ?? null;
  /** @type {{ path: string, source: string }[]} */
  const order = [];
  if (envPort) {
    order.push({ path: envPort, source: "env" });
    return order;
  }
  if (cachedPort) order.push({ path: cachedPort, source: "cache" });
  for (const row of scored) {
    if (row.path !== cachedPort) order.push({ path: row.path, source: "autodetect" });
  }
  return order;
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 * @param {{
 *   list?: () => Promise<Array<{ path?: string, manufacturer?: string, friendlyName?: string, vendorId?: string }>>,
 *   envPort?: string | null,
 *   cachedPort?: string | null,
 *   probe?: (candidates: string[]) => Promise<{
 *     ok: boolean,
 *     results: Array<{ path: string, ok: boolean, detail: string, skipped?: boolean }>,
 *     port?: string | null,
 *   }>,
 *   write?: (text: string) => void,
 * }} [deps]
 */
export async function runPort(out, argv, deps = {}) {
  const opts = parsePortArgs(argv);
  if (opts.error) {
    out.error(opts.error);
    process.exitCode = 1;
    return;
  }
  if (opts.help) {
    console.log(portHelp());
    return;
  }
  const write = deps.write ?? ((text) => process.stdout.write(text));
  let listed;
  try {
    listed = await (deps.list ?? (() => SerialPort.list()))();
  } catch (err) {
    out.error(err instanceof Error ? err.message : String(err));
    process.exitCode = 1;
    return;
  }
  const envPort = deps.envPort === undefined ? getSerialPort() : deps.envPort;
  const cachedPort = deps.cachedPort === undefined ? readPortCache() : deps.cachedPort;
  write(formatPortScan({ ports: listed, envPort, cachedPort }));
  if (!opts.probe) return;

  const order = probeCandidates({ ports: listed, envPort, cachedPort });
  if (order.length === 0) {
    write("probe: no candidate\n");
    process.exitCode = 1;
    return;
  }

  const sourceByPath = new Map(order.map((item) => [item.path, item.source]));
  const probeFn =
    deps.probe ??
    ((candidates) => serialProbe({ candidates }));

  let body;
  try {
    body = await probeFn(order.map((item) => item.path));
  } catch (err) {
    out.error(err instanceof Error ? err.message : String(err));
    process.exitCode = 1;
    return;
  }

  for (const row of body.results ?? []) {
    const source = sourceByPath.get(row.path) ?? "autodetect";
    write(`probe ${row.path} (${source}) ${row.detail}\n`);
  }
  if (!body.ok) process.exitCode = 1;
}
