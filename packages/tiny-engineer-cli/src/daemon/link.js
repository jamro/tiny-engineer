import { readFileSync } from "node:fs";

/**
 * One SerialRobotClient session owned by the daemon.
 * @param {{
 *   connect: (opts: { port?: string | null }) => Promise<{ port: string, client: import("../robot/serial/client.js").SerialRobotClient }>,
 *   preferredPort?: string | null,
 * }} opts
 */
export function createSerialLink({ connect, preferredPort = null }) {
  /** @typedef {{ port: string, client: import("../robot/serial/client.js").SerialRobotClient }} SerialSession */
  /** @type {SerialSession | null} */
  let session = null;
  /** @type {Promise<SerialSession> | null} */
  let opening = null;
  let playActive = false;
  /** @type {Set<(line: string) => void>} */
  const eventSubs = new Set();
  /** @type {Set<(line: string) => void>} */
  const logSubs = new Set();
  /** @type {Set<() => void>} */
  const linkDownSubs = new Set();

  function markDown() {
    if (!session) return;
    session = null;
    for (const fn of linkDownSubs) fn();
  }

  /**
   * @param {string | null} port
   * @returns {Promise<SerialSession>}
   */
  async function openSession(port) {
    const opened = await connect({ port });
    opened.client.setEventHandler((payload) => {
      const line = payload.toString("utf8");
      for (const fn of eventSubs) fn(line);
    });
    opened.client.setLogHandler((payload) => {
      const line = payload.toString("utf8");
      for (const fn of logSubs) fn(line);
    });
    if (typeof opened.client.setCloseHandler === "function") {
      opened.client.setCloseHandler(() => {
        markDown();
      });
    }
    session = opened;
    return opened;
  }

  /**
   * @param {string | null | undefined} requestedPort
   */
  async function ensure(requestedPort) {
    if (session) {
      if (requestedPort && requestedPort !== session.port) {
        throw new Error(`daemon is on ${session.port}, this command asked for ${requestedPort}`);
      }
      return session;
    }
    if (!opening) {
      opening = openSession(requestedPort || preferredPort || null).finally(() => {
        opening = null;
      });
    }
    await opening;
    return ensure(requestedPort);
  }

  return {
    get port() {
      return session?.port ?? null;
    },
    get playActive() {
      return playActive;
    },
    /** @param {string | null | undefined} requestedPort */
    ensure,
    /**
     * @param {string} op
     * @param {Record<string, unknown>} fields
     * @param {string | null | undefined} port
     */
    async request(op, fields, port) {
      const current = await ensure(port);
      try {
        return await current.client.request(op, fields);
      } catch (err) {
        const message = err instanceof Error ? err.message : String(err);
        if (/serial port closed|port is not open/i.test(message)) markDown();
        throw err;
      }
    },
    /**
     * @param {string} wavPath
     * @param {string} name
     * @param {string | null | undefined} port
     */
    async play(wavPath, name, port) {
      const current = await ensure(port);
      playActive = true;
      try {
        const wav = readFileSync(wavPath);
        return await current.client.play(wav, name);
      } finally {
        playActive = false;
      }
    },
    /**
     * @param {"serial.events" | "serial.logs"} channel
     * @param {(line: string) => void} fn
     */
    subscribe(channel, fn) {
      const set = channel === "serial.logs" ? logSubs : eventSubs;
      set.add(fn);
      return () => set.delete(fn);
    },
    onLinkDown(fn) {
      linkDownSubs.add(fn);
      return () => linkDownSubs.delete(fn);
    },
    async close() {
      const current = session;
      session = null;
      if (current) await current.client.close().catch(() => {});
    },
  };
}
