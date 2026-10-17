import { resolveTransport } from "../config/env.js";
import {
  robotFactoryReset,
  robotHardwareTest,
  robotServo,
  robotSettingsGet,
  robotSettingsSet,
} from "../robot/ops.js";
import { formatRobotError, robotHealth } from "../robot/transport.js";
import { parseCommonArgs } from "../util/args.js";
import { confirm } from "../wizard/prompt.js";

/**
 * @param {string[]} argv
 * @returns {ReturnType<typeof parseCommonArgs> & { yes?: boolean, sub?: string, restPos?: string[] }}
 */
function parseParityArgs(argv) {
  let yes = false;
  /** @type {string[]} */
  const filtered = [];
  for (const arg of argv) {
    if (arg === "--yes" || arg === "-y") {
      yes = true;
      continue;
    }
    filtered.push(arg);
  }
  const common = parseCommonArgs(filtered);
  return { ...common, yes, restPos: common.rest };
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 * @param {boolean} quietGlobal
 */
export async function runSettings(out, argv, quietGlobal) {
  const opts = parseParityArgs(argv);
  const quiet = quietGlobal || opts.quiet;
  const localOut = quiet === out.quiet ? out : out;

  if (opts.error) {
    localOut.error(opts.error);
    process.exitCode = 1;
    return;
  }
  if (opts.help) {
    console.log(`Usage: tiny-engineer settings [get|set] [options]

  settings get
  settings set --key=value ...   (volume, welcome, serial_log, link_mode, eyes_style, hostname)

Options:
  --transport <http|serial>  --url <base>  --port <path>
`);
    return;
  }

  const [sub, ...rest] = opts.restPos ?? [];
  const transportOpts = { url: opts.url, transport: opts.transport, port: opts.port };

  if (!sub || sub === "get") {
    const result = await robotSettingsGet(transportOpts);
    if (!result.ok) {
      localOut.error(formatRobotError(result));
      process.exitCode = 1;
      return;
    }
    console.log(JSON.stringify(result.bodyJson ?? {}, null, 2));
    process.exitCode = 0;
    return;
  }

  if (sub === "set") {
    /** @type {Record<string, unknown>} */
    const patch = {};
    for (const token of rest) {
      if (!token.startsWith("--") || !token.includes("=")) {
        localOut.error(`settings set expects --key=value (got ${token})`);
        process.exitCode = 1;
        return;
      }
      const eq = token.indexOf("=");
      const key = token.slice(2, eq);
      let value = token.slice(eq + 1);
      if (value === "true" || value === "false") {
        patch[key] = value === "true";
      } else if (/^\d+$/.test(value)) {
        patch[key] = Number(value);
      } else {
        patch[key] = value;
      }
    }
    if (Object.keys(patch).length === 0) {
      localOut.error("settings set needs at least one --key=value");
      process.exitCode = 1;
      return;
    }
    const result = await robotSettingsSet(patch, transportOpts);
    if (!result.ok) {
      localOut.error(formatRobotError(result));
      process.exitCode = 1;
      return;
    }
    localOut.ok("settings updated");
    process.exitCode = 0;
    return;
  }

  localOut.error(`unknown settings subcommand: ${sub}`);
  process.exitCode = 1;
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 * @param {boolean} quietGlobal
 */
export async function runTest(out, argv, quietGlobal) {
  const opts = parseParityArgs(argv);
  if (opts.error) {
    out.error(opts.error);
    process.exitCode = 1;
    return;
  }
  if (opts.help) {
    console.log(`Usage: tiny-engineer test <audio|bell|screen|movement|led> [--transport] [--url] [--port]
`);
    return;
  }

  const name = opts.restPos?.[0];
  const allowed = new Set(["audio", "bell", "screen", "movement", "led"]);
  if (!name || !allowed.has(name)) {
    out.error("test needs one of: audio, bell, screen, movement, led");
    process.exitCode = 1;
    return;
  }

  const result = await robotHardwareTest(name, {
    url: opts.url,
    transport: opts.transport,
    port: opts.port,
  });
  if (!result.ok) {
    out.error(formatRobotError(result));
    process.exitCode = 1;
    return;
  }
  out.ok(`test ${name} ok`);
  process.exitCode = 0;
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 * @param {boolean} quietGlobal
 */
export async function runServo(out, argv, quietGlobal) {
  const opts = parseParityArgs(argv);
  if (opts.error) {
    out.error(opts.error);
    process.exitCode = 1;
    return;
  }
  if (opts.help) {
    console.log(`Usage: tiny-engineer servo <index> <angle> [--transport] [--url] [--port]
`);
    return;
  }

  const index = Number(opts.restPos?.[0]);
  const angle = Number(opts.restPos?.[1]);
  if (!Number.isInteger(index) || Number.isNaN(angle)) {
    out.error("servo needs <index> <angle>");
    process.exitCode = 1;
    return;
  }

  const result = await robotServo(index, angle, {
    url: opts.url,
    transport: opts.transport,
    port: opts.port,
  });
  if (!result.ok) {
    out.error(formatRobotError(result));
    process.exitCode = 1;
    return;
  }
  out.ok(`servo ${index} → ${angle}°`);
  process.exitCode = 0;
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 * @param {boolean} quietGlobal
 */
export async function runHealth(out, argv, quietGlobal) {
  const opts = parseParityArgs(argv);
  if (opts.error) {
    out.error(opts.error);
    process.exitCode = 1;
    return;
  }
  if (opts.help) {
    console.log(`Usage: tiny-engineer health [--transport] [--url] [--port]
`);
    return;
  }

  const health = await robotHealth({
    url: opts.url,
    transport: opts.transport,
    port: opts.port,
  });
  if (!health.ok) {
    out.error(health.detail);
    process.exitCode = 1;
    return;
  }
  console.log(health.detail);
  process.exitCode = 0;
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 * @param {boolean} quietGlobal
 */
export async function runReset(out, argv, quietGlobal) {
  const opts = parseParityArgs(argv);
  if (opts.error) {
    out.error(opts.error);
    process.exitCode = 1;
    return;
  }
  if (opts.help) {
    console.log(`Usage: tiny-engineer reset [--yes] [--transport] [--url] [--port]

Factory-reset device settings (keeps servo calibration / RGB / OLED rotation).
Requires interactive confirmation or --yes.
`);
    return;
  }

  const resolved = resolveTransport(opts.transport);
  if (resolved.error) {
    out.error(resolved.error);
    process.exitCode = 1;
    return;
  }

  if (!opts.yes) {
    const ok = await confirm("Factory-reset the robot? This clears Wi-Fi and link mode.", false);
    if (!ok) {
      out.info("reset cancelled");
      process.exitCode = 0;
      return;
    }
  }

  const result = await robotFactoryReset({
    url: opts.url,
    transport: opts.transport,
    port: opts.port,
  });
  if (!result.ok) {
    out.error(formatRobotError(result));
    process.exitCode = 1;
    return;
  }
  out.ok("factory reset complete — power-cycle if the robot stays offline");
  process.exitCode = 0;
}
