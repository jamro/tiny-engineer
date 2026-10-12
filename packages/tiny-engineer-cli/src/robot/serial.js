import { SerialPort } from "serialport";
import { clearSerialCache } from "./serial-cache.js";

/** @typedef {import("./client.js").RobotResult} RobotResult */

/**
 * @param {RobotResult} result
 * @returns {boolean}
 */
function isUnreachable(result) {
  return Boolean(result.error?.startsWith("robot unreachable:"));
}

let nextId = 1;

/**
 * @param {Record<string, string | undefined>} query
 * @returns {Record<string, string>}
 */
function stringQuery(query) {
  /** @type {Record<string, string>} */
  const out = {};
  for (const [k, v] of Object.entries(query ?? {})) {
    if (v !== undefined) out[k] = String(v);
  }
  return out;
}

/**
 * Encode one serial API request line (no trailing newline).
 * @param {{ id?: number, method: string, path: string, query?: Record<string, string | undefined> }} opts
 */
export function encodeSerialRequest({ id, method, path, query }) {
  const reqId = id ?? nextId++;
  return {
    id: reqId,
    line: JSON.stringify({
      v: 1,
      id: reqId,
      op: `${method} ${path}`,
      q: stringQuery(query),
    }),
  };
}

/**
 * Encode serial POST /play begin line (JSON + len; body follows as raw bytes).
 * @param {{ id?: number, name?: string, len: number }} opts
 */
export function encodeSerialPlayBegin({ id, name, len }) {
  const reqId = id ?? nextId++;
  const path = name ? `/play/${name}` : "/play";
  return {
    id: reqId,
    line: JSON.stringify({
      v: 1,
      id: reqId,
      op: `POST ${path}`,
      len,
    }),
  };
}

/** Same ceiling as HTTP play; robot answers when the clip finishes. */
export const SERIAL_PLAY_TIMEOUT_MS = 10 * 60 * 1000;

/** 16-bit mono @ 22050 Hz ≈ bytes/sec of PCM. Match realtime; device has
 *  8 KiB CDC RX + 8 KiB software ring to absorb I2S.write blocking. */
const SERIAL_PLAY_BYTES_PER_SEC = 22050 * 2;
const SERIAL_PLAY_PACE = 1.0;
const SERIAL_PLAY_CHUNK = 2048;

/**
 * Write WAV body paced near realtime so ESP32 USB CDC RX does not overflow.
 * @param {import("serialport").SerialPort} port
 * @param {Uint8Array | Buffer} wav
 * @returns {Promise<void>}
 */
export function writeSerialPlayBody(port, wav) {
  const buf = Buffer.from(wav);
  const started = Date.now();

  /**
   * @param {number} offset
   * @returns {Promise<void>}
   */
  const writeNext = (offset) => {
    if (offset >= buf.length) return Promise.resolve();
    const end = Math.min(offset + SERIAL_PLAY_CHUNK, buf.length);
    const chunk = buf.subarray(offset, end);

    return new Promise((resolve, reject) => {
      port.write(chunk, (err) => {
        if (err) {
          reject(err);
          return;
        }
        port.drain((drainErr) => {
          if (drainErr) {
            reject(drainErr);
            return;
          }
          const elapsed = Date.now() - started;
          const targetMs = (end / (SERIAL_PLAY_BYTES_PER_SEC * SERIAL_PLAY_PACE)) * 1000;
          const wait = Math.max(0, Math.ceil(targetMs - elapsed));
          if (wait === 0) {
            writeNext(end).then(resolve, reject);
            return;
          }
          setTimeout(() => {
            writeNext(end).then(resolve, reject);
          }, wait);
        });
      });
    });
  };

  return writeNext(0);
}

/**
 * Parse an `R…` reply line into status + body.
 * @param {string} line
 * @returns {{ id?: number, status: number, body: unknown } | null}
 */
export function parseSerialReply(line) {
  const trimmed = line.trim();
  if (!trimmed.startsWith("R{")) return null;
  try {
    const json = JSON.parse(trimmed.slice(1));
    if (typeof json?.status !== "number") return null;
    return {
      id: typeof json.id === "number" ? json.id : undefined,
      status: json.status,
      body: json.body,
    };
  } catch {
    return null;
  }
}

/**
 * @param {unknown} body
 * @param {number} status
 * @returns {string}
 */
function errorFromBody(body, status) {
  if (body && typeof body === "object" && typeof body.error === "string" && body.error) {
    return body.error;
  }
  if (typeof body === "string" && body.trim()) return body.trim();
  return `HTTP ${status}`;
}

/**
 * @param {string} path
 * @param {{
 *   method: string,
 *   path: string,
 *   query?: Record<string, string | undefined>,
 *   timeoutMs?: number,
 *   clearCacheOnUnreachable?: boolean,
 * }} opts
 * @returns {Promise<RobotResult>}
 */
export async function serialRequest(path, opts) {
  const timeoutMs = opts.timeoutMs ?? 2000;
  const clearOnFail = opts.clearCacheOnUnreachable !== false;
  const { id, line } = encodeSerialRequest({
    method: opts.method,
    path: opts.path,
    query: opts.query,
  });

  return new Promise((resolve) => {
    /** @type {SerialPort | null} */
    let port = null;
    let settled = false;
    let buffer = "";

    const finish = (result) => {
      if (settled) return;
      settled = true;
      clearTimeout(timer);
      if (clearOnFail && isUnreachable(result)) {
        clearSerialCache();
      }
      if (port?.isOpen) {
        port.close(() => resolve(result));
      } else {
        resolve(result);
      }
    };

    const timer = setTimeout(() => {
      finish({ ok: false, error: `robot unreachable: serial timeout after ${timeoutMs}ms` });
    }, timeoutMs);

    try {
      port = new SerialPort({
        path,
        baudRate: 115200,
        autoOpen: false,
      });
    } catch (err) {
      clearTimeout(timer);
      resolve({
        ok: false,
        error: `robot unreachable: ${err instanceof Error ? err.message : String(err)}`,
      });
      return;
    }

    port.on("error", (err) => {
      finish({
        ok: false,
        error: `robot unreachable: ${err instanceof Error ? err.message : String(err)}`,
      });
    });

    port.on("data", (chunk) => {
      buffer += chunk.toString("utf8");
      let nl;
      while ((nl = buffer.indexOf("\n")) >= 0) {
        const raw = buffer.slice(0, nl);
        buffer = buffer.slice(nl + 1);
        const reply = parseSerialReply(raw);
        if (!reply) continue;
        if (reply.id !== undefined && reply.id !== id) continue;

        const bodyText =
          reply.body === undefined || reply.body === null
            ? ""
            : typeof reply.body === "string"
              ? reply.body
              : JSON.stringify(reply.body);

        if (reply.status >= 200 && reply.status < 300) {
          finish({ ok: true, status: reply.status, body: bodyText });
        } else {
          finish({
            ok: false,
            status: reply.status,
            body: bodyText,
            error: errorFromBody(reply.body, reply.status),
          });
        }
        return;
      }
    });

    port.open((err) => {
      if (err) {
        finish({
          ok: false,
          error: `robot unreachable: ${err.message}`,
        });
        return;
      }
      port.write(`${line}\n`, (writeErr) => {
        if (writeErr) {
          finish({
            ok: false,
            error: `robot unreachable: ${writeErr.message}`,
          });
        }
      });
    });
  });
}

/**
 * Stream a WAV to POST /play over serial (begin JSON + raw bytes + R reply).
 * @param {string} path
 * @param {{
 *   wav: Uint8Array | Buffer,
 *   name?: string,
 *   timeoutMs?: number,
 *   clearCacheOnUnreachable?: boolean,
 * }} opts
 * @returns {Promise<RobotResult>}
 */
export async function serialPlay(path, opts) {
  const timeoutMs = opts.timeoutMs ?? SERIAL_PLAY_TIMEOUT_MS;
  const clearOnFail = opts.clearCacheOnUnreachable !== false;
  const wav = opts.wav;
  const { id, line } = encodeSerialPlayBegin({
    name: opts.name,
    len: wav.byteLength,
  });

  return new Promise((resolve) => {
    /** @type {SerialPort | null} */
    let port = null;
    let settled = false;
    let buffer = "";

    const finish = (result) => {
      if (settled) return;
      settled = true;
      clearTimeout(timer);
      if (clearOnFail && isUnreachable(result)) {
        clearSerialCache();
      }
      if (port?.isOpen) {
        port.close(() => resolve(result));
      } else {
        resolve(result);
      }
    };

    const timer = setTimeout(() => {
      finish({ ok: false, error: `robot unreachable: serial timeout after ${timeoutMs}ms` });
    }, timeoutMs);

    try {
      port = new SerialPort({
        path,
        baudRate: 115200,
        autoOpen: false,
      });
    } catch (err) {
      clearTimeout(timer);
      resolve({
        ok: false,
        error: `robot unreachable: ${err instanceof Error ? err.message : String(err)}`,
      });
      return;
    }

    port.on("error", (err) => {
      finish({
        ok: false,
        error: `robot unreachable: ${err instanceof Error ? err.message : String(err)}`,
      });
    });

    port.on("data", (chunk) => {
      buffer += chunk.toString("utf8");
      let nl;
      while ((nl = buffer.indexOf("\n")) >= 0) {
        const raw = buffer.slice(0, nl);
        buffer = buffer.slice(nl + 1);
        const reply = parseSerialReply(raw);
        if (!reply) continue;
        if (reply.id !== undefined && reply.id !== id) continue;

        const bodyText =
          reply.body === undefined || reply.body === null
            ? ""
            : typeof reply.body === "string"
              ? reply.body
              : JSON.stringify(reply.body);

        if (reply.status >= 200 && reply.status < 300) {
          finish({ ok: true, status: reply.status, body: bodyText });
        } else {
          finish({
            ok: false,
            status: reply.status,
            body: bodyText,
            error: errorFromBody(reply.body, reply.status),
          });
        }
        return;
      }
    });

    port.open((err) => {
      if (err) {
        finish({
          ok: false,
          error: `robot unreachable: ${err.message}`,
        });
        return;
      }
      port.write(`${line}\n`, (writeErr) => {
        if (writeErr) {
          finish({
            ok: false,
            error: `robot unreachable: ${writeErr.message}`,
          });
          return;
        }
        writeSerialPlayBody(port, wav).catch((bodyErr) => {
          finish({
            ok: false,
            error: `robot unreachable: ${bodyErr instanceof Error ? bodyErr.message : String(bodyErr)}`,
          });
        });
      });
    });
  });
}

/**
 * @returns {Promise<{
 *   path: string,
 *   manufacturer?: string,
 *   friendlyName?: string,
 *   vendorId?: string,
 *   productId?: string,
 * }[]>}
 */
export async function listSerialPorts() {
  const ports = await SerialPort.list();
  return ports.map((p) => ({
    path: p.path,
    manufacturer: p.manufacturer,
    friendlyName: p.friendlyName,
    vendorId: p.vendorId,
    productId: p.productId,
  }));
}
