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
 * True if token looks like a serial device path (not a pose name).
 * @param {string} value
 * @returns {boolean}
 */
export function looksLikeSerialPath(value) {
  if (!value || value.startsWith("-")) return false;
  if (value === "auto") return false;
  const lower = value.toLowerCase();
  if (lower.startsWith("com") && /^com\d+/i.test(value)) return true;
  if (value.startsWith("/")) return true;
  if (lower.includes("cu.") || lower.includes("tty.")) return true;
  if (lower.includes("ttyacm") || lower.includes("ttyusb")) return true;
  return false;
}

/**
 * Parse `--serial` / `--serial=` into a path or the `"auto"` sentinel.
 * @param {string[]} argv
 * @param {number} i index of `--serial` or `--serial=…`
 * @returns {{ serial: string, nextIndex: number } | { error: string }}
 */
export function takeSerialFlag(argv, i) {
  const arg = argv[i];
  if (arg.startsWith("--serial=")) {
    const value = arg.slice("--serial=".length);
    if (!value || value === "auto") return { serial: "auto", nextIndex: i };
    return { serial: value, nextIndex: i };
  }

  // bare --serial
  const next = argv[i + 1];
  if (!next || next.startsWith("-")) {
    return { serial: "auto", nextIndex: i };
  }
  if (looksLikeSerialPath(next) || next === "auto") {
    return { serial: next === "auto" ? "auto" : next, nextIndex: i + 1 };
  }
  // Pose name or other positional — leave it for the caller.
  return { serial: "auto", nextIndex: i };
}

/**
 * Parse shared flags from argv. Flags may appear before or after positionals
 * (e.g. `hook cursor -q`). Non-flag tokens go to `rest` in order.
 * @param {string[]} argv
 * @returns {{ help: boolean, quiet: boolean, url?: string, serial?: string, rest: string[], error?: string }}
 */
export function parseCommonArgs(argv) {
  let help = false;
  let quiet = false;
  /** @type {string | undefined} */
  let url;
  /** @type {string | undefined} */
  let serial;
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
      if (!next) return { help, quiet, url, serial, rest, error: "--url requires a value" };
      url = next;
      continue;
    }
    if (arg.startsWith("--url=")) {
      url = arg.slice("--url=".length);
      if (!url) return { help, quiet, url, serial, rest, error: "--url requires a value" };
      continue;
    }
    if (arg === "--serial" || arg.startsWith("--serial=")) {
      const taken = takeSerialFlag(argv, i);
      if ("error" in taken) {
        return { help, quiet, url, serial, rest, error: taken.error };
      }
      serial = taken.serial;
      i = taken.nextIndex;
      continue;
    }
    if (arg.startsWith("-")) {
      return { help, quiet, url, serial, rest, error: `Unknown argument: ${arg}` };
    }
    rest.push(arg);
  }

  if (url && serial) {
    return {
      help,
      quiet,
      url,
      serial,
      rest,
      error: "use either --url or --serial, not both",
    };
  }

  return { help, quiet, url, serial, rest };
}

/**
 * @param {string[]} argv arguments after `play`
 * @returns {{ wavPath?: string, name?: string, url?: string, serial?: string, quiet: boolean, error?: string }}
 */
export function parsePlayArgs(argv) {
  /** @type {{ wavPath?: string, name?: string, url?: string, serial?: string, quiet: boolean }} */
  const opts = { quiet: false };

  for (let i = 0; i < argv.length; i++) {
    const arg = argv[i];

    if (isQuietFlag(arg)) {
      opts.quiet = true;
      continue;
    }
    if (arg === "--name") {
      const value = argv[++i];
      if (!value || value.startsWith("-")) return { quiet: opts.quiet, error: "--name requires a value" };
      opts.name = value;
      continue;
    }
    if (arg === "--url") {
      const value = argv[++i];
      if (!value || value.startsWith("-")) return { quiet: opts.quiet, error: "--url requires a value" };
      opts.url = value;
      continue;
    }
    if (arg.startsWith("--url=")) {
      opts.url = arg.slice("--url=".length);
      if (!opts.url) return { quiet: opts.quiet, error: "--url requires a value" };
      continue;
    }
    if (arg === "--serial" || arg.startsWith("--serial=")) {
      const taken = takeSerialFlag(argv, i);
      if ("error" in taken) return { quiet: opts.quiet, error: taken.error };
      opts.serial = taken.serial;
      i = taken.nextIndex;
      continue;
    }
    if (arg.startsWith("-") || opts.wavPath) {
      return { quiet: opts.quiet, error: `Unknown argument: ${arg}` };
    }
    opts.wavPath = arg;
  }

  if (opts.url && opts.serial) {
    return { quiet: opts.quiet, error: "use either --url or --serial, not both" };
  }
  if (!opts.wavPath) return { quiet: opts.quiet, error: "play needs a WAV file" };
  return opts;
}

/**
 * @param {string[]} argv arguments after `anim`
 * @returns {{ name?: string, url?: string, serial?: string, help: boolean, quiet: boolean, error?: string }}
 */
export function parseAnimArgs(argv) {
  let help = false;
  let quiet = false;
  /** @type {string | undefined} */
  let url;
  /** @type {string | undefined} */
  let serial;
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
    if (arg === "--serial" || arg.startsWith("--serial=")) {
      const taken = takeSerialFlag(argv, i);
      if ("error" in taken) return { help, quiet, error: taken.error };
      serial = taken.serial;
      i = taken.nextIndex;
      continue;
    }
    if (arg.startsWith("-")) return { help, quiet, error: `Unknown argument: ${arg}` };
    if (name) return { help, quiet, error: `Unexpected argument: ${arg}` };
    name = arg;
  }

  if (url && serial) {
    return { help, quiet, error: "use either --url or --serial, not both" };
  }
  if (!help && !name) return { help, quiet, error: "anim needs a pose name" };
  return { help, quiet, name, url, serial };
}
