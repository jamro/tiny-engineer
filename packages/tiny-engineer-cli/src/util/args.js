/**
 * @param {string} arg
 * @returns {boolean}
 */
export function isQuietFlag(arg) {
  return arg === "-q" || arg === "--quiet" || arg === "--silent";
}

/**
 * Strip leading global flags before the command name.
 * @param {string[]} argv
 * @returns {{ quiet: boolean, rest: string[], error?: string }}
 */
export function stripGlobalFlags(argv) {
  let quiet = false;
  let i = 0;

  while (i < argv.length) {
    const arg = argv[i];
    if (isQuietFlag(arg)) {
      quiet = true;
      i++;
      continue;
    }
    break;
  }

  return { quiet, rest: argv.slice(i) };
}

/**
 * Parse shared flags from argv. Flags may appear before or after positionals
 * (e.g. `hook cursor -q`). Non-flag tokens go to `rest` in order.
 * @param {string[]} argv
 * @returns {{
 *   help: boolean,
 *   quiet: boolean,
 *   url?: string,
 *   transport?: string,
 *   port?: string,
 *   rest: string[],
 *   error?: string,
 * }}
 */
export function parseCommonArgs(argv) {
  let help = false;
  let quiet = false;
  /** @type {string | undefined} */
  let url;
  /** @type {string | undefined} */
  let transport;
  /** @type {string | undefined} */
  let port;
  /** @type {string[]} */
  const rest = [];

  for (let i = 0; i < argv.length; i++) {
    const arg = argv[i];
    if (arg === "-h" || arg === "--help") {
      help = true;
      continue;
    }
    if (isQuietFlag(arg)) {
      quiet = true;
      continue;
    }
    if (arg === "--url") {
      const next = argv[++i];
      if (!next) return { help, quiet, url, transport, port, rest, error: "--url requires a value" };
      url = next;
      continue;
    }
    if (arg.startsWith("--url=")) {
      url = arg.slice("--url=".length);
      if (!url) return { help, quiet, url, transport, port, rest, error: "--url requires a value" };
      continue;
    }
    if (arg === "--transport") {
      const next = argv[++i];
      if (!next) {
        return { help, quiet, url, transport, port, rest, error: "--transport requires a value" };
      }
      transport = next;
      continue;
    }
    if (arg.startsWith("--transport=")) {
      transport = arg.slice("--transport=".length);
      if (!transport) {
        return { help, quiet, url, transport, port, rest, error: "--transport requires a value" };
      }
      continue;
    }
    if (arg === "--port") {
      const next = argv[++i];
      if (!next) return { help, quiet, url, transport, port, rest, error: "--port requires a value" };
      port = next;
      continue;
    }
    if (arg.startsWith("--port=")) {
      port = arg.slice("--port=".length);
      if (!port) return { help, quiet, url, transport, port, rest, error: "--port requires a value" };
      continue;
    }
    if (arg.startsWith("-")) {
      return { help, quiet, url, transport, port, rest, error: `Unknown argument: ${arg}` };
    }
    rest.push(arg);
  }

  return { help, quiet, url, transport, port, rest };
}

/**
 * @param {string[]} argv arguments after `play`
 * @returns {{
 *   wavPath?: string,
 *   name?: string,
 *   url?: string,
 *   transport?: string,
 *   port?: string,
 *   quiet: boolean,
 *   error?: string,
 * }}
 */
export function parsePlayArgs(argv) {
  /** @type {{
   *   wavPath?: string,
   *   name?: string,
   *   url?: string,
   *   transport?: string,
   *   port?: string,
   *   quiet: boolean,
   * }} */
  const opts = { quiet: false };

  for (let i = 0; i < argv.length; i++) {
    const arg = argv[i];

    if (isQuietFlag(arg)) {
      opts.quiet = true;
      continue;
    }
    if (arg === "--name" || arg === "--url" || arg === "--transport" || arg === "--port") {
      const value = argv[++i];
      if (!value || value.startsWith("-")) return { quiet: opts.quiet, error: `${arg} requires a value` };
      opts[/** @type {"name"|"url"|"transport"|"port"} */ (arg.slice(2))] = value;
      continue;
    }
    if (arg.startsWith("--url=")) {
      opts.url = arg.slice("--url=".length);
      if (!opts.url) return { quiet: opts.quiet, error: "--url requires a value" };
      continue;
    }
    if (arg.startsWith("--transport=")) {
      opts.transport = arg.slice("--transport=".length);
      if (!opts.transport) return { quiet: opts.quiet, error: "--transport requires a value" };
      continue;
    }
    if (arg.startsWith("--port=")) {
      opts.port = arg.slice("--port=".length);
      if (!opts.port) return { quiet: opts.quiet, error: "--port requires a value" };
      continue;
    }
    if (arg.startsWith("--name=")) {
      opts.name = arg.slice("--name=".length);
      if (!opts.name) return { quiet: opts.quiet, error: "--name requires a value" };
      continue;
    }
    if (arg.startsWith("-") || opts.wavPath) {
      return { quiet: opts.quiet, error: `Unknown argument: ${arg}` };
    }
    opts.wavPath = arg;
  }

  if (!opts.wavPath) return { quiet: opts.quiet, error: "play needs a WAV file" };
  return opts;
}

/**
 * @param {string[]} argv arguments after `anim`
 * @returns {{
 *   name?: string,
 *   url?: string,
 *   transport?: string,
 *   port?: string,
 *   help: boolean,
 *   quiet: boolean,
 *   error?: string,
 * }}
 */
export function parseAnimArgs(argv) {
  let help = false;
  let quiet = false;
  /** @type {string | undefined} */
  let url;
  /** @type {string | undefined} */
  let transport;
  /** @type {string | undefined} */
  let port;
  /** @type {string | undefined} */
  let name;

  for (let i = 0; i < argv.length; i++) {
    const arg = argv[i];
    if (arg === "-h" || arg === "--help") {
      help = true;
      continue;
    }
    if (isQuietFlag(arg)) {
      quiet = true;
      continue;
    }
    if (arg === "--url") {
      const next = argv[++i];
      if (!next) return { help, quiet, error: "--url requires a value" };
      url = next;
      continue;
    }
    if (arg.startsWith("--url=")) {
      url = arg.slice("--url=".length);
      if (!url) return { help, quiet, error: "--url requires a value" };
      continue;
    }
    if (arg === "--transport") {
      const next = argv[++i];
      if (!next) return { help, quiet, error: "--transport requires a value" };
      transport = next;
      continue;
    }
    if (arg.startsWith("--transport=")) {
      transport = arg.slice("--transport=".length);
      if (!transport) return { help, quiet, error: "--transport requires a value" };
      continue;
    }
    if (arg === "--port") {
      const next = argv[++i];
      if (!next) return { help, quiet, error: "--port requires a value" };
      port = next;
      continue;
    }
    if (arg.startsWith("--port=")) {
      port = arg.slice("--port=".length);
      if (!port) return { help, quiet, error: "--port requires a value" };
      continue;
    }
    if (arg.startsWith("-")) return { help, quiet, error: `Unknown argument: ${arg}` };
    if (name) return { help, quiet, error: `Unexpected argument: ${arg}` };
    name = arg;
  }

  if (!help && !name) return { help, quiet, error: "anim needs a pose name" };
  return { help, quiet, name, url, transport, port };
}
