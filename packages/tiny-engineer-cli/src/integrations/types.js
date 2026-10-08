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
 */

/**
 * Integration plugin contract. Each IDE implements this shape and registers in registry.js.
 *
 * @typedef {object} Integration
 * @property {string} id CLI id, e.g. "cursor"
 * @property {string} name Human label, e.g. "Cursor"
 * @property {(argv: string[], stdinText: string) => HookInput | null} parseHook
 *   Normalize IDE hook argv + stdin into HookInput, or null to skip.
 * @property {(input: HookInput) => string | null} mapToAnim
 *   Map HookInput to a robot animation name, or null to skip the POST.
 * @property {(input: HookInput, style: import("../util/style.js").Style) => string[]} [describeFiles]
 *   Human-readable file modification lines for stderr (hook mode).
 * @property {(ctx: HookRespondContext) => void} [respond]
 *   IDE stdout / decision contract after the POST. Cursor: no-op (silence).
 */

export {};
