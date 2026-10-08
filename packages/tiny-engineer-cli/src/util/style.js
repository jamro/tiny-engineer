/**
 * @typedef {{
 *   green: (s: string) => string,
 *   red: (s: string) => string,
 *   cyan: (s: string) => string,
 *   dim: (s: string) => string,
 *   bold: (s: string) => string,
 *   enabled: boolean,
 * }} Style
 */

/**
 * @param {NodeJS.WriteStream} [stream]
 * @returns {boolean}
 */
export function detectColor(stream = process.stdout) {
  if (process.env.NO_COLOR !== undefined) return false;
  if (process.env.TERM === "dumb") return false;
  return Boolean(stream?.isTTY);
}

/**
 * @param {{ color?: boolean }} [opts]
 * @returns {Style}
 */
export function createStyle({ color } = {}) {
  const enabled = color ?? detectColor();

  /**
   * @param {number} code
   * @returns {(s: string) => string}
   */
  function wrap(code) {
    if (!enabled) return (s) => s;
    return (s) => `\x1b[${code}m${s}\x1b[0m`;
  }

  return {
    enabled,
    green: wrap(32),
    red: wrap(31),
    cyan: wrap(36),
    dim: wrap(2),
    bold: wrap(1),
  };
}
