import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

/** Default hook command for any project (npm registry). */
export const NPX_CLI = "npx -y tiny-engineer";

/**
 * Absolute path to this package's `bin/tiny-engineer.js` (local checkout / CLI development).
 * @returns {string}
 */
export function resolveCliBinPath() {
  const here = dirname(fileURLToPath(import.meta.url));
  return join(here, "..", "..", "bin", "tiny-engineer.js");
}

/**
 * Shell command that runs `hook <ide>`.
 * Default: `npx -y tiny-engineer hook <ide>` (published package).
 * Pass `binPath` for a local `node …/tiny-engineer.js hook <ide>` override.
 * @param {string} ideId
 * @param {string} [binPath]
 * @returns {string}
 */
export function hookCommandFor(ideId, binPath) {
  if (binPath) {
    return `node ${JSON.stringify(binPath)} hook ${ideId}`;
  }
  return `${NPX_CLI} hook ${ideId}`;
}
