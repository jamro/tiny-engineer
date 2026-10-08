import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

/**
 * Absolute path to this package's `bin/tiny-engineer.js`.
 * @returns {string}
 */
export function resolveCliBinPath() {
  const here = dirname(fileURLToPath(import.meta.url));
  return join(here, "..", "..", "bin", "tiny-engineer.js");
}

/**
 * Shell command that runs `hook <ide>` via the local bin.
 * @param {string} ideId
 * @param {string} [binPath]
 * @returns {string}
 */
export function hookCommandFor(ideId, binPath = resolveCliBinPath()) {
  return `node ${JSON.stringify(binPath)} hook ${ideId}`;
}
