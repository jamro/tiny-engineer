import { getBaseUrl, getHttpAuthToken, resolveTransport } from "../config/env.js";
import { connectSerialRobot } from "../robot/serial/client.js";
import { parseMessage, SerialMsgType } from "../robot/serial/protocol.js";
import { parseCommonArgs } from "../util/args.js";

/**
 * Build the outbound-events WebSocket URL from an HTTP base URL.
 * Device has no TLS on port 81 — `https:` bases are rejected.
 * @param {string} baseUrl
 * @param {string | null} [token]
 * @returns {string}
 */
export function eventsWebSocketUrl(baseUrl, token = null) {
  const http = new URL(baseUrl);
  if (http.protocol === "https:") {
    throw new Error(
      "watch HTTP transport requires an http: robot URL (device has no TLS on port 81)",
    );
  }
  if (http.protocol !== "http:") {
    throw new Error(`watch HTTP transport requires an http: robot URL, got ${http.protocol}`);
  }
  const ws = new URL(http.href);
  ws.protocol = "ws:";
  ws.port = "81";
  ws.pathname = "/events";
  ws.search = "";
  ws.hash = "";
  if (token) {
    ws.searchParams.set("token", token);
  }
  return ws.toString();
}

/**
 * Redact the token query for logs/errors.
 * @param {string} url
 * @returns {string}
 */
export function redactEventsUrl(url) {
  try {
    const parsed = new URL(url);
    if (parsed.searchParams.has("token")) {
      parsed.searchParams.set("token", "***");
    }
    return parsed.toString();
  } catch {
    return url.replace(/([?&]token=)[^&]*/i, "$1***");
  }
}

/**
 * @param {Buffer} message
 * @param {(line: string) => void} print
 * @returns {boolean}
 */
export function handleWatchSerialMessage(message, print) {
  const msg = parseMessage(message);
  if (!msg || msg.type !== SerialMsgType.Evt) return false;
  print(msg.payload.toString("utf8"));
  return true;
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 */
export async function runWatch(out, argv) {
  const opts = parseCommonArgs(argv);
  if (opts.error) {
    out.error(opts.error);
    process.exitCode = 1;
    return;
  }
  if (opts.help) {
    console.log(`Usage: tiny-engineer watch [options]

Print outbound device events as one JSON line each.
  --transport http|serial   Flag > TINY_ENGINEER_TRANSPORT > http
  --url <base>              HTTP base when transport is http
  --port <path>             Serial port when transport is serial

HTTP opens ws://<host>:81/events from an http: robot URL only
(token query when Bearer would be sent). https: is rejected.
Serial prints Evt frame payloads only.
`);
    return;
  }

  const resolved = resolveTransport(opts.transport);
  if (resolved.error) {
    out.error(resolved.error);
    process.exitCode = 1;
    return;
  }

  if (resolved.transport === "serial") {
    let session;
    try {
      session = await connectSerialRobot({ port: opts.port });
    } catch (err) {
      out.error(err instanceof Error ? err.message : String(err));
      process.exitCode = 1;
      return;
    }

    out.info(`watch serial ${session.port} (Ctrl+C to stop)`);
    session.client.setEventHandler((payload) => {
      console.log(payload.toString("utf8"));
    });
    return;
  }

  const baseUrl = opts.url ?? getBaseUrl();
  const token = getHttpAuthToken({ urlOverride: opts.url !== undefined });
  let wsUrl;
  try {
    wsUrl = eventsWebSocketUrl(baseUrl, token);
  } catch (err) {
    out.error(err instanceof Error ? err.message : String(err));
    process.exitCode = 1;
    return;
  }

  let WebSocket;
  try {
    ({ default: WebSocket } = await import("ws"));
  } catch {
    out.error("watch requires the ws package");
    process.exitCode = 1;
    return;
  }

  await new Promise((resolve) => {
    const socket = new WebSocket(wsUrl, { handshakeTimeout: 2000 });

    socket.on("open", () => {
      out.info(`watch ${redactEventsUrl(wsUrl)} (Ctrl+C to stop)`);
    });

    socket.on("message", (data) => {
      const text = Buffer.isBuffer(data) ? data.toString("utf8") : String(data);
      console.log(text);
    });

    socket.on("error", (err) => {
      out.error(err instanceof Error ? err.message : String(err));
      process.exitCode = 1;
      resolve(undefined);
    });

    socket.on("close", () => {
      process.exitCode = 1;
      resolve(undefined);
    });
  });
}
