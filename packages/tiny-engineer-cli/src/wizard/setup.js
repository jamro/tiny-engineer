import { existsSync } from "node:fs";
import { join } from "node:path";

import {
  DEFAULT_TRANSPORT,
  DEFAULT_URL,
  TOKEN_KEY,
  getBaseUrl,
  getSerialPort,
  listConnectionKeysInFile,
  normalizeTransport,
  resolveTransport,
} from "../config/env.js";
import { userEnvPath } from "../config/paths.js";
import { AI_USAGE_LCD_KEY } from "../integrations/cursor/lcd-usage.js";
import { all as allIntegrations, get as getIntegration } from "../integrations/registry.js";
import { hookCommandFor } from "./cliPath.js";
import { mergeEnvKey, readEnvKey, readJsonFile, writeJsonFile } from "./merge.js";
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
 *   url?: string,
 *   transport?: string,
 *   port?: string,
 *   token?: string,
 *   command?: string,
 *   aiUsageLcd?: boolean,
 *   ides: string[],
 *   error?: string,
 * }}
 */
export function parseSetupArgs(argv) {
  let help = false;
  let yes = false;
  let all = false;
  let dryRun = false;
  /** @type {string | undefined} */
  let url;
  /** @type {string | undefined} */
  let transport;
  /** @type {string | undefined} */
  let port;
  /** @type {string | undefined} */
  let token;
  /** @type {string | undefined} */
  let command;
  /** @type {boolean | undefined} */
  let aiUsageLcd;
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
    if (arg === "--ai-usage-lcd") {
      aiUsageLcd = true;
      continue;
    }
    if (arg === "--no-ai-usage-lcd") {
      aiUsageLcd = false;
      continue;
    }
    if (arg === "--force-url" || arg === "--force-transport") {
      return {
        help,
        yes,
        all,
        dryRun,
        ides,
        error: `${arg} removed; interactive setup confirms replaces, --yes keeps existing`,
      };
    }
    if (arg === "--url") {
      const next = argv[++i];
      if (!next) return { help, yes, all, dryRun, ides, error: "--url requires a value" };
      url = next;
      continue;
    }
    if (arg.startsWith("--url=")) {
      url = arg.slice("--url=".length);
      if (!url) return { help, yes, all, dryRun, ides, error: "--url requires a value" };
      continue;
    }
    if (arg === "--transport") {
      const next = argv[++i];
      if (!next) {
        return { help, yes, all, dryRun, ides, error: "--transport requires a value" };
      }
      transport = next;
      continue;
    }
    if (arg.startsWith("--transport=")) {
      transport = arg.slice("--transport=".length);
      if (!transport) {
        return { help, yes, all, dryRun, ides, error: "--transport requires a value" };
      }
      continue;
    }
    if (arg === "--port") {
      const next = argv[++i];
      if (!next) return { help, yes, all, dryRun, ides, error: "--port requires a value" };
      port = next;
      continue;
    }
    if (arg.startsWith("--port=")) {
      port = arg.slice("--port=".length);
      if (!port) return { help, yes, all, dryRun, ides, error: "--port requires a value" };
      continue;
    }
    if (arg === "--token") {
      const next = argv[++i];
      if (next === undefined) return { help, yes, all, dryRun, ides, error: "--token requires a value" };
      token = next;
      continue;
    }
    if (arg.startsWith("--token=")) {
      token = arg.slice("--token=".length);
      continue;
    }
    if (arg === "--command") {
      const next = argv[++i];
      if (!next) return { help, yes, all, dryRun, ides, error: "--command requires a value" };
      command = next;
      continue;
    }
    if (arg.startsWith("--command=")) {
      command = arg.slice("--command=".length);
      if (!command) {
        return { help, yes, all, dryRun, ides, error: "--command requires a value" };
      }
      continue;
    }
    if (arg.startsWith("-")) {
      return { help, yes, all, dryRun, ides, error: `Unknown argument: ${arg}` };
    }
    ides.push(arg);
  }

  return { help, yes, all, dryRun, url, transport, port, token, command, aiUsageLcd, ides };
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
 * Warn when project `.env` overrides user/global connection keys.
 * @param {import("../util/output.js").Output} out
 * @param {string} projectRoot
 */
export function warnProjectOverrides(out, projectRoot) {
  const keys = listConnectionKeysInFile(join(projectRoot, ".env"));
  if (keys.length === 0) return;
  const named = keys.map((k) => (k === TOKEN_KEY ? `${TOKEN_KEY} (set)` : k)).join(", ");
  out.info(`warning: project .env overrides user config: ${named}`);
}

/**
 * Persist one connection key to user env with confirm / --yes keep rules.
 * @param {import("../util/output.js").Output} out
 * @param {{
 *   key: string,
 *   value: string,
 *   yes: boolean,
 *   dryRun: boolean,
 *   label: string,
 *   secret?: boolean,
 *   confirmFn?: typeof confirm,
 * }} opts
 * @returns {Promise<"created" | "updated" | "skipped" | "unchanged" | "kept" | "dry-run">}
 */
async function persistUserEnvKey(
  out,
  { key, value, yes, dryRun, label, secret = false, confirmFn = confirm },
) {
  const envPath = userEnvPath();
  const existing = readEnvKey(envPath, key);

  if (existing !== null && existing === value) {
    out.info(`user config ${label}: unchanged`);
    return "unchanged";
  }

  if (existing !== null && existing !== value) {
    if (yes) {
      out.info(`user config ${label}: kept existing (differs from proposed)`);
      return "kept";
    }
    const prompt = secret
      ? `Replace stored ${label} in user config?`
      : `Replace user config ${label}: ${existing} → ${value}?`;
    const ok = await confirmFn(prompt, false);
    if (!ok) {
      out.info(`user config ${label}: kept existing`);
      return "kept";
    }
  }

  if (dryRun) {
    out.info(`user config ${label}: would write ${secret ? "(secret)" : value}`);
    return "dry-run";
  }

  const result = mergeEnvKey(envPath, key, value, { force: true, private: true });
  out.ok(`user config ${label}: ${result}`);
  return result;
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 * @param {{
 *   projectRoot?: string,
 *   ask?: typeof ask,
 *   confirm?: typeof confirm,
 *   multiSelect?: typeof multiSelect,
 * }} [opts]
 */
export async function runSetup(
  out,
  argv,
  {
    projectRoot = process.cwd(),
    ask: askFn = ask,
    confirm: confirmFn = confirm,
    multiSelect: multiSelectFn = multiSelect,
  } = {},
) {
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

  const userEnv = userEnvPath();
  out.info(`User config (global): ${userEnv}`);
  warnProjectOverrides(out, projectRoot);

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
    selected = await multiSelectFn("Install Tiny Engineer hooks for:", choices);
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
    const answer = await askFn(
      `Host transport (http or serial) [user config]`,
      defaultTransport,
    );
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
  /** @type {string | undefined} */
  let token = args.token;

  if (transport === "http") {
    if (!args.yes) {
      url = await askFn(`Robot base URL [user config]`, url || getBaseUrl() || DEFAULT_URL);
      // Prompt only when --token absent so Enter cannot wipe the flag.
      if (args.token === undefined) {
        const tokenAnswer = await askFn(`Access token optional, Enter to skip [user config]`, "");
        token = tokenAnswer || undefined;
      }
    } else {
      url = url || getBaseUrl();
    }
  } else if (!args.yes) {
    const portDefault = port || getSerialPort() || "";
    const answer = await askFn(`Serial port optional, Enter to skip [user config]`, portDefault);
    port = answer || undefined;
  }

  if (!args.yes) {
    const ok = await confirmFn(
      `Write project hook configs for: ${selected.join(", ")}?`,
      true,
    );
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
    out.info("Project hooks:");
    for (const plan of plans) {
      out.info(`  ${plan.relativePath}`);
      out.info(`    events: ${plan.events.join(", ")}`);
      out.info(`    command: ${plan.commandPreview}`);
    }
    out.info(`User config (global): ${userEnv}`);
    out.info(`  TINY_ENGINEER_TRANSPORT=${transport}`);
    if (transport === "http" && url) {
      out.info(`  TINY_ENGINEER_URL=${url}`);
    }
    if (transport === "http" && token) {
      out.info(`  TINY_ENGINEER_TOKEN=(secret)`);
    }
    if (transport === "serial" && port) {
      out.info(`  TINY_ENGINEER_PORT=${port}`);
    }
    process.exitCode = 0;
    return;
  }

  out.info("Project hooks:");
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
    out.ok(`  wrote ${rel}`);
  }

  out.info(`User config (global): ${userEnv}`);
  await persistUserEnvKey(out, {
    key: "TINY_ENGINEER_TRANSPORT",
    value: transport,
    yes: args.yes,
    dryRun: false,
    label: "TINY_ENGINEER_TRANSPORT",
    confirmFn,
  });

  if (transport === "http" && url) {
    await persistUserEnvKey(out, {
      key: "TINY_ENGINEER_URL",
      value: url,
      yes: args.yes,
      dryRun: false,
      label: "TINY_ENGINEER_URL",
      confirmFn,
    });
  }

  if (transport === "http" && token) {
    await persistUserEnvKey(out, {
      key: TOKEN_KEY,
      value: token,
      yes: args.yes,
      dryRun: false,
      label: "TINY_ENGINEER_TOKEN",
      secret: true,
      confirmFn,
    });
  }

  if (transport === "serial" && port) {
    await persistUserEnvKey(out, {
      key: "TINY_ENGINEER_PORT",
      value: port,
      yes: args.yes,
      dryRun: false,
      label: "TINY_ENGINEER_PORT",
      confirmFn,
    });
  }

  /** @type {boolean | undefined} */
  let aiUsageLcd = args.aiUsageLcd;
  if (aiUsageLcd === undefined && !args.yes) {
    aiUsageLcd = await confirmFn(
      "Show remaining Cursor AI budget on LCD1602 after completed turns?",
      false,
    );
  }
  if (aiUsageLcd !== undefined) {
    await persistUserEnvKey(out, {
      key: AI_USAGE_LCD_KEY,
      value: aiUsageLcd ? "1" : "0",
      yes: args.yes,
      dryRun: false,
      label: AI_USAGE_LCD_KEY,
      confirmFn,
    });
  }

  out.ok("setup complete — run: tiny-engineer doctor");
  process.exitCode = 0;
}

function printSetupHelp() {
  console.log(`Usage: tiny-engineer setup [ide...] [options]

Merge Tiny Engineer hook commands into the current project (project-local).
Connection prefs (transport, URL, port, token, optional LCD AI budget) go to user config (global).
Cursor LCD AI budget updates are opt-in (TINY_ENGINEER_AI_USAGE_LCD=1).

Options:
  --all                   Install all integrations (cursor, claude-code, antigravity)
  --yes, -y               Non-interactive (requires --all or ide args); keeps existing user config on conflict
  --dry-run               Print plan without writing files
  --transport <http|serial>
                          Host transport for user config (default: http)
  --url <base>            Robot URL when transport is http (user config)
  --port <path>           Optional serial port when transport is serial (user config)
  --token <value>         Optional HTTP access token (user config; omit to skip)
  --ai-usage-lcd          Enable LCD AI budget after Cursor ring (user config)
  --no-ai-usage-lcd       Disable LCD AI budget (user config)
  --command <prefix>      Override CLI launcher (default: npx -y tiny-engineer)
  -h, --help              Show this help

Examples:
  npx -y tiny-engineer setup
  npx -y tiny-engineer setup cursor --yes
  npx -y tiny-engineer setup --all --yes --url http://192.168.1.10
  npx -y tiny-engineer setup cursor --yes --transport serial
  npx -y tiny-engineer setup cursor --yes --ai-usage-lcd
  tiny-engineer setup --dry-run
  tiny-engineer setup cursor --yes --command 'node packages/tiny-engineer-cli/bin/tiny-engineer.js'
`);
}
