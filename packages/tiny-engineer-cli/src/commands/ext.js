import { robotExt } from "../robot/ops.js";
import { formatRobotError } from "../robot/transport.js";
import { parseCommonArgs } from "../util/args.js";
import { createOutput } from "../util/output.js";

/** @type {ReadonlySet<string>} */
export const KNOWN_EXTENSION_IDS = new Set(["lcd1602"]);

/**
 * @param {string[]} argv
 */
function parseExtArgs(argv) {
  const common = parseCommonArgs(argv, { passthroughUnknown: true });
  return { ...common, restPos: common.rest };
}

/**
 * @param {string[]} tokens
 * @returns {Record<string, string>}
 */
function parseFlags(tokens) {
  /** @type {Record<string, string>} */
  const flags = {};
  for (const token of tokens) {
    if (!token.startsWith("--") || !token.includes("=")) {
      continue;
    }
    const eq = token.indexOf("=");
    flags[token.slice(2, eq)] = token.slice(eq + 1);
  }
  return flags;
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 * @param {boolean} quietGlobal
 */
export async function runExt(out, argv, quietGlobal) {
  const opts = parseExtArgs(argv);
  const quiet = quietGlobal || opts.quiet;
  const localOut = quiet === out.quiet ? out : createOutput({ quiet, style: out.style });

  if (opts.error) {
    localOut.error(opts.error);
    process.exitCode = 1;
    return;
  }

  if (opts.help) {
    console.log(`Usage: tiny-engineer ext <id> <resource> <action> [options]

Extensions (v1):
  lcd1602 text get
  lcd1602 text set --line1=TEXT [--align=left|right|center] [--line2=TEXT] [--align2=...]
  lcd1602 text clear
  lcd1602 brightness get
  lcd1602 brightness set <0-100>

Enable the module with: tiny-engineer settings set --lcd1602=true

Options:
  --transport <http|serial>  --url <base>  --port <path>
`);
    return;
  }

  const [id, resource, action, ...rest] = opts.restPos ?? [];
  if (!id) {
    localOut.error("usage: tiny-engineer ext <id> <resource> <action>");
    process.exitCode = 1;
    return;
  }

  if (!KNOWN_EXTENSION_IDS.has(id)) {
    localOut.error(`unknown extension: ${id}`);
    process.exitCode = 1;
    return;
  }

  const transportOpts = { url: opts.url, transport: opts.transport, port: opts.port };

  if (id === "lcd1602") {
    await runLcd1602(localOut, resource, action, rest, transportOpts);
    return;
  }

  localOut.error(`unknown extension: ${id}`);
  process.exitCode = 1;
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string | undefined} resource
 * @param {string | undefined} action
 * @param {string[]} rest
 * @param {{ url?: string, transport?: string, port?: string | null }} transportOpts
 */
async function runLcd1602(out, resource, action, rest, transportOpts) {
  if (resource === "text") {
    if (action === "get") {
      const result = await robotExt(
        { id: "lcd1602", suffix: "text", method: "GET" },
        transportOpts,
      );
      if (!result.ok) {
        out.error(formatRobotError(result));
        process.exitCode = 1;
        return;
      }
      console.log(JSON.stringify(result.bodyJson ?? {}, null, 2));
      process.exitCode = 0;
      return;
    }

    if (action === "clear") {
      const result = await robotExt(
        { id: "lcd1602", suffix: "text", method: "DELETE" },
        transportOpts,
      );
      if (!result.ok) {
        out.error(formatRobotError(result));
        process.exitCode = 1;
        return;
      }
      out.ok("lcd text cleared");
      process.exitCode = 0;
      return;
    }

    if (action === "set") {
      const flags = parseFlags(rest);
      /** @type {Record<string, unknown>} */
      const body = {};
      if (flags.line1 !== undefined) {
        body.line1 = { text: flags.line1, ...(flags.align ? { align: flags.align } : {}) };
      }
      if (flags.line2 !== undefined) {
        body.line2 = {
          text: flags.line2,
          ...(flags.align2 ? { align: flags.align2 } : {}),
        };
      }
      if (!body.line1 && !body.line2) {
        out.error("text set needs --line1=... and/or --line2=...");
        process.exitCode = 1;
        return;
      }
      const result = await robotExt(
        { id: "lcd1602", suffix: "text", method: "POST", body },
        transportOpts,
      );
      if (!result.ok) {
        out.error(formatRobotError(result));
        process.exitCode = 1;
        return;
      }
      console.log(JSON.stringify(result.bodyJson ?? {}, null, 2));
      process.exitCode = 0;
      return;
    }

    out.error("lcd1602 text actions: get | set | clear");
    process.exitCode = 1;
    return;
  }

  if (resource === "brightness") {
    if (action === "get") {
      const result = await robotExt(
        { id: "lcd1602", suffix: "brightness", method: "GET" },
        transportOpts,
      );
      if (!result.ok) {
        out.error(formatRobotError(result));
        process.exitCode = 1;
        return;
      }
      console.log(JSON.stringify(result.bodyJson ?? {}, null, 2));
      process.exitCode = 0;
      return;
    }

    if (action === "set") {
      const valueToken = rest.find((t) => !t.startsWith("--"));
      const n = valueToken !== undefined ? Number(valueToken) : NaN;
      if (!Number.isInteger(n) || n < 0 || n > 100) {
        out.error("brightness set needs an integer 0–100");
        process.exitCode = 1;
        return;
      }
      const result = await robotExt(
        { id: "lcd1602", suffix: "brightness", method: "POST", body: { brightness: n } },
        transportOpts,
      );
      if (!result.ok) {
        out.error(formatRobotError(result));
        process.exitCode = 1;
        return;
      }
      console.log(JSON.stringify(result.bodyJson ?? {}, null, 2));
      process.exitCode = 0;
      return;
    }

    out.error("lcd1602 brightness actions: get | set");
    process.exitCode = 1;
    return;
  }

  out.error("lcd1602 resources: text | brightness");
  process.exitCode = 1;
}
