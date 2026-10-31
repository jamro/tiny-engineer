import { userConfigDir } from "../config/paths.js";
import { clearYield, daemonStatusText, ensureDaemon, stopDaemon } from "./client.js";
import { serveDaemon } from "./server.js";

/**
 * @param {string[]} argv
 * @returns {{ help: boolean, yield: boolean, port?: string, serve: boolean, command: string | null, error?: string }}
 */
export function parseDaemonArgs(argv) {
  let help = false;
  let yieldStop = false;
  let serve = false;
  /** @type {string | undefined} */
  let port;
  /** @type {string | null} */
  let command = null;
  for (let i = 0; i < argv.length; i++) {
    const arg = argv[i];
    if (arg === "-h" || arg === "--help") {
      help = true;
      continue;
    }
    if (arg === "--yield") {
      yieldStop = true;
      continue;
    }
    if (arg === "--serve") {
      serve = true;
      continue;
    }
    if (arg === "--port") {
      const next = argv[++i];
      if (!next) return { help, yield: yieldStop, serve, port, command, error: "--port requires a value" };
      port = next;
      continue;
    }
    if (!command && !arg.startsWith("-")) {
      command = arg;
      continue;
    }
    return { help, yield: yieldStop, serve, port, command, error: `unknown daemon argument ${arg}` };
  }
  return { help, yield: yieldStop, serve, port, command };
}

export function daemonHelp() {
  return `Usage: tiny-engineer daemon <start|stop|status|restart> [options]

Host session for this machine. Serial commands auto-start it.
The process keeps the USB serial port until stop.

  start [--port <path>]   Start unless already running. Clears an upload yield.
  stop [--yield]          Stop. --yield refuses auto-start for 180s (firmware upload).
  status                  Print pid, version, channel, package root, proto, and port.
  restart [--port <path>] Stop, then start. --port is the port the new process opens.
`;
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 */
export async function runDaemon(out, argv) {
  const opts = parseDaemonArgs(argv);
  if (opts.error) {
    out.error(opts.error);
    process.exitCode = 1;
    return;
  }
  if (opts.help || (!opts.command && !opts.serve)) {
    console.log(daemonHelp());
    return;
  }
  if (opts.serve) {
    await serveDaemon({
      configDir: userConfigDir(),
      preferredPort: opts.port ?? null,
      exitProcess: true,
    });
    return;
  }

  const configDir = userConfigDir();
  if (opts.command === "start") {
    clearYield(configDir);
    try {
      const conn = await ensureDaemon({ configDir, port: opts.port ?? null, forceStart: true });
      conn.socket.end();
    } catch (err) {
      out.error(err instanceof Error ? err.message : String(err));
      process.exitCode = 1;
    }
    return;
  }
  if (opts.command === "stop") {
    await stopDaemon({ configDir, yield: opts.yield });
    return;
  }
  if (opts.command === "status") {
    process.stdout.write(await daemonStatusText({ configDir }));
    return;
  }
  if (opts.command === "restart") {
    await stopDaemon({ configDir, yield: false });
    clearYield(configDir);
    try {
      const conn = await ensureDaemon({ configDir, port: opts.port ?? null, forceStart: true });
      conn.socket.end();
    } catch (err) {
      out.error(err instanceof Error ? err.message : String(err));
      process.exitCode = 1;
    }
    return;
  }
  out.error(`unknown daemon command ${opts.command}`);
  console.log(daemonHelp());
  process.exitCode = 1;
}
