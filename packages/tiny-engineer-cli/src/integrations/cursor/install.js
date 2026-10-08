import { existsSync } from "node:fs";
import { join } from "node:path";

export const CURSOR_EVENTS = [
  "sessionStart",
  "beforeSubmitPrompt",
  "afterAgentThought",
  "preCompact",
  "preToolUse",
  "beforeReadFile",
  "beforeShellExecution",
  "subagentStart",
  "afterFileEdit",
  "stop",
];

const MARKERS = ["hook cursor", "tiny-engineer-cursor"];

/**
 * @param {string} projectRoot
 * @returns {boolean}
 */
export function detect(projectRoot) {
  return (
    existsSync(join(projectRoot, ".cursor")) ||
    existsSync(join(projectRoot, ".cursor", "hooks.json"))
  );
}

/**
 * @returns {string}
 */
export function configRelativePath() {
  return ".cursor/hooks.json";
}

/**
 * @param {string} command
 * @returns {boolean}
 */
function isOurs(command) {
  if (typeof command !== "string") return false;
  return MARKERS.some((m) => command.includes(m));
}

/**
 * @param {import("../types.js").InstallContext} ctx
 * @returns {import("../types.js").InstallPlan}
 */
export function planInstall(ctx) {
  return {
    ide: "cursor",
    path: join(ctx.projectRoot, configRelativePath()),
    relativePath: configRelativePath(),
    events: [...CURSOR_EVENTS],
    commandPreview: ctx.cliCommand,
  };
}

/**
 * @param {import("../types.js").InstallContext} ctx
 * @param {object | null} existing
 * @returns {object}
 */
export function applyInstall(ctx, existing) {
  /** @type {{ version: number, hooks: Record<string, object[]> }} */
  const root = {
    version: 1,
    hooks: {},
  };

  if (existing && typeof existing === "object") {
    if (typeof existing.version === "number") root.version = existing.version;
    if (existing.hooks && typeof existing.hooks === "object") {
      for (const [event, list] of Object.entries(existing.hooks)) {
        root.hooks[event] = Array.isArray(list) ? [...list] : [];
      }
    }
  }

  const entry = { command: ctx.cliCommand, timeout: ctx.timeout };

  for (const event of CURSOR_EVENTS) {
    const list = root.hooks[event] ?? [];
    const has = list.some((item) => isOurs(/** @type {{ command?: string }} */ (item)?.command));
    if (!has) list.push({ ...entry });
    root.hooks[event] = list;
  }

  return root;
}
