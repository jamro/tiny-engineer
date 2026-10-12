import { SerialPort } from "serialport";
import {
  clearSerialCache,
  readSerialCache,
  writeSerialCache,
} from "./serial-cache.js";
import { serialRequest } from "./serial.js";

/** Espressif USB VID (native USB-Serial/JTAG on C3/S3/…); serialport may return hex with/without 0x. */
export const ESPRESSIF_VID = "303A";
/** USB-Serial/JTAG function PID (ESP32-C3 / S3 / C6 / …). */
export const USB_SERIAL_JTAG_PID = "1001";
/** Health probe budget per candidate (Windows USB open can be slow). */
export const PROBE_TIMEOUT_MS = 800;

/**
 * @typedef {{
 *   path: string,
 *   manufacturer?: string,
 *   friendlyName?: string,
 *   vendorId?: string,
 *   productId?: string,
 * }} SerialPortInfo
 */

/**
 * @param {string | undefined} id
 * @returns {string}
 */
export function normalizeUsbId(id) {
  if (!id) return "";
  return id.replace(/^0x/i, "").toUpperCase();
}

/**
 * @param {string | undefined} vendorId
 * @returns {boolean}
 */
export function isEspressifVid(vendorId) {
  return normalizeUsbId(vendorId) === ESPRESSIF_VID;
}

/**
 * @param {SerialPortInfo} port
 * @returns {boolean}
 */
export function isEspressifDevice(port) {
  if (isEspressifVid(port.vendorId)) return true;
  const mfg = (port.manufacturer ?? "").toLowerCase();
  if (mfg.includes("espressif")) return true;
  const name = (port.friendlyName ?? "").toLowerCase();
  // Windows Device Manager style names
  if (name.includes("usb jtag") || name.includes("usb-serial/jtag")) return true;
  return false;
}

/**
 * macOS: call-out devices use cu.*; serialport often only lists tty.*.
 * Map tty.foo → cu.foo for opening. Linux /dev/ttyACM* is untouched (no "tty.").
 * @param {string} path
 * @returns {string}
 */
export function preferCalloutPath(path) {
  if (path.startsWith("/dev/tty.")) {
    return `/dev/cu.${path.slice("/dev/tty.".length)}`;
  }
  return path;
}

/**
 * Inverse of preferCalloutPath (macOS only).
 * @param {string} path
 * @returns {string | null}
 */
export function dialinTwinPath(path) {
  if (path.startsWith("/dev/cu.")) {
    return `/dev/tty.${path.slice("/dev/cu.".length)}`;
  }
  return null;
}

/**
 * @param {string} a
 * @param {string} b
 * @returns {boolean}
 */
export function serialPathsEqual(a, b) {
  return normalizePortPath(a) === normalizePortPath(b);
}

/**
 * Normalize Windows COM casing for stable cache keys.
 * @param {string} path
 * @returns {string}
 */
export function normalizePortPath(path) {
  const callout = preferCalloutPath(path);
  if (/^com\d+$/i.test(callout)) {
    return callout.toUpperCase();
  }
  return callout;
}

/**
 * Path looks like a USB CDC / ACM serial device (not Bluetooth / macOS debug).
 * @param {string} path
 * @returns {boolean}
 */
export function looksLikeCdcPath(path) {
  const p = path.toLowerCase();
  if (p.includes("bluetooth")) return false;
  if (p.includes("debug-console") || p.includes("wlan-debug")) return false;
  if (p.includes("bose") || p.includes("airpod") || p.includes("headset")) return false;
  // Windows: COM3, com12
  if (/^com\d+$/i.test(path)) return true;
  // macOS native CDC
  if (p.includes("usbmodem")) return true;
  // Linux CDC-ACM (ESP32-C3 native USB)
  if (/\/ttyacm\d*$/i.test(path) || p.includes("ttyacm")) return true;
  return false;
}

/**
 * Sort: Espressif USB-Serial/JTAG PID first, then other Espressif, then rest.
 * @param {SerialPortInfo[]} ports
 * @returns {SerialPortInfo[]}
 */
export function rankSerialCandidates(ports) {
  return [...ports].sort((a, b) => scorePort(b) - scorePort(a));
}

/**
 * @param {SerialPortInfo} port
 * @returns {number}
 */
function scorePort(port) {
  let score = 0;
  if (isEspressifDevice(port)) score += 100;
  if (normalizeUsbId(port.productId) === USB_SERIAL_JTAG_PID) score += 50;
  if (looksLikeCdcPath(port.path)) score += 10;
  return score;
}

/**
 * Prefer Espressif devices; else CDC-like paths.
 * Normalize macOS tty→cu, Windows COM casing; dedupe.
 * @param {SerialPortInfo[]} ports
 * @returns {SerialPortInfo[]}
 */
export function filterSerialCandidates(ports) {
  /** @type {Map<string, SerialPortInfo>} */
  const byPath = new Map();
  for (const p of ports) {
    const path = normalizePortPath(p.path);
    const prev = byPath.get(path);
    if (!prev || scorePort(p) > scorePort(prev)) {
      byPath.set(path, { ...p, path });
    }
  }
  const normalized = [...byPath.values()];
  const espressif = normalized.filter((p) => isEspressifDevice(p));
  const pool = espressif.length > 0 ? espressif : normalized.filter((p) => looksLikeCdcPath(p.path));
  return rankSerialCandidates(pool);
}

/**
 * @param {string} body
 * @returns {boolean}
 */
function healthBodyOk(body) {
  try {
    const json = JSON.parse(body);
    return json?.ok === true;
  } catch {
    return false;
  }
}

/**
 * @param {string} path
 * @param {typeof serialRequest} requestFn
 * @returns {Promise<boolean>}
 */
async function probeOnce(path, requestFn) {
  const result = await requestFn(path, {
    method: "GET",
    path: "/health",
    timeoutMs: PROBE_TIMEOUT_MS,
    clearCacheOnUnreachable: false,
  });
  return Boolean(result.ok && result.body && healthBodyOk(result.body));
}

/**
 * Probe preferred open path; on macOS fall back to tty twin if cu fails.
 * @param {string} path
 * @param {typeof serialRequest} [requestFn]
 * @returns {Promise<string | null>} matching open path, or null
 */
export async function probeTinyEngineerHealth(path, requestFn = serialRequest) {
  const primary = normalizePortPath(path);
  if (await probeOnce(primary, requestFn)) return primary;
  const twin = dialinTwinPath(primary);
  if (twin && (await probeOnce(twin, requestFn))) return twin;
  return null;
}

/**
 * @param {SerialPortInfo[]} ports
 * @param {typeof serialRequest} [requestFn]
 * @returns {Promise<string[]>}
 */
export async function probeMatchingPaths(ports, requestFn = serialRequest) {
  const results = await Promise.all(
    ports.map(async (p) => probeTinyEngineerHealth(p.path, requestFn)),
  );
  /** @type {string[]} */
  const matches = [];
  for (const path of results) {
    if (path && !matches.some((m) => serialPathsEqual(m, path))) {
      matches.push(path);
    }
  }
  return matches;
}

/**
 * @returns {Promise<SerialPortInfo[]>}
 */
export async function listSerialPortsDetailed() {
  const ports = await SerialPort.list();
  return ports.map((p) => ({
    path: p.path,
    manufacturer: p.manufacturer,
    friendlyName: p.friendlyName,
    vendorId: p.vendorId,
    productId: p.productId,
  }));
}

/**
 * Auto-detect a single Tiny Engineer serial port (cache-aware).
 * @param {{
 *   listFn?: () => Promise<SerialPortInfo[]>,
 *   requestFn?: typeof serialRequest,
 *   skipCache?: boolean,
 * }} [opts]
 * @returns {Promise<string>}
 */
export async function detectSerialPort(opts = {}) {
  const listFn = opts.listFn ?? listSerialPortsDetailed;
  const requestFn = opts.requestFn ?? serialRequest;

  if (!opts.skipCache) {
    const cached = readSerialCache();
    if (cached) {
      const ports = await listFn();
      const stillPresent = ports.some((p) => serialPathsEqual(p.path, cached));
      if (stillPresent) {
        const openPath = await probeTinyEngineerHealth(cached, requestFn);
        if (openPath) {
          writeSerialCache(openPath);
          return openPath;
        }
      }
      clearSerialCache();
    }
  }

  const ports = await listFn();
  const candidates = filterSerialCandidates(ports);
  const matches = await probeMatchingPaths(candidates, requestFn);

  if (matches.length === 1) {
    writeSerialCache(matches[0]);
    return matches[0];
  }

  if (matches.length === 0) {
    if (candidates.length > 0) {
      const listed = candidates.map((c) => c.path).join(", ");
      throw new Error(
        `no Tiny Engineer reply on ${listed} (close pio monitor / other serial apps, wait for boot, or pass --serial <path>)`,
      );
    }
    throw new Error(
      "no Tiny Engineer serial port found (try `tiny-engineer ports` or pass --serial <path>)",
    );
  }

  throw new Error(`multiple Tiny Engineer ports: ${matches.join(", ")}`);
}
