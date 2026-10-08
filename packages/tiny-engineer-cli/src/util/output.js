import { createStyle, detectColor } from "./style.js";

/**
 * @typedef {import("./style.js").Style} Style
 *
 * @typedef {{
 *   ok: (message: string) => void,
 *   info: (message: string) => void,
 *   error: (message: string) => void,
 *   style: Style,
 *   quiet: boolean,
 * }} Output
 */

/**
 * @param {{
 *   quiet?: boolean,
 *   style?: Style,
 *   log?: (...args: unknown[]) => void,
 *   error?: (...args: unknown[]) => void,
 * }} [opts]
 * @returns {Output}
 */
export function createOutput({
  quiet = false,
  style = createStyle({ color: detectColor(process.stdout) }),
  log = console.log.bind(console),
  error = console.error.bind(console),
} = {}) {
  return {
    quiet,
    style,
    ok(message) {
      if (!quiet) log(`${style.green("✓")}  ${message}`);
    },
    info(message) {
      if (!quiet) error(message);
    },
    error(message) {
      error(`${style.red("✗")}  error: ${message}`);
    },
  };
}
