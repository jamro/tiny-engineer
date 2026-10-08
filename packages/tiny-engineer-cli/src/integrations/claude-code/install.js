import { existsSync } from "node:fs";
import { join } from "node:path";

export const CLAUDE_EVENTS = [
  "SessionStart",
  "SessionEnd",
  "UserPromptSubmit",
  "SubagentStart",
  "PreToolUse",
  "PostToolBatch",
  "PostToolUseFailure",
  "PreCompact",
  "PermissionRequest",
  "PermissionDenied",
  "Notification",
  "Stop",
  "StopFailure",
];

const MARKERS = ["hook claude-code", "tiny-engineer-claude-code"];

/**
 * @param {string} projectRoot
 * @returns {boolean}
 */
export function detect(projectRoot) {
  return (
    existsSync(join(projectRoot, ".claude")) ||
    existsSync(join(projectRoot, ".claude", "settings.json"))
  );
}

/**
 * @returns {string}
 */
export function configRelativePath() {
  return ".claude/settings.json";
}

/**
 * @param {unknown} eventBlock
 * @returns {boolean}
 */
function blockHasOurs(eventBlock) {
  if (!Array.isArray(eventBlock)) return false;
  for (const wrapper of eventBlock) {
    const hooks = wrapper?.hooks;
    if (!Array.isArray(hooks)) continue;
    for (const h of hooks) {
      if (typeof h?.command === "string" && MARKERS.some((m) => h.command.includes(m))) {
        return true;
      }
    }
  }
  return false;
}

/**
 * @param {import("../types.js").InstallContext} ctx
 * @returns {import("../types.js").InstallPlan}
 */
export function planInstall(ctx) {
  return {
    ide: "claude-code",
    path: join(ctx.projectRoot, configRelativePath()),
    relativePath: configRelativePath(),
    events: [...CLAUDE_EVENTS],
    commandPreview: ctx.cliCommand,
  };
}

/**
 * @param {import("../types.js").InstallContext} ctx
 * @param {object | null} existing
 * @returns {object}
 */
export function applyInstall(ctx, existing) {
  /** @type {Record<string, unknown>} */
  const root =
    existing && typeof existing === "object" ? structuredClone(existing) : {};

  /** @type {Record<string, unknown>} */
  const hooks =
    root.hooks && typeof root.hooks === "object"
      ? /** @type {Record<string, unknown>} */ (root.hooks)
      : {};

  const entry = {
    hooks: [
      {
        type: "command",
        command: ctx.cliCommand,
        async: true,
      },
    ],
  };

  for (const event of CLAUDE_EVENTS) {
    if (blockHasOurs(hooks[event])) continue;
    const list = Array.isArray(hooks[event]) ? [...hooks[event]] : [];
    list.push(structuredClone(entry));
    hooks[event] = list;
  }

  root.hooks = hooks;
  return root;
}
