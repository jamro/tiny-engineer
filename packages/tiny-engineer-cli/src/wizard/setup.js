import { existsSync } from "node:fs";
import { homedir } from "node:os";
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
 *   global: boolean,
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
  let globalHooks = false;
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
    if (arg === "--global") {
      globalHooks = true;
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
        global: globalHooks,
        ides,
        error: `${arg} removed; interactive setup confirms replaces, --yes keeps existing`,
      };
    }
    if (arg === "--url") {
      const next = argv[++i];
      if (!next) return { help, yes, all, dryRun, global: globalHooks, ides, error: "--url requires a value" };
      url = next;
      continue;
    }
    if (arg.startsWith("--url=")) {
      url = arg.slice("--url=".length);
      if (!url) return { help, yes, all, dryRun, global: globalHooks, ides, error: "--url requires a value" };
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

  return {
    help,
    yes,
    all,
    dryRun,
    global: globalHooks,
    url,
    transport,
    port,
    token,
    command,
    aiUsageLcd,
    ides,
  };
}

/**
 * Non-Cursor integrations cannot install user-level hooks yet.
 * @param {import("../util/output.js").Output} out
 * @param {string[]} ids
 * @returns {boolean} true when the caller must stop
 */
function rejectUnsupportedGlobal(out, ids) {
  const rejected = ids.filter((id) => id !== "cursor");
  if (rejected.length === 0) return false;
  out.error(`--global not supported for: ${rejected.join(", ")}`);
  process.exitCode = 1;
  return true;
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {typeof ask} askFn
 * @returns {Promise<boolean | null>} true = global, false = project, null = invalid
 */
async function askHookScope(out, askFn) {
  const raw = await askFn("Hook scope (global or project)", "project");
  const scopeAnswer = String(raw ?? "").trim().toLowerCase();
  if (scopeAnswer === "global" || scopeAnswer === "g") return true;
  if (scopeAnswer === "project" || scopeAnswer === "p" || scopeAnswer === "") return false;
  out.error(`invalid hook scope ${scopeAnswer} (use global or project)`);
  process.exitCode = 1;
  return null;
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
    out.info(secret ? `${label}: unchanged` : `${label}=${existing} (unchanged)`);
    return "unchanged";
  }

  if (existing !== null && existing !== value) {
    if (yes) {
      out.info(`${label}: kept existing`);
      return "kept";
    }
    const prompt = secret
      ? `Replace stored ${label} in user config?`
      : `Replace user config ${label}: ${existing} → ${value}?`;
    const ok = await confirmFn(prompt, false);
    if (!ok) {
      out.info(`${label}: kept existing`);
      return "kept";
    }
  }

  if (dryRun) {
    out.info(`${label}: would write ${secret ? "(secret)" : value}`);
    return "dry-run";
  }

  const result = mergeEnvKey(envPath, key, value, { force: true, private: true });
  out.ok(`${label}: ${result}`);
  return result;
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 * @param {{
 *   projectRoot?: string,
 *   home?: string,
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
    home = homedir(),
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

  if (args.global && (args.all || args.ides.length > 0)) {
    const ids = args.all ? allIntegrations().map((integration) => integration.id) : args.ides;
    if (rejectUnsupportedGlobal(out, ids)) return;
  }

  const userEnv = userEnvPath();
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
    selected = await multiSelectFn("\nInstall Tiny Engineer hooks for:", choices);
  }

  if (selected.length === 0) {
    out.error("no integrations selected");
    process.exitCode = 1;
    return;
  }

  if (!args.yes && !args.global) {
    const globalChoice = await askHookScope(out, askFn);
    if (globalChoice === null) return;
    args.global = globalChoice;
  }

  if (args.global && rejectUnsupportedGlobal(out, selected)) return;

  /** @type {"project" | "user"} */
  const scope = args.global ? "user" : "project";

  if (!args.yes) {
    out.info(`\nConnection, saved in ${userEnv}`);
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
    const answer = await askFn("Transport (http or serial)", defaultTransport);
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
      url = await askFn("Robot URL", url || getBaseUrl() || DEFAULT_URL);
      // Prompt only when --token absent so Enter cannot wipe the flag.
      if (args.token === undefined) {
        const tokenAnswer = await askFn("Access token, or empty to skip", "");
        token = tokenAnswer || undefined;
      }
    } else {
      url = url || getBaseUrl();
    }
  } else if (!args.yes) {
    const portDefault = port || getSerialPort() || "";
    const answer = await askFn("Serial port to connect, empty = autodetect", portDefault);
    port = answer || undefined;
  }

  /** @type {boolean | undefined} */
  let aiUsageLcd = args.aiUsageLcd;
  if (aiUsageLcd === undefined && !args.yes) {
    aiUsageLcd = await confirmFn(
      "\nCursor AI budget on LCD1602 after turns? Needs lcd1602 extension and lcd_cabinet mod.",
      false,
    );
  }

  if (!args.yes) {
    const hookWhere = [
      ...new Set(
        selected.map((id) =>
          args.global
            ? join(home, ".cursor", "hooks.json")
            : (getIntegration(id)?.configRelativePath?.() ?? id),
        ),
      ),
    ].join(", ");
    const ok = await confirmFn(`\nWrite hooks to ${hookWhere}?`, true);
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
      scope,
      home,
    };
    plans.push(integration.planInstall(ctx));
  }

  /** @type {boolean | undefined} */
  const aiUsageLcdPreview = aiUsageLcd;

  if (args.dryRun) {
    out.ok("dry-run — no files written");
    out.info(scope === "user" ? "User hooks:" : "Project hooks:");
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
    if (aiUsageLcdPreview !== undefined) {
      out.info(`  ${AI_USAGE_LCD_KEY}=${aiUsageLcdPreview ? "1" : "0"}`);
    }
    process.exitCode = 0;
    return;
  }

  for (const plan of plans) {
    const integration = getIntegration(plan.ide);
    const cliCommand = args.command
      ? `${args.command} hook ${plan.ide}`
      : hookCommandFor(plan.ide);
    const ctx = {
      projectRoot,
      cliCommand,
      timeout: DEFAULT_TIMEOUT,
      url,
      scope,
      home,
    };
    const abs = plan.path;
    const existing = readJsonFile(abs);
    if (existing === null && existsSync(abs)) {
      out.error(`${plan.relativePath} is not valid JSON; fix it before running setup`);
      process.exitCode = 1;
      return;
    }
    if (!integration?.applyInstall) {
      out.error(`integration ${plan.ide} does not support setup`);
      process.exitCode = 1;
      return;
    }
    const merged = integration.applyInstall(ctx, existing);
    writeJsonFile(abs, merged);
    out.ok(plan.relativePath);
  }

  if (args.yes) out.info(`User config: ${userEnv}`);
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
Interactive setup asks which IDE, then hook scope (global or project, default project).
--yes skips that question. --global skips it and writes Cursor hooks to ~/.cursor/hooks.json
(other IDEs: not supported).
Connection prefs (transport, URL, port, token, optional LCD AI budget) go to user config (global).
Cursor LCD AI budget updates are opt-in (TINY_ENGINEER_AI_USAGE_LCD=1) and need the lcd1602 extension plus the lcd_cabinet mod.

Options:
  --all                   Install all integrations (cursor, claude-code, antigravity)
  --yes, -y               Non-interactive (requires --all or ide args); keeps existing user config on conflict
  --global                Cursor user hooks (~/.cursor/hooks.json). Other IDEs are not supported
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
  npx -y tiny-engineer setup cursor --yes --global
  npx -y tiny-engineer setup --all --yes --url http://192.168.1.10
  npx -y tiny-engineer setup cursor --yes --transport serial
  npx -y tiny-engineer setup cursor --yes --ai-usage-lcd
  tiny-engineer setup --dry-run
  tiny-engineer setup cursor --yes --command 'node packages/tiny-engineer-cli/bin/tiny-engineer.js'
`);
}
