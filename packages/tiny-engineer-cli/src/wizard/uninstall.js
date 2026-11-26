import { homedir } from "node:os";

import { all as allIntegrations, get as getIntegration } from "../integrations/registry.js";
import { readJsonFile, writeJsonFile } from "./merge.js";
import { confirm, multiSelect } from "./prompt.js";

/**
 * @param {string[]} argv
 * @returns {{
 *   help: boolean,
 *   yes: boolean,
 *   all: boolean,
 *   dryRun: boolean,
 *   global: boolean,
 *   ides: string[],
 *   error?: string,
 * }}
 */
export function parseUninstallArgs(argv) {
  let help = false;
  let yes = false;
  let all = false;
  let dryRun = false;
  let globalHooks = false;
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
    if (arg === "--purge-env") {
      return {
        help,
        yes,
        all,
        dryRun,
        global: globalHooks,
        ides,
        error: "--purge-env removed; edit user config env or project .env by hand",
      };
    }
    if (arg.startsWith("-")) {
      return { help, yes, all, dryRun, global: globalHooks, ides, error: `Unknown argument: ${arg}` };
    }
    ides.push(arg);
  }

  return { help, yes, all, dryRun, global: globalHooks, ides };
}

/**
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
 * @param {string[]} argv
 * @param {{ projectRoot?: string, home?: string }} [opts]
 */
export async function runUninstall(out, argv, { projectRoot = process.cwd(), home = homedir() } = {}) {
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

  if (args.global && (args.all || args.ides.length > 0)) {
    const ids = args.all ? allIntegrations().map((integration) => integration.id) : args.ides;
    if (rejectUnsupportedGlobal(out, ids)) return;
  }

  /** @type {"project" | "user"} */
  const scope = args.global ? "user" : "project";
  const planOpts = { scope, home };

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
        const plan = i.planUninstall(projectRoot, planOpts);
        return { id: i.id, name: i.name, removeCount: plan.removeCount };
      })
      .filter((p) => p.removeCount > 0);

    if (withHooks.length === 0) {
      out.info(
        args.global
          ? "no Tiny Engineer hooks found in user Cursor config"
          : "no Tiny Engineer hooks found in this project",
      );
      selected = [];
    } else {
      selected = await multiSelect(
        args.global ? "Remove Tiny Engineer user hooks for:" : "Remove Tiny Engineer hooks for:",
        withHooks.map((p) => ({
          id: p.id,
          label: `${p.name} (${p.removeCount})`,
        })),
      );
    }
  }

  if (args.global && selected.length > 0 && rejectUnsupportedGlobal(out, selected)) return;

  /** @type {import("../integrations/types.js").UninstallPlan[]} */
  const plans = [];
  for (const id of selected) {
    const integration = getIntegration(id);
    if (!integration?.planUninstall || !integration.applyUninstall) {
      out.error(`integration ${id} does not support uninstall`);
      process.exitCode = 1;
      return;
    }
    plans.push(integration.planUninstall(projectRoot, planOpts));
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
    process.exitCode = 0;
    return;
  }

  if (total === 0) {
    out.ok("nothing to remove");
    process.exitCode = 0;
    return;
  }

  if (!args.yes) {
    const labels = plans.map((p) => `${p.ide} (${p.removeCount})`).join(", ");
    const ok = await confirm(`Remove ${labels}?`, true);
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

  out.ok("uninstall complete");
  process.exitCode = 0;
}

function printUninstallHelp() {
  console.log(`Usage: tiny-engineer uninstall [ide...] [options]

Remove Tiny Engineer hook entries from the current project.
--global removes Cursor entries from ~/.cursor/hooks.json (other IDEs: not supported).
Only commands matching Tiny Engineer markers are removed; other hooks stay.
Config files are never deleted. Connection prefs (user config / .env) are not touched.

Options:
  --all              Target all integrations
  --yes, -y          Non-interactive (requires --all or ide args)
  --global           Cursor user hooks (~/.cursor/hooks.json). Other IDEs are not supported
  --dry-run          Print plan without writing files
  -h, --help         Show this help

Examples:
  tiny-engineer uninstall
  tiny-engineer uninstall cursor --yes
  tiny-engineer uninstall cursor --yes --global
  tiny-engineer uninstall --all --yes --dry-run
`);
}
