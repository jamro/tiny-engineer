import { existsSync, readFileSync } from "node:fs";
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

export const MARKERS = ["hook claude-code", "tiny-engineer-claude-code"];

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
 * @param {string} command
 * @returns {boolean}
 */
export function isOurCommand(command) {
  if (typeof command !== "string") return false;
  return MARKERS.some((m) => command.includes(m));
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
      if (isOurCommand(h?.command)) return true;
    }
  }
  return false;
}

/**
 * @param {string} path
 * @returns {object | null}
 */
function readConfig(path) {
  if (!existsSync(path)) return null;
  try {
    const parsed = JSON.parse(readFileSync(path, "utf8"));
    return parsed && typeof parsed === "object" ? parsed : null;
  } catch {
    return null;
  }
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

/**
 * @param {object | null} existing
 * @returns {number}
 */
function countOurs(existing) {
  if (!existing?.hooks || typeof existing.hooks !== "object") return 0;
  let n = 0;
  for (const eventBlock of Object.values(existing.hooks)) {
    if (!Array.isArray(eventBlock)) continue;
    for (const wrapper of eventBlock) {
      const hooks = wrapper?.hooks;
      if (!Array.isArray(hooks)) continue;
      for (const h of hooks) {
        if (isOurCommand(h?.command)) n++;
      }
    }
  }
  return n;
}

/**
 * @param {string} projectRoot
 * @returns {import("../types.js").UninstallPlan}
 */
export function planUninstall(projectRoot) {
  const relativePath = configRelativePath();
  const path = join(projectRoot, relativePath);
  const existing = readConfig(path);
  const removeCount = countOurs(existing);
  return {
    ide: "claude-code",
    path,
    relativePath,
    removeCount,
    detail: removeCount === 1 ? "1 hook entry" : `${removeCount} hook entries`,
  };
}

/**
 * @param {object | null} existing
 * @returns {object}
 */
export function applyUninstall(existing) {
  if (!existing || typeof existing !== "object") {
    return { hooks: {} };
  }

  /** @type {Record<string, unknown>} */
  const root = structuredClone(existing);
  /** @type {Record<string, unknown>} */
  const hooks =
    root.hooks && typeof root.hooks === "object"
      ? /** @type {Record<string, unknown>} */ (root.hooks)
      : {};

  /** @type {Record<string, unknown>} */
  const next = {};
  for (const [event, eventBlock] of Object.entries(hooks)) {
    if (!Array.isArray(eventBlock)) continue;
    /** @type {object[]} */
    const keptWrappers = [];
    for (const wrapper of eventBlock) {
      if (!wrapper || typeof wrapper !== "object") continue;
      const inner = Array.isArray(wrapper.hooks) ? wrapper.hooks : [];
      const keptInner = inner.filter((h) => !isOurCommand(h?.command));
      if (keptInner.length === 0) continue;
      keptWrappers.push({ ...wrapper, hooks: keptInner });
    }
    if (keptWrappers.length > 0) next[event] = keptWrappers;
  }
  root.hooks = next;
  return root;
}
