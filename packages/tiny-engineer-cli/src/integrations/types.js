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
 * @typedef {object} InstallContext
 * @property {string} projectRoot
 * @property {string} cliCommand Base command ending with `hook <ide>` (no event suffix)
 * @property {number} timeout
 * @property {string} [url]
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
 * Integration plugin contract. Each IDE implements this shape and registers in registry.js.
 *
 * @typedef {object} Integration
 * @property {string} id CLI id, e.g. "cursor"
 * @property {string} name Human label, e.g. "Cursor"
 * @property {boolean} [hardExitAfterPost]
 * @property {(argv: string[], stdinText: string) => HookInput | null} parseHook
 * @property {(input: HookInput) => string | null} mapToAnim
 * @property {(input: HookInput, style: import("../util/style.js").Style) => string[]} [describeFiles]
 * @property {(ctx: HookRespondContext) => void} [respond]
 * @property {(projectRoot: string) => boolean} [detect]
 * @property {() => string} [configRelativePath]
 * @property {(ctx: InstallContext) => InstallPlan} [planInstall]
 * @property {(ctx: InstallContext, existing: object | null) => object} [applyInstall]
 */

export {};
