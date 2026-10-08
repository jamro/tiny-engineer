import { existsSync, readFileSync } from "node:fs";
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

export const MARKERS = ["hook cursor", "tiny-engineer-cursor"];

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
export function isOurCommand(command) {
  if (typeof command !== "string") return false;
  return MARKERS.some((m) => command.includes(m));
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
    const has = list.some((item) => isOurCommand(/** @type {{ command?: string }} */ (item)?.command));
    if (!has) list.push({ ...entry });
    root.hooks[event] = list;
  }

  return root;
}

/**
 * @param {object | null} existing
 * @returns {number}
 */
function countOurs(existing) {
  if (!existing?.hooks || typeof existing.hooks !== "object") return 0;
  let n = 0;
  for (const list of Object.values(existing.hooks)) {
    if (!Array.isArray(list)) continue;
    for (const item of list) {
      if (isOurCommand(item?.command)) n++;
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
    ide: "cursor",
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
    return { version: 1, hooks: {} };
  }

  /** @type {Record<string, unknown>} */
  const root = structuredClone(existing);
  /** @type {Record<string, object[]>} */
  const hooks =
    root.hooks && typeof root.hooks === "object"
      ? /** @type {Record<string, object[]>} */ (root.hooks)
      : {};

  /** @type {Record<string, object[]>} */
  const next = {};
  for (const [event, list] of Object.entries(hooks)) {
    if (!Array.isArray(list)) continue;
    const kept = list.filter(
      (item) => !isOurCommand(/** @type {{ command?: string }} */ (item)?.command),
    );
    if (kept.length > 0) next[event] = kept;
  }
  root.hooks = next;
  if (typeof root.version !== "number") root.version = 1;
  return root;
}
