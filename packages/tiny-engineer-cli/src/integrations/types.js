/**
 * @typedef {object} HookInput
 * @property {string} event Normalized event name (IDE-specific string is fine for now)
 * @property {string} [tool]
 * @property {string} [status]
 * @property {Record<string, unknown>} [meta]
 */

/**
 * @typedef {object} HookRespondContext
 * @property {boolean} animPosted
 * @property {string | null} anim
 * @property {HookInput} [input]
 */

/**
 * @typedef {object} AfterAnimContext
 * @property {string | null} anim
 * @property {HookInput} [input]
 * @property {string} [url]
 * @property {string} [transport]
 * @property {string | null} [port]
 */

/**
 * @typedef {object} InstallContext
 * @property {string} projectRoot
 * @property {string} cliCommand Base command ending with `hook <ide>` (no event suffix)
 * @property {number} timeout
 * @property {string} [url]
 * @property {"project" | "user"} [scope] `user` is Cursor `~/.cursor/hooks.json` via `--global`
 * @property {string} [home] Home directory for user-scope paths (tests)
 */

/**
 * @typedef {object} InstallPlan
 * @property {string} ide
 * @property {string} path Absolute config path
 * @property {string} relativePath
 * @property {string[]} events
 * @property {string} commandPreview Example command that will be written
 */

/**
 * @typedef {object} UninstallPlan
 * @property {string} ide
 * @property {string} path Absolute config path
 * @property {string} relativePath
 * @property {number} removeCount
 * @property {string} detail
 */

/**
 * Integration plugin contract. Each IDE implements this shape and registers in registry.js.
 *
 * @typedef {object} Integration
 * @property {string} id CLI id, e.g. "cursor"
 * @property {string} name Human label, e.g. "Cursor"
 * @property {boolean} [hardExitAfterPost]
 * @property {(argv: string[], stdinText: string) => HookInput | null} parseHook
 * @property {(input: HookInput) => string | null} mapToAnim
 * @property {(input: HookInput, style: import("../util/style.js").Style) => string[]} [describeFiles]
 * @property {(ctx: AfterAnimContext) => void | Promise<void>} [afterAnim]
 * @property {(ctx: HookRespondContext) => void} [respond]
 * @property {(projectRoot: string) => boolean} [detect]
 * @property {() => string} [configRelativePath]
 * @property {(ctx: InstallContext) => InstallPlan} [planInstall]
 * @property {(ctx: InstallContext, existing: object | null) => object} [applyInstall]
 * @property {(projectRoot: string, opts?: { scope?: "project" | "user", home?: string }) => UninstallPlan} [planUninstall]
 * @property {(existing: object | null) => object} [applyUninstall]
 */

export {};
