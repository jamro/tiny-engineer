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
 * @returns {{ help: boolean, quiet: boolean, url?: string, rest: string[], error?: string }}
 */
export function parseCommonArgs(argv) {
  let help = false;
  let quiet = false;
  /** @type {string | undefined} */
  let url;
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
      if (!next) return { help, quiet, url, rest, error: "--url requires a value" };
      url = next;
      continue;
    }
    if (arg.startsWith("--url=")) {
      url = arg.slice("--url=".length);
      if (!url) return { help, quiet, url, rest, error: "--url requires a value" };
      continue;
    }
    if (arg.startsWith("-")) {
      return { help, quiet, url, rest, error: `Unknown argument: ${arg}` };
    }
    rest.push(arg);
  }

  return { help, quiet, url, rest };
}

/**
 * @param {string[]} argv arguments after `play`
 * @returns {{ wavPath?: string, name?: string, url?: string, quiet: boolean, error?: string }}
 */
export function parsePlayArgs(argv) {
  /** @type {{ wavPath?: string, name?: string, url?: string, quiet: boolean }} */
  const opts = { quiet: false };

  for (let i = 0; i < argv.length; i++) {
    const arg = argv[i];

    if (isQuietFlag(arg)) {
      opts.quiet = true;
      continue;
    }
    if (arg === "--name" || arg === "--url") {
      const value = argv[++i];
      if (!value || value.startsWith("-")) return { quiet: opts.quiet, error: `${arg} requires a value` };
      opts[arg.slice(2)] = value;
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
 * @returns {{ name?: string, url?: string, help: boolean, quiet: boolean, error?: string }}
 */
export function parseAnimArgs(argv) {
  let help = false;
  let quiet = false;
  /** @type {string | undefined} */
  let url;
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
    if (arg.startsWith("-")) return { help, quiet, error: `Unknown argument: ${arg}` };
    if (name) return { help, quiet, error: `Unexpected argument: ${arg}` };
    name = arg;
  }

  if (!help && !name) return { help, quiet, error: "anim needs a pose name" };
  return { help, quiet, name, url };
}
