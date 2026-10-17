import { resolveTransport } from "../config/env.js";
import { connectSerialRobot } from "../robot/serial/client.js";
import { parseMessage, SerialMsgType } from "../robot/serial/protocol.js";
import { parseCommonArgs } from "../util/args.js";

/**
 * Decode LOG frame payload JSON `{"t":"..."}` to text.
 * @param {Buffer} payload
 * @returns {string | null}
 */
export function logTextFromPayload(payload) {
  try {
    const json = JSON.parse(payload.toString("utf8"));
    if (typeof json?.t === "string") return json.t;
  } catch {
    // ignore
  }
  return null;
}

/**
 * Process a decoded application message; print LOG text lines.
 * @param {Buffer} message
 * @param {(line: string) => void} print
 * @returns {boolean} true if a log line was printed
 */
export function handleMonitorMessage(message, print) {
  const msg = parseMessage(message);
  if (!msg || msg.type !== SerialMsgType.Log) return false;
  const text = logTextFromPayload(msg.payload);
  if (text === null) return false;
  print(text);
  return true;
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 */
export async function runMonitor(out, argv) {
  const opts = parseCommonArgs(argv);
  if (opts.error) {
    out.error(opts.error);
    process.exitCode = 1;
    return;
  }
  if (opts.help) {
    console.log(`Usage: tiny-engineer monitor [--port <path>] [--transport serial]

Print application LOG frames from the USB serial protocol.
Exclusive use of the port (close PlatformIO monitor first).
`);
    return;
  }

  const resolved = resolveTransport(opts.transport ?? "serial");
  if (resolved.error) {
    out.error(resolved.error);
    process.exitCode = 1;
    return;
  }
  if (resolved.transport !== "serial") {
    out.error("monitor requires serial transport");
    process.exitCode = 1;
    return;
  }

  let session;
  try {
    session = await connectSerialRobot({ port: opts.port });
  } catch (err) {
    out.error(err instanceof Error ? err.message : String(err));
    process.exitCode = 1;
    return;
  }

  out.info(`monitor ${session.port} (Ctrl+C to stop)`);

  session.client.setLogHandler((payload) => {
    const text = logTextFromPayload(payload);
    if (text !== null) console.log(text);
  });

  process.once("SIGINT", () => {
    session.client.close().finally(() => process.exit(0));
  });
}
