import { join } from "node:path";

import { all as allIntegrations, get as getIntegration } from "../integrations/registry.js";
import { readJsonFile, stripEnvTinyEngineer, writeJsonFile } from "./merge.js";
import { confirm, multiSelect } from "./prompt.js";

/**
 * @param {string[]} argv
 * @returns {{
 *   help: boolean,
 *   yes: boolean,
 *   all: boolean,
 *   dryRun: boolean,
 *   purgeEnv: boolean,
 *   ides: string[],
 *   error?: string,
 * }}
 */
export function parseUninstallArgs(argv) {
  let help = false;
  let yes = false;
  let all = false;
  let dryRun = false;
  let purgeEnv = false;
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
    if (arg === "--purge-env") {
      purgeEnv = true;
      continue;
    }
    if (arg.startsWith("-")) {
      return { help, yes, all, dryRun, purgeEnv, ides, error: `Unknown argument: ${arg}` };
    }
    ides.push(arg);
  }

  return { help, yes, all, dryRun, purgeEnv, ides };
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 * @param {{ projectRoot?: string }} [opts]
 */
export async function runUninstall(out, argv, { projectRoot = process.cwd() } = {}) {
  const args = parseUninstallArgs(argv);
  if (args.error) {
    out.error(args.error);
    process.exitCode = 1;
    return;
  }
  if (args.help) {
    printUninstallHelp();
    return;
  }

  if (args.yes && !args.all && args.ides.length === 0) {
    out.error("uninstall --yes requires --all or at least one ide id");
    process.exitCode = 1;
    return;
  }

  for (const id of args.ides) {
    if (!getIntegration(id)) {
      out.error(
        `Unknown integration: ${id} (available: ${allIntegrations().map((i) => i.id).join(", ")})`,
      );
      process.exitCode = 1;
      return;
    }
  }

  /** @type {string[]} */
  let selected;
  if (args.all) {
    selected = allIntegrations().map((i) => i.id);
  } else if (args.ides.length > 0) {
    selected = args.ides;
  } else {
    const withHooks = allIntegrations()
      .filter((i) => i.planUninstall)
      .map((i) => {
        const plan = i.planUninstall(projectRoot);
        return { id: i.id, name: i.name, removeCount: plan.removeCount };
      })
      .filter((p) => p.removeCount > 0);

    if (withHooks.length === 0) {
      out.info("no Tiny Engineer hooks found in this project");
      if (args.purgeEnv) {
        // still allow interactive purge? skip without --yes path for env-only
      }
      selected = [];
    } else {
      selected = await multiSelect(
        "Remove Tiny Engineer hooks for:",
        withHooks.map((p) => ({
          id: p.id,
          label: `${p.name} (${p.removeCount})`,
        })),
      );
    }
  }

  /** @type {import("../integrations/types.js").UninstallPlan[]} */
  const plans = [];
  for (const id of selected) {
    const integration = getIntegration(id);
    if (!integration?.planUninstall || !integration.applyUninstall) {
      out.error(`integration ${id} does not support uninstall`);
      process.exitCode = 1;
      return;
    }
    plans.push(integration.planUninstall(projectRoot));
  }

  const total = plans.reduce((n, p) => n + p.removeCount, 0);

  if (args.dryRun) {
    out.ok("dry-run — no files written");
    if (plans.length === 0) {
      out.info("no integrations selected");
    }
    for (const plan of plans) {
      out.info(`${plan.relativePath}: remove ${plan.detail}`);
    }
    if (args.purgeEnv) {
      out.info(".env: strip TINY_ENGINEER_URL / TINY_ENGINEER_TOKEN");
    }
    process.exitCode = 0;
    return;
  }

  if (total === 0 && !args.purgeEnv) {
    out.ok("nothing to remove");
    process.exitCode = 0;
    return;
  }

  if (!args.yes) {
    const labels =
      plans.length > 0
        ? plans.map((p) => `${p.ide} (${p.removeCount})`).join(", ")
        : "(hooks none)";
    const envNote = args.purgeEnv ? " + purge .env Tiny Engineer keys" : "";
    const ok = await confirm(`Remove ${labels}${envNote}?`, true);
    if (!ok) {
      out.info("uninstall cancelled");
      process.exitCode = 0;
      return;
    }
  }

  for (const plan of plans) {
    if (plan.removeCount === 0) {
      out.info(`${plan.relativePath}: nothing to remove`);
      continue;
    }
    const integration = getIntegration(plan.ide);
    const existing = readJsonFile(plan.path);
    const next = integration.applyUninstall(existing);
    writeJsonFile(plan.path, next);
    out.ok(`updated ${plan.relativePath} (removed ${plan.detail})`);
  }

  if (args.purgeEnv) {
    const envPath = join(projectRoot, ".env");
    const result = stripEnvTinyEngineer(envPath);
    if (result === "removed") out.ok(".env: removed TINY_ENGINEER_* keys");
    else if (result === "absent") out.info(".env: absent");
    else out.info(".env: no TINY_ENGINEER_* keys");
  }

  out.ok("uninstall complete");
  process.exitCode = 0;
}

function printUninstallHelp() {
  console.log(`Usage: tiny-engineer uninstall [ide...] [options]

Remove Tiny Engineer hook entries from the current project.
Only commands matching Tiny Engineer markers are removed; other hooks stay.
Config files are never deleted.

Options:
  --all              Target all integrations
  --yes, -y          Non-interactive (requires --all or ide args)
  --dry-run          Print plan without writing files
  --purge-env        Also remove TINY_ENGINEER_URL / TINY_ENGINEER_TOKEN from .env
  -h, --help         Show this help

Examples:
  tiny-engineer uninstall
  tiny-engineer uninstall cursor --yes
  tiny-engineer uninstall --all --yes --dry-run
  tiny-engineer uninstall --all --yes --purge-env
`);
}
