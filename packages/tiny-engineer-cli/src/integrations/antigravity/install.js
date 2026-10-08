import { existsSync } from "node:fs";
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
