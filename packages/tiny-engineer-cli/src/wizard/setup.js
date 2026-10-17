import { existsSync } from "node:fs";
import { join } from "node:path";

import {
  DEFAULT_TRANSPORT,
  DEFAULT_URL,
  getBaseUrl,
  getSerialPort,
  normalizeTransport,
  resolveTransport,
} from "../config/env.js";
import { all as allIntegrations, get as getIntegration } from "../integrations/registry.js";
import { hookCommandFor } from "./cliPath.js";
import { mergeEnvKey, mergeEnvUrl, readJsonFile, writeJsonFile } from "./merge.js";
import { ask, confirm, multiSelect } from "./prompt.js";

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
 *   forceTransport: boolean,
 *   url?: string,
 *   transport?: string,
 *   port?: string,
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
  let forceTransport = false;
  /** @type {string | undefined} */
  let url;
  /** @type {string | undefined} */
  let transport;
  /** @type {string | undefined} */
  let port;
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
    if (arg === "--force-transport") {
      forceTransport = true;
      continue;
    }
    if (arg === "--url") {
      const next = argv[++i];
      if (!next) return { help, yes, all, dryRun, forceUrl, forceTransport, ides, error: "--url requires a value" };
      url = next;
      continue;
    }
    if (arg.startsWith("--url=")) {
      url = arg.slice("--url=".length);
      if (!url) return { help, yes, all, dryRun, forceUrl, forceTransport, ides, error: "--url requires a value" };
      continue;
    }
    if (arg === "--transport") {
      const next = argv[++i];
      if (!next) {
        return { help, yes, all, dryRun, forceUrl, forceTransport, ides, error: "--transport requires a value" };
      }
      transport = next;
      continue;
    }
    if (arg.startsWith("--transport=")) {
      transport = arg.slice("--transport=".length);
      if (!transport) {
        return { help, yes, all, dryRun, forceUrl, forceTransport, ides, error: "--transport requires a value" };
      }
      continue;
    }
    if (arg === "--port") {
      const next = argv[++i];
      if (!next) return { help, yes, all, dryRun, forceUrl, forceTransport, ides, error: "--port requires a value" };
      port = next;
      continue;
    }
    if (arg.startsWith("--port=")) {
      port = arg.slice("--port=".length);
      if (!port) return { help, yes, all, dryRun, forceUrl, forceTransport, ides, error: "--port requires a value" };
      continue;
    }
    if (arg === "--command") {
      const next = argv[++i];
      if (!next) return { help, yes, all, dryRun, forceUrl, forceTransport, ides, error: "--command requires a value" };
      command = next;
      continue;
    }
    if (arg.startsWith("--command=")) {
      command = arg.slice("--command=".length);
      if (!command) {
        return { help, yes, all, dryRun, forceUrl, forceTransport, ides, error: "--command requires a value" };
      }
      continue;
    }
    if (arg.startsWith("-")) {
      return { help, yes, all, dryRun, forceUrl, forceTransport, ides, error: `Unknown argument: ${arg}` };
    }
    ides.push(arg);
  }

  return { help, yes, all, dryRun, forceUrl, forceTransport, url, transport, port, command, ides };
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
  let transport;
  if (args.transport !== undefined) {
    const normalized = normalizeTransport(args.transport);
    if (!normalized) {
      out.error(`invalid --transport ${args.transport} (use http or serial)`);
      process.exitCode = 1;
      return;
    }
    transport = normalized;
  } else if (args.yes) {
    const resolved = resolveTransport(undefined);
    if (resolved.error) {
      out.error(resolved.error);
      process.exitCode = 1;
      return;
    }
    transport = resolved.transport;
  } else {
    const current = resolveTransport(undefined);
    const defaultTransport = current.transport ?? DEFAULT_TRANSPORT;
    const answer = await ask("Host transport (http or serial)", defaultTransport);
    const normalized = normalizeTransport(answer);
    if (!normalized) {
      out.error(`invalid transport ${answer} (use http or serial)`);
      process.exitCode = 1;
      return;
    }
    transport = normalized;
  }

  /** @type {string | undefined} */
  let url = args.url;
  /** @type {string | undefined} */
  let port = args.port;

  if (transport === "http") {
    if (!args.yes) {
      url = await ask("Robot base URL", url || getBaseUrl() || DEFAULT_URL);
    } else {
      url = url || getBaseUrl();
    }
  } else if (!args.yes) {
    const portDefault = port || getSerialPort() || "";
    const answer = await ask("Serial port (optional, Enter to skip)", portDefault);
    port = answer || undefined;
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
      url,
    };
    plans.push(integration.planInstall(ctx));
  }

  if (args.dryRun) {
    out.ok("dry-run — no files written");
    for (const plan of plans) {
      out.info(`${plan.relativePath}`);
      out.info(`  events: ${plan.events.join(", ")}`);
      out.info(`  command: ${plan.commandPreview}`);
    }
    out.info(
      `.env TINY_ENGINEER_TRANSPORT=${transport}${args.forceTransport ? " (force)" : " (if unset)"}`,
    );
    if (transport === "http" && (args.url || url)) {
      out.info(`.env TINY_ENGINEER_URL=${url}${args.forceUrl ? " (force)" : " (if unset)"}`);
    }
    if (transport === "serial" && port) {
      out.info(`.env TINY_ENGINEER_PORT=${port}`);
    }
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
      url,
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

  const envPath = join(projectRoot, ".env");
  const transportResult = mergeEnvKey(envPath, "TINY_ENGINEER_TRANSPORT", transport, {
    force: args.forceTransport,
  });
  if (transportResult === "skipped") {
    out.info(".env TINY_ENGINEER_TRANSPORT already set (use --force-transport to replace)");
  } else {
    out.ok(`.env TINY_ENGINEER_TRANSPORT ${transportResult}`);
  }

  if (transport === "http" && url) {
    const result = mergeEnvUrl(envPath, url, { force: args.forceUrl });
    if (result === "skipped") {
      out.info(".env TINY_ENGINEER_URL already set (use --force-url to replace)");
    } else {
      out.ok(`.env TINY_ENGINEER_URL ${result}`);
    }
  }

  if (transport === "serial" && port) {
    const result = mergeEnvKey(envPath, "TINY_ENGINEER_PORT", port, { force: true });
    out.ok(`.env TINY_ENGINEER_PORT ${result}`);
  }

  out.ok("setup complete — run: tiny-engineer doctor");
  process.exitCode = 0;
}

function printSetupHelp() {
  console.log(`Usage: tiny-engineer setup [ide...] [options]

Merge Tiny Engineer hook commands into the current project.

Options:
  --all                   Install all integrations (cursor, claude-code, antigravity)
  --yes, -y               Non-interactive (requires --all or ide args)
  --dry-run               Print plan without writing files
  --transport <http|serial>
                          Host transport for project .env (default: http)
  --force-transport       Overwrite existing TINY_ENGINEER_TRANSPORT in .env
  --url <base>            Robot URL when transport is http; merge into .env when unset
  --force-url             Overwrite existing TINY_ENGINEER_URL in .env
  --port <path>           Optional serial port when transport is serial
  --command <prefix>      Override CLI launcher (default: npx -y tiny-engineer)
  -h, --help              Show this help

Examples:
  npx -y tiny-engineer setup
  npx -y tiny-engineer setup cursor --yes
  npx -y tiny-engineer setup --all --yes --url http://192.168.1.10
  npx -y tiny-engineer setup cursor --yes --transport serial
  tiny-engineer setup --dry-run
  tiny-engineer setup cursor --yes --command 'node packages/tiny-engineer-cli/bin/tiny-engineer.js'
`);
}
