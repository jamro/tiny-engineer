import { existsSync } from "node:fs";
import { join } from "node:path";

import { DEFAULT_URL, getBaseUrl, getSerialPath } from "../config/env.js";
import { all as allIntegrations, get as getIntegration } from "../integrations/registry.js";
import { takeSerialFlag } from "../util/args.js";
import { hookCommandFor } from "./cliPath.js";
import {
  mergeEnvSerial,
  mergeEnvUrl,
  readJsonFile,
  stripEnvKey,
  writeJsonFile,
} from "./merge.js";
import { ask, choose, confirm, multiSelect } from "./prompt.js";

/** Cold `npx` can exceed a few seconds; keep hook timeout comfortable. */
const DEFAULT_TIMEOUT = 30;

/**
 * @param {string[]} argv
 * @returns {{
 *   help: boolean,
 *   yes: boolean,
 *   all: boolean,
 *   dryRun: boolean,
 *   forceUrl: boolean,
 *   forceSerial: boolean,
 *   url?: string,
 *   serial?: string,
 *   command?: string,
 *   ides: string[],
 *   error?: string,
 * }}
 */
export function parseSetupArgs(argv) {
  let help = false;
  let yes = false;
  let all = false;
  let dryRun = false;
  let forceUrl = false;
  let forceSerial = false;
  /** @type {string | undefined} */
  let url;
  /** @type {string | undefined} */
  let serial;
  /** @type {string | undefined} */
  let command;
  /** @type {string[]} */
  const ides = [];

  for (let i = 0; i < argv.length; i++) {
    const arg = argv[i];
    if (arg === "-h" || arg === "--help") {
      help = true;
      continue;
    }
    if (arg === "--yes" || arg === "-y") {
      yes = true;
      continue;
    }
    if (arg === "--all") {
      all = true;
      continue;
    }
    if (arg === "--dry-run") {
      dryRun = true;
      continue;
    }
    if (arg === "--force-url") {
      forceUrl = true;
      continue;
    }
    if (arg === "--force-serial") {
      forceSerial = true;
      continue;
    }
    if (arg === "--url") {
      const next = argv[++i];
      if (!next) {
        return { help, yes, all, dryRun, forceUrl, forceSerial, ides, error: "--url requires a value" };
      }
      url = next;
      continue;
    }
    if (arg.startsWith("--url=")) {
      url = arg.slice("--url=".length);
      if (!url) {
        return { help, yes, all, dryRun, forceUrl, forceSerial, ides, error: "--url requires a value" };
      }
      continue;
    }
    if (arg === "--serial" || arg.startsWith("--serial=")) {
      const taken = takeSerialFlag(argv, i);
      if ("error" in taken) {
        return { help, yes, all, dryRun, forceUrl, forceSerial, ides, error: taken.error };
      }
      serial = taken.serial;
      i = taken.nextIndex;
      continue;
    }
    if (arg === "--command") {
      const next = argv[++i];
      if (!next) {
        return {
          help,
          yes,
          all,
          dryRun,
          forceUrl,
          forceSerial,
          ides,
          error: "--command requires a value",
        };
      }
      command = next;
      continue;
    }
    if (arg.startsWith("--command=")) {
      command = arg.slice("--command=".length);
      if (!command) {
        return {
          help,
          yes,
          all,
          dryRun,
          forceUrl,
          forceSerial,
          ides,
          error: "--command requires a value",
        };
      }
      continue;
    }
    if (arg.startsWith("-")) {
      return {
        help,
        yes,
        all,
        dryRun,
        forceUrl,
        forceSerial,
        ides,
        error: `Unknown argument: ${arg}`,
      };
    }
    ides.push(arg);
  }

  if (url && serial) {
    return {
      help,
      yes,
      all,
      dryRun,
      forceUrl,
      forceSerial,
      url,
      serial,
      command,
      ides,
      error: "pass --url or --serial, not both",
    };
  }

  return { help, yes, all, dryRun, forceUrl, forceSerial, url, serial, command, ides };
}

/**
 * @param {string} projectRoot
 * @returns {{ id: string, name: string, detected: boolean }[]}
 */
export function detectIntegrations(projectRoot) {
  return allIntegrations().map((integration) => ({
    id: integration.id,
    name: integration.name,
    detected: Boolean(integration.detect?.(projectRoot)),
  }));
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string} envPath
 * @param {"http" | "serial"} mode
 * @param {string} value
 * @param {{ forceUrl: boolean, forceSerial: boolean, dryRun?: boolean }} flags
 */
function applyTransportEnv(out, envPath, mode, value, flags) {
  if (flags.dryRun) {
    if (mode === "http") {
      out.info(
        `.env TINY_ENGINEER_URL=${value}${flags.forceUrl ? " (force)" : " (if unset)"}; strip TINY_ENGINEER_SERIAL`,
      );
    } else {
      out.info(
        `.env TINY_ENGINEER_SERIAL=${value}${flags.forceSerial ? " (force)" : " (if unset)"}; strip TINY_ENGINEER_URL`,
      );
    }
    return;
  }

  if (mode === "http") {
    const result = mergeEnvUrl(envPath, value, { force: flags.forceUrl });
    if (result === "skipped") {
      out.info(".env TINY_ENGINEER_URL already set (use --force-url to replace)");
    } else {
      out.ok(`.env TINY_ENGINEER_URL ${result}`);
    }
    const stripped = stripEnvKey(envPath, "TINY_ENGINEER_SERIAL");
    if (stripped === "removed") out.info(".env cleared TINY_ENGINEER_SERIAL");
    return;
  }

  const result = mergeEnvSerial(envPath, value, { force: flags.forceSerial });
  if (result === "skipped") {
    out.info(".env TINY_ENGINEER_SERIAL already set (use --force-serial to replace)");
  } else {
    out.ok(`.env TINY_ENGINEER_SERIAL ${result}`);
  }
  const stripped = stripEnvKey(envPath, "TINY_ENGINEER_URL");
  if (stripped === "removed") out.info(".env cleared TINY_ENGINEER_URL");
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 * @param {{ projectRoot?: string }} [opts]
 */
export async function runSetup(out, argv, { projectRoot = process.cwd() } = {}) {
  const args = parseSetupArgs(argv);
  if (args.error) {
    out.error(args.error);
    process.exitCode = 1;
    return;
  }
  if (args.help) {
    printSetupHelp();
    return;
  }

  if (args.yes && !args.all && args.ides.length === 0) {
    out.error("setup --yes requires --all or at least one ide id (cursor, claude-code, antigravity)");
    process.exitCode = 1;
    return;
  }

  for (const id of args.ides) {
    if (!getIntegration(id)) {
      out.error(`Unknown integration: ${id} (available: ${allIntegrations().map((i) => i.id).join(", ")})`);
      process.exitCode = 1;
      return;
    }
  }

  const detected = detectIntegrations(projectRoot);
  /** @type {string[]} */
  let selected;

  if (args.all) {
    selected = allIntegrations().map((i) => i.id);
  } else if (args.ides.length > 0) {
    selected = args.ides;
  } else if (args.yes) {
    selected = [];
  } else {
    const detectedOpts = detected.filter((d) => d.detected);
    const choices =
      detectedOpts.length > 0
        ? detectedOpts.map((d) => ({ id: d.id, label: d.name }))
        : detected.map((d) => ({ id: d.id, label: d.name }));
    selected = await multiSelect("Install Tiny Engineer hooks for:", choices);
  }

  if (selected.length === 0) {
    out.error("no integrations selected");
    process.exitCode = 1;
    return;
  }

  /** @type {"http" | "serial"} */
  let mode;
  /** @type {string} */
  let transportValue;
  /** @type {string | undefined} */
  let urlForCtx;

  if (args.yes) {
    if (args.serial) {
      mode = "serial";
      transportValue = args.serial;
      urlForCtx = undefined;
    } else {
      mode = "http";
      transportValue = args.url || getBaseUrl();
      urlForCtx = transportValue;
    }
  } else if (args.serial) {
    mode = "serial";
    transportValue = await ask(
      "Serial port (path or auto; list with: tiny-engineer ports)",
      args.serial,
    );
    urlForCtx = undefined;
  } else if (args.url) {
    mode = "http";
    transportValue = await ask("Robot base URL", args.url);
    urlForCtx = transportValue;
  } else {
    const defaultMode = getSerialPath() ? "serial" : "http";
    mode = /** @type {"http" | "serial"} */ (
      await choose(
        "Robot connection:",
        [
          { id: "http", label: "HTTP (Wi-Fi)" },
          { id: "serial", label: "Serial (USB)" },
        ],
        defaultMode,
      )
    );
    if (mode === "serial") {
      out.info("Tip: tiny-engineer ports lists CDC devices; auto probes Espressif USB.");
      transportValue = await ask(
        "Serial port (path or auto)",
        getSerialPath() || "auto",
      );
      urlForCtx = undefined;
    } else {
      transportValue = await ask("Robot base URL", getBaseUrl() || DEFAULT_URL);
      urlForCtx = transportValue;
    }
  }

  if (!transportValue) {
    out.error(mode === "serial" ? "serial port required" : "robot URL required");
    process.exitCode = 1;
    return;
  }

  if (!args.yes) {
    const ok = await confirm(`Write hook configs for: ${selected.join(", ")}?`, true);
    if (!ok) {
      out.info("setup cancelled");
      process.exitCode = 0;
      return;
    }
  }

  /** @type {import("../integrations/types.js").InstallPlan[]} */
  const plans = [];

  for (const id of selected) {
    const integration = getIntegration(id);
    if (!integration?.planInstall || !integration.applyInstall) {
      out.error(`integration ${id} does not support setup`);
      process.exitCode = 1;
      return;
    }

    const cliCommand = args.command
      ? `${args.command} hook ${id}`
      : hookCommandFor(id);

    const ctx = {
      projectRoot,
      cliCommand,
      timeout: DEFAULT_TIMEOUT,
      url: urlForCtx,
    };
    plans.push(integration.planInstall(ctx));
  }

  const envPath = join(projectRoot, ".env");
  const envFlags = {
    forceUrl: args.forceUrl,
    forceSerial: args.forceSerial,
    dryRun: args.dryRun,
  };

  if (args.dryRun) {
    out.ok("dry-run — no files written");
    for (const plan of plans) {
      out.info(`${plan.relativePath}`);
      out.info(`  events: ${plan.events.join(", ")}`);
      out.info(`  command: ${plan.commandPreview}`);
    }
    applyTransportEnv(out, envPath, mode, transportValue, envFlags);
    process.exitCode = 0;
    return;
  }

  for (const id of selected) {
    const integration = getIntegration(id);
    const cliCommand = args.command
      ? `${args.command} hook ${id}`
      : hookCommandFor(id);
    const ctx = {
      projectRoot,
      cliCommand,
      timeout: DEFAULT_TIMEOUT,
      url: urlForCtx,
    };
    const rel = integration.configRelativePath();
    const abs = join(projectRoot, rel);
    const existing = readJsonFile(abs);
    if (existing === null && existsSync(abs)) {
      out.error(`${rel} is not valid JSON; fix it before running setup`);
      process.exitCode = 1;
      return;
    }
    const merged = integration.applyInstall(ctx, existing);
    writeJsonFile(abs, merged);
    out.ok(`wrote ${rel}`);
  }

  applyTransportEnv(out, envPath, mode, transportValue, envFlags);

  out.ok("setup complete — run: tiny-engineer doctor");
  process.exitCode = 0;
}

function printSetupHelp() {
  console.log(`Usage: tiny-engineer setup [ide...] [options]

Merge Tiny Engineer hook commands into the current project.
Interactive setup asks HTTP (Wi-Fi) vs Serial (USB), then writes one transport key to .env.

Options:
  --all              Install all integrations (cursor, claude-code, antigravity)
  --yes, -y          Non-interactive (requires --all or ide args)
  --dry-run          Print plan without writing files
  --url <base>       HTTP mode; merge TINY_ENGINEER_URL into .env (clears SERIAL)
  --force-url        Overwrite existing TINY_ENGINEER_URL in .env
  --serial [path]    Serial mode; path or auto (bare --serial = auto); clears URL
  --force-serial     Overwrite existing TINY_ENGINEER_SERIAL in .env
  --command <prefix> Override CLI launcher (default: npx -y tiny-engineer)
  -h, --help         Show this help

Examples:
  npx -y tiny-engineer setup
  npx -y tiny-engineer setup cursor --yes
  npx -y tiny-engineer setup --all --yes --url http://192.168.1.10
  npx -y tiny-engineer setup cursor --yes --serial=auto
  tiny-engineer setup --dry-run
  tiny-engineer setup cursor --yes --command 'node packages/tiny-engineer-cli/bin/tiny-engineer.js'
`);
}
