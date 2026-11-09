import { spawnSync } from "node:child_process";
import { existsSync, mkdirSync, rmSync, writeFileSync } from "node:fs";
import { homedir } from "node:os";
import { dirname, join } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

import { shutdownDaemon, startWrapperProcess, tryConnect } from "./client.js";
import { daemonPaths } from "./paths.js";
import { delay } from "./proc.js";
import { writeDaemonTarget } from "./target.js";
import { wrapperAlive } from "./wrapper.js";

export const SUPERVISOR_LABEL = "com.tinyengineer.daemon";

/**
 * Element text. Ampersand first so later entities are not escaped again.
 * @param {string} value
 */
function xmlText(value) {
  return value.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
}

/**
 * One systemd word. Percent, then backslash, then quote.
 * @param {string} value
 */
function systemdWord(value) {
  const escaped = value.replace(/%/g, "%%").replace(/\\/g, "\\\\").replace(/"/g, '\\"');
  return `"${escaped}"`;
}

/**
 * @param {{ nodePath: string, wrapperPath: string, configDir: string }} opts
 */
export function launchdPlist(opts) {
  // KeepAlive SuccessfulExit=false: restart after crash (non-zero), not after
  // clean disable/uninstall exit 0.
  return `<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>Label</key>
  <string>${SUPERVISOR_LABEL}</string>
  <key>RunAtLoad</key>
  <true/>
  <key>KeepAlive</key>
  <dict>
    <key>SuccessfulExit</key>
    <false/>
  </dict>
  <key>ProgramArguments</key>
  <array>
    <string>${xmlText(opts.nodePath)}</string>
    <string>${xmlText(opts.wrapperPath)}</string>
  </array>
  <key>EnvironmentVariables</key>
  <dict>
    <key>TINY_ENGINEER_CONFIG_DIR</key>
    <string>${xmlText(opts.configDir)}</string>
  </dict>
</dict>
</plist>
`;
}

/**
 * @param {{ nodePath: string, wrapperPath: string, configDir: string }} opts
 */
export function systemdUnit(opts) {
  // on-failure: crash restart only. Clean disable/uninstall exit 0 stays down.
  return `[Unit]
Description=Tiny Engineer host daemon

[Service]
Type=simple
ExecStart=${systemdWord(opts.nodePath)} ${systemdWord(opts.wrapperPath)}
Environment=${systemdWord(`TINY_ENGINEER_CONFIG_DIR=${opts.configDir}`)}
Restart=on-failure
RestartSec=2

[Install]
WantedBy=default.target
`;
}

/**
 * Logon task. No failure restart.
 * @param {{ nodePath: string, wrapperPath: string, configDir: string }} opts
 */
export function windowsTaskArgs(opts) {
  return [
    "/Create",
    "/F",
    "/SC",
    "ONLOGON",
    "/TN",
    SUPERVISOR_LABEL,
    "/TR",
    `"${opts.nodePath}" "${opts.wrapperPath}"`,
  ];
}

/**
 * @param {NodeJS.Platform} platform
 * @param {{ nodePath: string, wrapperPath: string, configDir: string }} opts
 */
export function unitSpec(platform, opts) {
  if (platform === "win32") return { kind: "schtasks", args: windowsTaskArgs(opts) };
  if (platform === "linux") return { kind: "systemd", body: systemdUnit(opts) };
  return { kind: "plist", body: launchdPlist(opts) };
}

/**
 * @param {string} moduleUrl
 * @param {string} configDir
 */
export function wrapperSource(moduleUrl, configDir) {
  return `import { runWrapper } from ${JSON.stringify(moduleUrl)};
const code = await runWrapper({ configDir: ${JSON.stringify(configDir)} });
process.exit(code ?? 0);
`;
}

/**
 * @param {{
 *   configDir: string,
 *   nodePath?: string,
 *   scriptPath: string,
 *   platform?: NodeJS.Platform,
 *   registrar: {
 *     install: (unit: ReturnType<typeof unitSpec>) => Promise<boolean | void> | boolean | void,
 *     uninstall?: () => Promise<void> | void,
 *   },
 *   wrapperModuleUrl?: string,
 *   startWrapper?: (configDir: string) => void | Promise<void>,
 * }} opts
 */
export async function installSupervisor(opts) {
  const configDir = opts.configDir;
  const paths = daemonPaths(configDir);
  const live = await tryConnect(paths.sock, 200);
  if (live) {
    live.socket.destroy();
    await shutdownDaemon(configDir, "replace");
    const still = await tryConnect(paths.sock, 200);
    if (still) {
      still.socket.destroy();
      throw new Error("daemon socket still open");
    }
  }
  mkdirSync(paths.dir, { recursive: true, mode: 0o700 });
  const hadSupervisor = existsSync(paths.supervisor);
  const nodePath = opts.nodePath ?? process.execPath;
  writeDaemonTarget(configDir, { execPath: nodePath, script: opts.scriptPath });
  const moduleUrl = opts.wrapperModuleUrl
    ?? pathToFileURL(fileURLToPath(new URL("./wrapper.js", import.meta.url))).href;
  writeFileSync(paths.wrapper, wrapperSource(moduleUrl, configDir), { mode: 0o700 });
  writeFileSync(paths.supervisor, "{}\n", { mode: 0o600 });
  const unit = unitSpec(opts.platform ?? process.platform, {
    nodePath,
    wrapperPath: paths.wrapper,
    configDir,
  });
  let startedByUnit;
  try {
    startedByUnit = await opts.registrar.install(unit);
  } catch (err) {
    if (!hadSupervisor) {
      rmSync(paths.supervisor, { force: true });
      rmSync(paths.target, { force: true });
      rmSync(paths.wrapper, { force: true });
    }
    throw err;
  }
  if (!(await wrapperAlive(configDir)) && startedByUnit !== true) {
    await (opts.startWrapper ?? startWrapperProcess)(configDir);
  }
}

/**
 * @param {{
 *   configDir: string,
 *   registrar: { uninstall: () => Promise<void> | void },
 *   shutdown?: (configDir: string) => Promise<void>,
 *   wrapperAlive?: (configDir: string) => boolean | Promise<boolean>,
 *   sleep?: (ms: number) => Promise<void>,
 *   now?: () => number,
 * }} opts
 */
export async function uninstallSupervisor(opts) {
  const paths = daemonPaths(opts.configDir);
  const installed = existsSync(paths.supervisor);
  await opts.registrar.uninstall();
  rmSync(paths.supervisor, { force: true });
  if (!installed) return;
  const stop = opts.shutdown ?? ((dir) => shutdownDaemon(dir, "stop"));
  const alive = opts.wrapperAlive ?? wrapperAlive;
  const sleep = opts.sleep ?? delay;
  const now = opts.now ?? Date.now;
  await stop(opts.configDir);
  // waitForHello can block ~2s before the wrapper sees the missing file.
  const deadline = now() + 3000;
  while (await alive(opts.configDir) && now() < deadline) {
    await sleep(30);
    await stop(opts.configDir);
  }
}

/**
 * Default plist path for this user. Not a system LaunchDaemon.
 * @param {string} [home]
 */
export function launchAgentPath(home = homedir()) {
  return join(home, "Library", "LaunchAgents", `${SUPERVISOR_LABEL}.plist`);
}

/**
 * Writes the user login unit and enables it. Not a system-boot service.
 * @param {NodeJS.Platform} [platform]
 */
export function osRegistrar(platform = process.platform) {
  const uid = typeof process.getuid === "function" ? process.getuid() : 0;
  return {
    /**
     * @param {ReturnType<typeof unitSpec>} unit
     * @returns {boolean} true when the unit starts the wrapper now
     */
    install(unit) {
      if (platform === "linux" && unit.kind === "systemd") {
        const dir = join(homedir(), ".config", "systemd", "user");
        mkdirSync(dir, { recursive: true });
        const path = join(dir, `${SUPERVISOR_LABEL}.service`);
        writeFileSync(path, unit.body);
        const reload = spawnSync("systemctl", ["--user", "daemon-reload"], { stdio: "ignore" });
        if (reload.error || reload.status) throw new Error("systemctl --user daemon-reload failed");
        const enable = spawnSync("systemctl", ["--user", "enable", "--now", SUPERVISOR_LABEL], { stdio: "ignore" });
        if (enable.error || enable.status) throw new Error("systemctl --user enable failed");
        return true;
      }
      if (platform === "win32" && unit.kind === "schtasks") {
        const created = spawnSync("schtasks", unit.args, { stdio: "ignore" });
        if (created.error || created.status) throw new Error("schtasks failed");
        return false;
      }
      if (unit.kind !== "plist") return false;
      const path = launchAgentPath();
      mkdirSync(dirname(path), { recursive: true });
      writeFileSync(path, unit.body);
      spawnSync("launchctl", ["bootout", `gui/${uid}`, path], { stdio: "ignore" });
      const boot = spawnSync("launchctl", ["bootstrap", `gui/${uid}`, path], { stdio: "ignore" });
      if (boot.error || boot.status) throw new Error("launchctl bootstrap failed");
      return true;
    },
    uninstall() {
      if (platform === "linux") {
        spawnSync("systemctl", ["--user", "disable", "--now", SUPERVISOR_LABEL], { stdio: "ignore" });
        rmSync(join(homedir(), ".config", "systemd", "user", `${SUPERVISOR_LABEL}.service`), { force: true });
        return;
      }
      if (platform === "win32") {
        spawnSync("schtasks", ["/Delete", "/F", "/TN", SUPERVISOR_LABEL], { stdio: "ignore" });
        return;
      }
      const path = launchAgentPath();
      spawnSync("launchctl", ["bootout", `gui/${uid}`, path], { stdio: "ignore" });
      rmSync(path, { force: true });
    },
  };
}
