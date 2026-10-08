import { existsSync, readFileSync } from "node:fs";
import { join } from "node:path";

export const ANTIGRAVITY_EVENTS = [
  "PreInvocation",
  "PreToolUse",
  "PostToolUse",
  "Stop",
];

/**
 * @param {string} projectRoot
 * @returns {boolean}
 */
export function detect(projectRoot) {
  return (
    existsSync(join(projectRoot, ".agents")) ||
    existsSync(join(projectRoot, ".agents", "hooks.json"))
  );
}

/**
 * @returns {string}
 */
export function configRelativePath() {
  return ".agents/hooks.json";
}

/**
 * @param {string} cliCommand base `node … hook antigravity`
 * @param {string} event
 * @returns {string}
 */
export function commandForEvent(cliCommand, event) {
  return `${cliCommand} ${event}`;
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
    ide: "antigravity",
    path: join(ctx.projectRoot, configRelativePath()),
    relativePath: configRelativePath(),
    events: [...ANTIGRAVITY_EVENTS],
    commandPreview: commandForEvent(ctx.cliCommand, "PreToolUse"),
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
  const block = {};

  for (const event of ANTIGRAVITY_EVENTS) {
    const command = commandForEvent(ctx.cliCommand, event);
    if (event === "PreToolUse" || event === "PostToolUse") {
      block[event] = [
        {
          matcher: "*",
          hooks: [{ command, timeout: ctx.timeout }],
        },
      ];
    } else {
      block[event] = [{ command, timeout: ctx.timeout }];
    }
  }

  root["tiny-engineer"] = block;
  return root;
}

/**
 * @param {string} projectRoot
 * @returns {import("../types.js").UninstallPlan}
 */
export function planUninstall(projectRoot) {
  const relativePath = configRelativePath();
  const path = join(projectRoot, relativePath);
  const existing = readConfig(path);
  const has = Boolean(existing && Object.prototype.hasOwnProperty.call(existing, "tiny-engineer"));
  return {
    ide: "antigravity",
    path,
    relativePath,
    removeCount: has ? 1 : 0,
    detail: has ? 'key "tiny-engineer"' : "nothing to remove",
  };
}

/**
 * @param {object | null} existing
 * @returns {object}
 */
export function applyUninstall(existing) {
  if (!existing || typeof existing !== "object") {
    return {};
  }
  /** @type {Record<string, unknown>} */
  const root = structuredClone(existing);
  delete root["tiny-engineer"];
  return root;
}
