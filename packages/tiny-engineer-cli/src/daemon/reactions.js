import { existsSync, mkdirSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { join } from "node:path";

import {
  DEFAULT_TRANSPORT,
  DEFAULT_URL,
  PORT_KEY,
  TOKEN_KEY,
  TRANSPORT_KEY,
  URL_KEY,
  normalizeTransport,
  parseDotEnvLine,
} from "../config/env.js";
import { eventsWebSocketUrl, redactEventsUrl } from "../robot/events-url.js";
import { createActionRunner } from "./actions.js";
import { appendLog } from "./log.js";
import { daemonPaths } from "./paths.js";
import { createRuleStore, matchRules } from "./rules.js";

/**
 * User-config connection for the reaction link. Does not write process.env.
 * Duplicate keys use first wins, matching the dotenv loader.
 * @param {string} configDir
 */
export function readUserConnection(configDir) {
  /** @type {{ transport: "http" | "serial", url: string, token: string | null, port: string | null }} */
  const found = { transport: DEFAULT_TRANSPORT, url: DEFAULT_URL, token: null, port: null };
  /** @type {Set<string>} */
  const seen = new Set();
  let raw = "";
  try {
    raw = readFileSync(join(configDir, "env"), "utf8");
  } catch {
    return found;
  }
  for (const line of raw.split(/\r?\n/)) {
    const parsed = parseDotEnvLine(line);
    if (!parsed || seen.has(parsed[0])) continue;
    seen.add(parsed[0]);
    const [key, value] = parsed;
    if (key === TRANSPORT_KEY) {
      const normalized = normalizeTransport(value);
      if (normalized) found.transport = normalized;
    } else if (key === URL_KEY && value.trim()) {
      found.url = value.trim().replace(/\/+$/, "");
    } else if (key === TOKEN_KEY && value.trim()) {
      found.token = value.trim();
    } else if (key === PORT_KEY && value.trim()) {
      found.port = value.trim();
    }
  }
  return found;
}

/**
 * @param {string} configDir
 * @param {number} [now]
 */
function yieldIsActive(configDir, now = Date.now()) {
  try {
    const until = JSON.parse(readFileSync(daemonPaths(configDir).yieldFile, "utf8")).until;
    return typeof until === "number" && until > now;
  } catch {
    return false;
  }
}

/**
 * @param {string} configDir
 */
export function writeReactionsSuspended(configDir) {
  const paths = daemonPaths(configDir);
  mkdirSync(paths.dir, { recursive: true, mode: 0o700 });
  writeFileSync(paths.reactionsSuspended, "{}\n", { mode: 0o600 });
}

/**
 * @param {string} configDir
 */
export function clearReactionsSuspended(configDir) {
  rmSync(daemonPaths(configDir).reactionsSuspended, { force: true });
}

/**
 * @param {string} configDir
 */
export function reactionsSuspended(configDir) {
  return existsSync(daemonPaths(configDir).reactionsSuspended);
}

/**
 * @param {{
 *   configDir: string,
 *   link: {
 *     port: string | null,
 *     ensure: (port?: string | null) => Promise<unknown>,
     *     setReactionHandler: (fn: ((line: string) => void) | null) => void,
     *     onLinkDown?: (fn: (reason: "drop" | "release") => void) => () => void,
     *   },
 *   logPath?: string,
 *   runner?: ReturnType<typeof createActionRunner>,
 *   readConnection?: () => ReturnType<typeof readUserConnection>,
 *   openWebSocket?: (url: string) => Promise<{
 *     on: (ev: string, fn: (data?: unknown) => void) => void,
 *     close?: () => void,
 *     terminate?: () => void,
 *   }>,
 *   retryMs?: number,
 *   now?: () => number,
 * }} opts
 */
export function createReactions(opts) {
  const paths = daemonPaths(opts.configDir);
  const logPath = opts.logPath ?? paths.log;
  const store = createRuleStore();
  const runner = opts.runner ?? createActionRunner({
    onOutcome: (action) => {
      if (action.kind === "none") return;
      const code = action.kind === "exit" && action.code !== undefined ? ` ${action.code}` : "";
      appendLog(logPath, `reaction type=${action.type} program=${action.program} ${action.kind}${code}`);
    },
  });
  const readConnection = opts.readConnection ?? (() => readUserConnection(opts.configDir));
  const retryMs = opts.retryMs ?? 500;
  const now = opts.now ?? Date.now;

  /** @type {"off" | "up" | "retrying" | "suspended" | "error"} */
  let linkState = "off";
  let detail = "";
  /** @type {"http" | "serial"} */
  let transport = DEFAULT_TRANSPORT;
  let serialWanted = false;
  let stopped = false;
  let reconciling = false;
  let again = false;
  let epoch = 0;
  /** @type {ReturnType<typeof setTimeout> | null} */
  let retryTimer = null;
  /** @type {Awaited<ReturnType<NonNullable<typeof opts.openWebSocket>>> | null} */
  let ws = null;
  /** @type {Promise<void> | null} */
  let opening = null;
  const offDown = opts.link.onLinkDown?.((reason) => {
    if (stopped || reason !== "drop" || !serialWanted) return;
    linkState = "retrying";
    scheduleRetry(epoch);
  });

  function log(line) {
    appendLog(logPath, line);
  }

  function loadRules() {
    try {
      store.loadText(readFileSync(paths.rules, "utf8"));
    } catch {
      store.loadMissing();
    }
  }

  function serialBlocked() {
    return reactionsSuspended(opts.configDir) || yieldIsActive(opts.configDir, now());
  }

  /**
   * @param {string} line
   */
  function handleLine(line) {
    let event;
    try {
      event = JSON.parse(line);
    } catch {
      return;
    }
    for (const rule of matchRules(store.rules, event)) {
      runner.start(rule, event);
    }
  }

  function detachSerial() {
    serialWanted = false;
    opts.link.setReactionHandler(null);
  }

  function clearRetry() {
    if (retryTimer) clearTimeout(retryTimer);
    retryTimer = null;
  }

  function closeWs() {
    epoch += 1;
    clearRetry();
    const current = ws;
    ws = null;
    if (!current) return;
    current.close?.();
    current.terminate?.();
  }

  /**
   * @param {number} gen
   */
  function scheduleRetry(gen) {
    if (stopped || gen !== epoch) return;
    clearRetry();
    retryTimer = setTimeout(() => {
      if (gen !== epoch || stopped) return;
      void reconcile();
    }, retryMs);
  }

  /**
   * @param {ReturnType<typeof readUserConnection>} conn
   */
  async function openHttp(conn) {
    const gen = epoch;
    let url;
    try {
      url = eventsWebSocketUrl(conn.url, conn.token);
    } catch (err) {
      linkState = "error";
      detail = err instanceof Error ? err.message : String(err);
      log(`reaction link ${detail}`);
      return;
    }
    const opener = opts.openWebSocket ?? defaultOpenWebSocket;
    linkState = "retrying";
    detail = redactEventsUrl(url);
    let socket;
    try {
      socket = await opener(url);
    } catch (err) {
      const message = err instanceof Error ? err.message : String(err);
      log(`reaction link ${message}`);
      linkState = "retrying";
      scheduleRetry(gen);
      return;
    }
    if (stopped || gen !== epoch) {
      socket.close?.();
      socket.terminate?.();
      return;
    }
    ws = socket;
    socket.on("open", () => {
      if (gen !== epoch) return;
      linkState = "up";
      detail = redactEventsUrl(url);
    });
    socket.on("message", (data) => {
      if (gen !== epoch) return;
      const text = Buffer.isBuffer(data) ? data.toString("utf8") : String(data ?? "");
      handleLine(text);
    });
    socket.on("close", () => {
      if (gen !== epoch) return;
      ws = null;
      linkState = "retrying";
      scheduleRetry(gen);
    });
    socket.on("error", () => {});
  }

  async function openSerial(port) {
    if (opts.link.port) {
      linkState = "up";
      detail = opts.link.port;
      return;
    }
    if (opening) return;
    linkState = "retrying";
    const gen = epoch;
    opening = opts.link.ensure(port).then(() => {
      if (stopped || gen !== epoch || !serialWanted || serialBlocked()) return;
      linkState = "up";
      detail = opts.link.port ?? "";
    }).catch((err) => {
      const message = err instanceof Error ? err.message : String(err);
      log(`reaction link ${message}`);
      linkState = "retrying";
      scheduleRetry(gen);
    }).finally(() => {
      opening = null;
    });
    await opening;
  }

  async function reconcileOnce() {
    loadRules();
    const conn = readConnection();
    transport = conn.transport;
    if (store.rules.length === 0) {
      closeWs();
      detachSerial();
      linkState = "off";
      detail = "";
      return;
    }
    if (transport === "http") {
      detachSerial();
      if (!ws) await openHttp(conn);
      return;
    }
    closeWs();
    if (serialBlocked()) {
      detachSerial();
      linkState = "suspended";
      detail = "";
      return;
    }
    serialWanted = true;
    opts.link.setReactionHandler(handleLine);
    await openSerial(conn.port);
  }

  async function reconcile() {
    if (stopped) return;
    if (reconciling) {
      again = true;
      return;
    }
    reconciling = true;
    try {
      do {
        again = false;
        await reconcileOnce();
      } while (again && !stopped);
    } finally {
      reconciling = false;
    }
  }

  return {
    reconcile,
    reload() {
      runner.retarget();
      return reconcile();
    },
    /**
     * Successful port release. HTTP sockets stay up.
     */
    noteReleased() {
      writeReactionsSuspended(opts.configDir);
      if (transport === "serial") {
        detachSerial();
        clearRetry();
        linkState = "suspended";
        detail = "";
      }
    },
    /**
     * A serial command opened the port. Later event frames reach the rules.
     */
    onPortOpened() {
      if (yieldIsActive(opts.configDir, now())) return;
      clearReactionsSuspended(opts.configDir);
      if (stopped || store.rules.length === 0 || transport !== "serial") return;
      serialWanted = true;
      opts.link.setReactionHandler(handleLine);
      linkState = "up";
      detail = opts.link.port ?? "";
    },
    stop() {
      stopped = true;
      offDown?.();
      epoch += 1;
      clearRetry();
      closeWs();
      detachSerial();
      runner.killAll();
    },
    status() {
      return {
        reactionLink: linkState,
        reactionDetail: detail,
        rulesCount: store.rules.length,
        rulesError: store.fileError,
        lastAction: runner.lastAction,
      };
    },
  };
}

/**
 * @param {string} url
 */
async function defaultOpenWebSocket(url) {
  let WebSocket;
  try {
    ({ default: WebSocket } = await import("ws"));
  } catch {
    throw new Error("reactions require the ws package");
  }
  return new WebSocket(url, { handshakeTimeout: 2000 });
}
