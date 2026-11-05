import { ensureDaemon } from "./ensure.js";
import { supervisorInstalled } from "./flags.js";
import { delay } from "./proc.js";

/**
 * @param {string} op
 * @param {Record<string, unknown>} fields
 * @param {{ port?: string | null, configDir?: string }} [opts]
 */
export async function daemonRobotRequest(op, fields = {}, opts = {}) {
  const conn = await ensureDaemon({ configDir: opts.configDir, port: opts.port ?? null });
  try {
    const res = await conn.peer.request(
      "robot.request",
      { op, fields, port: opts.port ?? null },
      15_000,
    );
    if (typeof res.error === "string") throw new Error(res.error);
    return /** @type {Record<string, unknown>} */ (res.result && /** @type {{ body?: Record<string, unknown> }} */ (res.result).body) ?? {};
  } finally {
    conn.socket.end();
  }
}

/**
 * @param {{ path: string, name?: string, port?: string | null, configDir?: string }} opts
 */
export async function daemonPlay(opts) {
  const conn = await ensureDaemon({ configDir: opts.configDir, port: opts.port ?? null });
  try {
    const res = await conn.peer.request(
      "play",
      { path: opts.path, name: opts.name ?? "talking", port: opts.port ?? null },
      11 * 60 * 1000,
    );
    if (typeof res.error === "string") throw new Error(res.error);
    return /** @type {Record<string, unknown>} */ (
      /** @type {{ body?: Record<string, unknown> }} */ (res.result)?.body ?? {}
    );
  } finally {
    conn.socket.end();
  }
}

/**
 * @param {{
 *   channel: "serial.events" | "serial.logs",
 *   port?: string | null,
 *   configDir?: string,
 *   onLine: (line: string) => void,
 *   onLinkDown?: () => void,
 *   onSubscribed?: () => void,
 * }} opts
 * @returns {Promise<"stop" | "replace" | "drop">}
 */
export async function subscribeOnce(opts) {
  const conn = await ensureDaemon({ configDir: opts.configDir, port: opts.port ?? null });
  const res = await conn.peer.request(
    "subscribe",
    { channel: opts.channel, port: opts.port ?? null },
    5000,
  );
  if (typeof res.error === "string") {
    conn.socket.destroy();
    throw new Error(res.error);
  }
  opts.onSubscribed?.();
  return new Promise((resolve) => {
    let settled = false;
    const finish = (reason) => {
      if (settled) return;
      settled = true;
      resolve(reason);
    };
    conn.socket.on("close", () => {
      setTimeout(() => finish("drop"), 30);
    });
    conn.socket.on("error", () => {
      setTimeout(() => finish("drop"), 30);
    });
    conn.peer.on((msg) => {
      if (msg.method === "event" && msg.params && /** @type {{ channel?: string }} */ (msg.params).channel === opts.channel) {
        const data = /** @type {{ data?: string }} */ (msg.params).data;
        if (typeof data === "string") opts.onLine(data);
      }
      if (msg.method === "link-down") opts.onLinkDown?.();
      if (msg.method === "bye") {
        const reason = /** @type {{ reason?: string }} */ (msg.params)?.reason;
        finish(reason === "replace" ? "replace" : "stop");
      }
    });
  });
}

/**
 * @param {{
 *   channel: "serial.events" | "serial.logs",
 *   port?: string | null,
 *   configDir?: string,
 *   onLine: (line: string) => void,
 *   onLinkDown?: () => void,
 *   onSubscribed?: () => void,
 *   subscribe?: (opts: {
 *     channel: "serial.events" | "serial.logs",
 *     port?: string | null,
 *     configDir?: string,
 *     onLine: (line: string) => void,
 *     onLinkDown?: () => void,
 *     onSubscribed?: () => void,
 *   }) => Promise<"stop" | "replace" | "drop">,
 * }} opts
 */
export async function followDaemonChannel(opts) {
  const subscribe = opts.subscribe ?? ((inner) => subscribeOnce(inner));
  let drops = 0;
  for (;;) {
    const reason = await subscribe({
      ...opts,
      onSubscribed() {
        drops = 0;
        opts.onSubscribed?.();
      },
    });
    if (reason === "stop") return;
    if (reason === "replace") {
      drops = 0;
      continue;
    }
    const supervised = Boolean(opts.configDir && supervisorInstalled(opts.configDir));
    if (supervised) {
      drops += 1;
      if (drops > 40) throw new Error("daemon connection lost");
      await delay(30);
      continue;
    }
    drops += 1;
    if (drops > 1) throw new Error("daemon connection lost");
  }
}
