import { closeSync, openSync } from "node:fs";

import { SerialPort } from "serialport";

import { getSerialPort } from "../../config/env.js";
import { cobsEncodeFrame, CobsFrameDecoder } from "./cobs.js";
import {
  clearPortCache,
  defaultCachePath,
  readPortCache,
  writePortCache,
} from "./port-cache.js";
import {
  packMessage,
  packStreamPayload,
  parseMessage,
  SerialMsgType,
  SerialStreamKind,
} from "./protocol.js";

const BAUD = 115200;
const DEFAULT_TIMEOUT_MS = 3000;

/**
 * macOS `/dev/tty.*` raises DTR on open. ESP32-C3 USB-Serial-JTAG treats that
 * as a bus reset, so the first framed command is dropped (or the chip reboots
 * through the 1s setup delay before the link is up). `/dev/cu.*` does not.
 * Linux `/dev/ttyACM0` has no dot and is left alone.
 * @param {string} path
 * @param {NodeJS.Platform} [platform]
 * @returns {string}
 */
export function hostCalloutPath(path, platform = process.platform) {
  if (platform === "darwin" && path.startsWith("/dev/tty.")) {
    return `/dev/cu.${path.slice("/dev/tty.".length)}`;
  }
  return path;
}

/**
 * macOS callout node to the tty sibling that configures USB serial.
 * `/dev/cu.usbmodemX` → `/dev/tty.usbmodemX`. Other paths have no sibling.
 * @param {string} path
 * @returns {string | null}
 */
export function darwinTtyPath(path) {
  if (typeof path !== "string" || !path.startsWith("/dev/cu.")) return null;
  return `/dev/tty.${path.slice("/dev/cu.".length)}`;
}

/**
 * After a Mac reboot the cu node opens, but IOKit has not configured the USB
 * serial interface. Opening the tty sibling fails with ENXIO, and that failure
 * is what configures the driver. A successful open raises DTR and can reset
 * the chip, so call this only after sync has already failed.
 * @param {string} cuPath
 * @param {{ openFile?: (path: string, flags: string) => number, closeFile?: (fd: number) => void }} [opts]
 */
export function kickDarwinTty(cuPath, { openFile = openSync, closeFile = closeSync } = {}) {
  const tty = darwinTtyPath(cuPath);
  if (!tty) return;
  try {
    const fd = openFile(tty, "r+");
    try {
      closeFile(fd);
    } catch {
      // The open already nudged the driver.
    }
  } catch {
    // ENXIO is the useful case.
  }
}

/**
 * @param {((path: string) => void) | null | undefined} kick
 * @returns {((path: string) => void) | null}
 */
function resolveKick(kick) {
  if (kick === null) return null;
  if (typeof kick === "function") return kick;
  if (process.platform === "darwin") return kickDarwinTty;
  return null;
}

/**
 * @param {number} ms
 * @returns {Promise<void>}
 */
function delay(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

/**
 * Handshake after open. The first frame can be discarded on USB bus reset,
 * and a DTR pulse reboots through setup() before serialLinkBegin().
 * @param {{ sync: (opts?: { timeoutMs?: number }) => Promise<Record<string, unknown>> }} client
 * @returns {Promise<Record<string, unknown>>}
 */
export async function syncAfterOpen(client) {
  const steps = [
    { waitMs: 0, timeoutMs: 400 },
    { waitMs: 250, timeoutMs: 700 },
    { waitMs: 600, timeoutMs: 1000 },
  ];
  /** @type {Error} */
  let lastError = new Error("serial request timed out (op=sync)");
  for (const step of steps) {
    if (step.waitMs) await delay(step.waitMs);
    try {
      return await client.sync({ timeoutMs: step.timeoutMs });
    } catch (err) {
      lastError = err instanceof Error ? err : new Error(String(err));
    }
  }
  throw lastError;
}

/**
 * Open and sync. On macOS a failed sync can mean the USB serial driver never
 * configured after boot. One tty kick, then one more sync. A healthy first
 * sync does not touch tty.
 * @param {{ portPath: string, open: () => Promise<void>, close: () => Promise<void>, sync: (opts?: { timeoutMs?: number }) => Promise<Record<string, unknown>> }} client
 * @param {{ kick?: ((path: string) => void) | null }} [opts]
 *   `undefined` kicks on darwin. `null` never retries. A function is the kick.
 * @returns {Promise<Record<string, unknown>>}
 */
export async function openSynced(client, { kick } = {}) {
  await client.open();
  try {
    return await syncAfterOpen(client);
  } catch (err) {
    const nudge = resolveKick(kick);
    if (!nudge) throw err;
    await client.close().catch(() => {});
    nudge(client.portPath);
    await client.open();
    return await syncAfterOpen(client);
  }
}

/**
 * @param {{ path?: string, manufacturer?: string, friendlyName?: string, vendorId?: string }} port
 * @returns {{ path: string, score: number, reasons: string[] } | null}
 */
export function scoreSerialPort(port) {
  const path = port.path;
  if (!path) return null;
  const hay = `${port.manufacturer ?? ""} ${port.friendlyName ?? ""} ${port.path}`.toLowerCase();
  /** @type {string[]} */
  const reasons = [];
  let score = 0;
  const vid = (port.vendorId ?? "").toLowerCase();
  if (vid === "303a") {
    score += 100;
    reasons.push("vid 303a");
  }
  if (/tiny.?engineer/.test(hay)) {
    score += 80;
    reasons.push("name tiny engineer");
  }
  if (/espressif|usb.?jtag|cdc/.test(hay)) {
    score += 40;
    reasons.push("cdc");
  }
  if (/usbserial|usbmodem|ttyacm|ttyusb|cu\./.test(hay)) {
    score += 10;
    reasons.push("serial path");
  }
  return { path, score, reasons };
}

/** VID, product name, or Espressif/CDC. A bare usbserial path scores 10. */
const LIKELY_ROBOT_SCORE = 40;

/**
 * @param {Awaited<ReturnType<typeof SerialPort.list>>} ports
 * @returns {Array<{ path: string, score: number, reasons: string[] }>}
 */
function rankedSerialPorts(ports) {
  return ports
    .map((port) => scoreSerialPort(port))
    .filter((row) => row && row.score > 0)
    .sort((a, b) => b.score - a.score);
}

/**
 * @param {Awaited<ReturnType<typeof SerialPort.list>>} ports
 * @returns {string[]}
 */
export function rankSerialPorts(ports) {
  return rankedSerialPorts(ports).map((row) => row.path);
}

/**
 * @param {{ list?: () => Promise<Awaited<ReturnType<typeof SerialPort.list>>> }} [deps]
 * @returns {Promise<string[]>}
 */
export async function listCandidatePorts({ list = () => SerialPort.list() } = {}) {
  const ports = await list();
  return rankSerialPorts(ports);
}

export class SerialRobotClient {
  /**
   * @param {string} portPath
   * @param {{ openPort?: (path: string) => import("serialport").SerialPort }} [deps]
   */
  constructor(portPath, { openPort } = {}) {
    this.portPath = hostCalloutPath(portPath);
    /** @type {import("serialport").SerialPort | null} */
    this._port = null;
    this._openPort = openPort;
    this._decoder = new CobsFrameDecoder();
    /** @type {Map<number, { resolve: (v: unknown) => void, reject: (e: Error) => void, timer: NodeJS.Timeout }>} */
    this._pending = new Map();
    this._nextId = 1;
    /** @type {((payload: Buffer) => void) | null} */
    this._onLog = null;
    /** @type {((payload: Buffer) => void) | null} */
    this._onEvent = null;
    /** @type {{ streamId: number, seq: number, resolve: (c: number) => void, reject: (e: Error) => void, timer: NodeJS.Timeout }[]} */
    this._creditWaiters = [];
    this._decoder.setHandler((payload) => this._onPayload(payload));
  }

  /**
   * @param {(payload: Buffer) => void} handler
   */
  setLogHandler(handler) {
    this._onLog = handler;
  }

  /**
   * @param {(payload: Buffer) => void} handler
   */
  setEventHandler(handler) {
    this._onEvent = handler;
  }

  /**
   * @param {() => void} handler
   */
  setCloseHandler(handler) {
    this._onClose = handler;
  }

  /**
   * @returns {Promise<void>}
   */
  async open() {
    if (this._port?.isOpen) return;

    const port =
      this._openPort?.(this.portPath) ??
      new SerialPort({
        path: this.portPath,
        baudRate: BAUD,
        autoOpen: false,
        // HUPCL drops DTR on close and reboots the C3 on the next open.
        hupcl: false,
      });

    await new Promise((resolve, reject) => {
      port.open((err) => (err ? reject(err) : resolve(undefined)));
    });

    port.on("data", (chunk) => {
      this._decoder.feed(chunk);
    });
    port.on("close", () => {
      this._onClose?.();
    });

    this._port = port;
  }

  /**
   * @returns {Promise<void>}
   */
  async close() {
    for (const [, pending] of this._pending) {
      clearTimeout(pending.timer);
      pending.reject(new Error("serial port closed"));
    }
    this._pending.clear();

    const port = this._port;
    this._port = null;
    if (!port) return;
    if (!port.isOpen) return;
    await new Promise((resolve) => {
      port.close(() => resolve(undefined));
    });
  }

  /**
   * @param {Buffer} payload
   */
  _onPayload(payload) {
    const msg = parseMessage(payload);
    if (!msg) return;
    if (msg.type === SerialMsgType.Log) {
      this._onLog?.(msg.payload);
      return;
    }
    if (msg.type === SerialMsgType.Evt) {
      this._onEvent?.(msg.payload);
      return;
    }
    if (msg.type === SerialMsgType.Stream && msg.payload.length >= 4) {
      const streamId = msg.payload[0];
      const kind = msg.payload[1];
      const seq = msg.payload.readUInt16LE(2);
      if (kind === SerialStreamKind.Credit && msg.payload.length >= 6) {
        const credits = msg.payload.readUInt16LE(4);
        const idx = this._creditWaiters.findIndex((w) => w.streamId === streamId && w.seq === seq);
        if (idx >= 0) {
          const [waiter] = this._creditWaiters.splice(idx, 1);
          clearTimeout(waiter.timer);
          waiter.resolve(credits);
        }
      } else if (kind === SerialStreamKind.Error) {
        const reason =
          msg.payload.length > 4
            ? msg.payload.subarray(4).toString("utf8")
            : "stream error";
        const idx = this._creditWaiters.findIndex((w) => w.streamId === streamId && w.seq === seq);
        if (idx >= 0) {
          const [waiter] = this._creditWaiters.splice(idx, 1);
          clearTimeout(waiter.timer);
          waiter.reject(new Error(`serial play rejected: ${reason}`));
        }
      }
      return;
    }
    if (msg.type !== SerialMsgType.Res && msg.type !== SerialMsgType.Sync) return;

    const pending = this._pending.get(msg.id);
    if (!pending) return;
    this._pending.delete(msg.id);
    clearTimeout(pending.timer);

    try {
      const text = msg.payload.toString("utf8");
      pending.resolve(text ? JSON.parse(text) : {});
    } catch (err) {
      pending.reject(err instanceof Error ? err : new Error(String(err)));
    }
  }

  /**
   * @param {string} op
   * @param {Record<string, unknown>} [fields]
   * @param {{ timeoutMs?: number }} [opts]
   * @returns {Promise<Record<string, unknown>>}
   */
  async request(op, fields = {}, { timeoutMs = DEFAULT_TIMEOUT_MS } = {}) {
    if (!this._port?.isOpen) {
      throw new Error("serial port is not open");
    }

    const id = this._nextId++ & 0xffff;
    if (id === 0) this._nextId = 1;

    const body = JSON.stringify({ op, ...fields });
    const message = packMessage(SerialMsgType.Req, id, body);
    const frame = cobsEncodeFrame(message);

    /** @type {Promise<Record<string, unknown>>} */
    const response = new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        this._pending.delete(id);
        reject(new Error(`serial request timed out (op=${op})`));
      }, timeoutMs);
      this._pending.set(id, {
        resolve: (v) => resolve(/** @type {Record<string, unknown>} */ (v)),
        reject,
        timer,
      });
    });

    await new Promise((resolve, reject) => {
      this._port.write(frame, (err) => (err ? reject(err) : resolve(undefined)));
    });

    return response;
  }

  /**
   * @param {{ timeoutMs?: number }} [opts]
   */
  async sync(opts = {}) {
    return this.request("sync", {}, opts);
  }

  /**
   * Wait for a Stream credit frame for |streamId| / |seq|.
   * @param {number} streamId
   * @param {number} seq
   * @param {number} [timeoutMs]
   * @returns {Promise<number>} credits
   */
  waitCredit(streamId, seq, timeoutMs = DEFAULT_TIMEOUT_MS) {
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        this._creditWaiters = this._creditWaiters.filter((w) => w !== waiter);
        reject(new Error("serial credit timed out"));
      }, timeoutMs);
      const waiter = { streamId, seq, resolve, reject, timer };
      this._creditWaiters.push(waiter);
    });
  }

  /**
   * Drop a pending credit waiter (e.g. after a failed port write) so its timeout
   * cannot become an unhandled rejection.
   * @param {number} streamId
   * @param {number} seq
   * @param {unknown} err
   */
  cancelCreditWait(streamId, seq, err) {
    const idx = this._creditWaiters.findIndex((w) => w.streamId === streamId && w.seq === seq);
    if (idx < 0) {
      return;
    }
    const waiter = this._creditWaiters[idx];
    this._creditWaiters.splice(idx, 1);
    clearTimeout(waiter.timer);
    waiter.reject(err instanceof Error ? err : new Error(String(err)));
  }

  /**
   * Stream a WAV play session (credit-windowed).
   * @param {Buffer | Uint8Array} wav
   * @param {string} [name]
   * @param {{ timeoutMs?: number }} [opts]
   * @returns {Promise<Record<string, unknown>>}
   */
  async play(wav, name = "talking", { timeoutMs = 10 * 60 * 1000 } = {}) {
    const start = await this.request("play", { name }, { timeoutMs: 5000 });
    if (start.ok === false) {
      throw new Error(typeof start.error === "string" ? start.error : "play failed");
    }
    const streamId = Number(start.stream_id);
    let credits = Number(start.credits) || 1024;
    let offset = 0;
    let seq = 1;
    const bytes = Buffer.from(wav);
    let ended = false;

    try {
      while (offset < bytes.length) {
        const chunkLen = Math.min(credits, bytes.length - offset, 1024);
        if (chunkLen <= 0) {
          throw new Error("serial play stalled (no credits)");
        }
        const chunk = bytes.subarray(offset, offset + chunkLen);
        const payload = packStreamPayload(streamId, SerialStreamKind.Data, seq, chunk);
        const message = packMessage(SerialMsgType.Stream, 0, payload);
        const frame = cobsEncodeFrame(message);
        credits = 0;
        const creditPromise = this.waitCredit(streamId, seq, 10_000);
        try {
          await new Promise((resolve, reject) => {
            this._port.write(frame, (err) => (err ? reject(err) : resolve(undefined)));
          });
        } catch (err) {
          this.cancelCreditWait(streamId, seq, err);
          creditPromise.catch(() => {});
          throw err;
        }
        credits = await creditPromise;
        offset += chunkLen;
        seq = (seq + 1) & 0xffff;
        if (seq === 0) seq = 1;
      }

      const result = await this.request("play_end", { stream_id: streamId }, { timeoutMs });
      ended = true;
      return result;
    } finally {
      // Best-effort close so firmware does not stay busy after credit/stream errors.
      if (!ended && this._port?.isOpen) {
        await this.request("play_end", { stream_id: streamId }, { timeoutMs: 5000 }).catch(() => {});
      }
    }
  }
}

/**
 * Resolve a serial port path: explicit > cache > autodetect (probe sync).
 * Invalidates cache when open/probe fails.
 *
 * @param {{
 *   port?: string | null,
 *   cachePath?: string,
 *   list?: () => Promise<Awaited<ReturnType<typeof SerialPort.list>>>,
 *   openClient?: (path: string) => SerialRobotClient,
 *   kick?: ((path: string) => void) | null,
 * }} [opts]
 * @returns {Promise<{ client: SerialRobotClient, port: string, source: "flag" | "env" | "cache" | "autodetect" }>}
 */
export async function connectSerialRobot({
  port,
  cachePath = defaultCachePath(),
  list = () => SerialPort.list(),
  openClient = (path) => new SerialRobotClient(path),
  kick,
} = {}) {
  const explicitRaw = port?.trim() || getSerialPort();
  const explicit = explicitRaw ? hostCalloutPath(explicitRaw) : explicitRaw;
  if (explicit) {
    const client = openClient(explicit);
    try {
      await openSynced(client, { kick });
      writePortCache(explicit, cachePath);
      return {
        client,
        port: explicit,
        source: port?.trim() ? "flag" : "env",
      };
    } catch (err) {
      await client.close().catch(() => {});
      clearPortCache(cachePath);
      throw err;
    }
  }

  const cachedRaw = readPortCache(cachePath);
  const cached = cachedRaw ? hostCalloutPath(cachedRaw) : cachedRaw;
  if (cached) {
    const client = openClient(cached);
    try {
      await openSynced(client, { kick });
      return { client, port: cached, source: "cache" };
    } catch {
      await client.close().catch(() => {});
      clearPortCache(cachePath);
    }
  }

  /** @type {{ path: string, score: number }[]} */
  const candidates = [];
  for (const row of rankedSerialPorts(await list())) {
    const path = hostCalloutPath(row.path);
    if (candidates.some((item) => item.path === path)) continue;
    candidates.push({ path, score: row.score });
  }
  if (candidates.length === 0) {
    throw new Error("no Tiny Engineer serial ports found (set --port or TINY_ENGINEER_PORT)");
  }

  /** @type {Error | null} */
  let lastError = null;
  for (const candidate of candidates) {
    const client = openClient(candidate.path);
    const allowKick = candidates.length === 1 || candidate.score >= LIKELY_ROBOT_SCORE;
    try {
      await openSynced(client, { kick: allowKick ? kick : null });
      writePortCache(candidate.path, cachePath);
      return { client, port: candidate.path, source: "autodetect" };
    } catch (err) {
      lastError = err instanceof Error ? err : new Error(String(err));
      await client.close().catch(() => {});
    }
  }

  throw lastError ?? new Error("failed to open a Tiny Engineer serial port");
}
