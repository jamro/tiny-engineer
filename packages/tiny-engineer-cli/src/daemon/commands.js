import { userConfigDir } from "../config/paths.js";
import { installSupervisor, osRegistrar, uninstallSupervisor } from "./install.js";
import { clearDisable, clearYield, daemonStatusText, disableDaemon, ensureDaemon, releaseDaemon, shutdownDaemon, supervisorInstalled, tryConnect } from "./client.js";
import { clearReactionsSuspended } from "./reactions.js";
import { daemonPaths } from "./paths.js";
import { resolveCliScript } from "./policy.js";
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
  return `Usage: tiny-engineer daemon <start|stop|status|restart|release|disable|install|uninstall> [options]

Host session for this machine. Serial commands auto-start it unless disabled.
Plain stop matches release: the process stays up and the USB port closes.
The port stays closed until the next serial command.

  start [--port <path>]   Start unless already running. Clears disable and yield.
                          Opens the USB port when serial reactions are armed
                          (a rule in rules.json and user transport serial).
                          Otherwise the port stays closed.
                          --port is ignored while daemon install is on.
  stop [--yield]          Same as release. --yield refuses serial open for 180s
                          and leaves the process running (firmware upload).
                          Suspends serial reaction auto-open until start,
                          restart, or a serial command. HTTP reactions stay up.
  release                 Close the USB port. Process stays up. No yield.
                          Suspends serial reaction auto-open the same way as stop.
  disable                 Exit and refuse auto-start until start.
  status                  Print pid, version, channel, package root, proto, and port.
  restart [--port <path>] Clear disable and yield, then start a fresh process.
                          Opens the USB port when serial reactions are armed.
                          --port is the port the new process opens next.
                          --port is ignored while daemon install is on.
  install                 Start the login wrapper for this user. Not a boot service.
  uninstall               Remove the login wrapper. Does not disable auto-start.
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
    clearDisable(configDir);
    clearYield(configDir);
    clearReactionsSuspended(configDir);
    try {
      warnIgnoredPort(out, configDir, opts.port);
      const conn = await ensureDaemon({ configDir, port: opts.port ?? null, forceStart: true });
      await conn.peer.request("reactions.sync", {}, 2000);
      conn.socket.end();
    } catch (err) {
      out.error(err instanceof Error ? err.message : String(err));
      process.exitCode = 1;
    }
    return;
  }
  if (opts.command === "release" || (opts.command === "stop" && !opts.yield)) {
    try {
      await releaseDaemon({ configDir });
    } catch (err) {
      out.error(err instanceof Error ? err.message : String(err));
      process.exitCode = 1;
    }
    return;
  }
  if (opts.command === "stop") {
    try {
      await releaseDaemon({ configDir, yield: true });
    } catch (err) {
      out.error(err instanceof Error ? err.message : String(err));
      process.exitCode = 1;
    }
    return;
  }
  if (opts.command === "disable") {
    try {
      await disableDaemon({ configDir });
    } catch (err) {
      out.error(err instanceof Error ? err.message : String(err));
      process.exitCode = 1;
    }
    return;
  }
  if (opts.command === "install") {
    try {
      let playActive = false;
      const conn = await tryConnect(daemonPaths(configDir).sock, 200);
      if (conn) {
        try {
          const res = await conn.peer.request("daemon.status", {}, 1000);
          playActive = Boolean(/** @type {{ playActive?: boolean }} */ (res.result)?.playActive);
        } finally {
          conn.socket.destroy();
        }
      }
      await installSupervisor({
        configDir,
        scriptPath: resolveCliScript(),
        playActive,
        registrar: osRegistrar(),
      });
    } catch (err) {
      out.error(err instanceof Error ? err.message : String(err));
      process.exitCode = 1;
    }
    return;
  }
  if (opts.command === "uninstall") {
    try {
      await uninstallSupervisor({ configDir, registrar: osRegistrar() });
    } catch (err) {
      out.error(err instanceof Error ? err.message : String(err));
      process.exitCode = 1;
    }
    return;
  }
  if (opts.command === "status") {
    process.stdout.write(await daemonStatusText({ configDir }));
    return;
  }
  if (opts.command === "restart") {
    clearDisable(configDir);
    clearYield(configDir);
    clearReactionsSuspended(configDir);
    await shutdownDaemon(configDir, "stop");
    try {
      warnIgnoredPort(out, configDir, opts.port);
      const conn = await ensureDaemon({ configDir, port: opts.port ?? null, forceStart: true });
      await conn.peer.request("reactions.sync", {}, 2000);
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

/**
 * The login wrapper starts `daemon --serve` with no port.
 * @param {import("../util/output.js").Output} out
 * @param {string} configDir
 * @param {string | undefined} port
 */
function warnIgnoredPort(out, configDir, port) {
  if (port && supervisorInstalled(configDir)) {
    out.info("warning: --port is ignored while daemon install is on");
  }
}
