import { readFileSync } from "node:fs";

import { hostCalloutPath } from "../robot/serial/client.js";

/**
 * One SerialRobotClient session owned by the daemon.
 * @param {{
 *   connect: (opts: { port?: string | null, writeCache?: boolean }) => Promise<{ port: string, client: import("../robot/serial/client.js").SerialRobotClient }>,
 *   preferredPort?: string | null,
 * }} opts
 */
export function createSerialLink({ connect, preferredPort = null }) {
  /** @typedef {{ port: string, client: import("../robot/serial/client.js").SerialRobotClient }} SerialSession */
  /** @type {SerialSession | null} */
  let session = null;
  /** @type {Promise<SerialSession> | null} */
  let opening = null;
  /** Bumped by release/close so in-flight connects do not re-hold the tty. */
  let generation = 0;
  let playActive = false;
  /** @type {Set<(line: string) => void>} */
  const eventSubs = new Set();
  /** @type {Set<(line: string) => void>} */
  const logSubs = new Set();
  /** @type {Set<(reason: "drop" | "release") => void>} */
  const linkDownSubs = new Set();
  /** @type {((line: string) => void) | null} */
  let reactionHandler = null;

  function markDown() {
    if (!session) return;
    session = null;
    for (const fn of linkDownSubs) fn("drop");
  }

  /**
   * @param {string | null} port
   * @param {{ writeCache?: boolean }} [connectOpts]
   * @returns {Promise<SerialSession>}
   */
  async function openSession(port, connectOpts = {}) {
    const gen = generation;
    const opened = await connect({ port, ...connectOpts });
    if (gen !== generation) {
      await opened.client.close().catch(() => {});
      throw new Error("open-cancelled");
    }
    opened.client.setEventHandler((payload) => {
      const line = payload.toString("utf8");
      for (const fn of eventSubs) fn(line);
      reactionHandler?.(line);
    });
    opened.client.setLogHandler((payload) => {
      const line = payload.toString("utf8");
      for (const fn of logSubs) fn(line);
    });
    if (typeof opened.client.setCloseHandler === "function") {
      opened.client.setCloseHandler(() => {
        if (session === opened) markDown();
      });
    }
    session = opened;
    if (gen !== generation) {
      if (session === opened) session = null;
      await opened.client.close().catch(() => {});
      throw new Error("open-cancelled");
    }
    return opened;
  }

  /**
   * Invalidate in-flight opens (old generation), then drop the held session.
   * A newer `ensure` started after the bump may set `session` while we await
   * the old close — leave that replacement alone.
   * @param {"drop" | "release" | null} downReason
   *   `null` = silent close (process shutdown). Otherwise notify subscribers.
   */
  async function dropSession(downReason) {
    generation += 1;
    const pending = opening;
    const current = session;
    session = null;
    if (current && downReason) {
      for (const fn of linkDownSubs) fn(downReason);
    }
    if (current) await current.client.close().catch(() => {});
    if (pending) await pending.catch(() => {});
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
     * Ordered sync probe. Does not write the port cache. Refuses while play is active.
     * When a session is held, only that path is synced; other candidates are skipped.
     * @param {string[]} candidates
     * @returns {Promise<{
     *   ok: boolean,
     *   results: Array<{ path: string, ok: boolean, detail: string, skipped?: boolean }>,
     *   port: string | null,
     * }>}
     */
    async probe(candidates) {
      if (playActive) {
        throw new Error("play-active");
      }
      /** @type {Array<{ path: string, ok: boolean, detail: string, skipped?: boolean }>} */
      const results = [];
      const normalized = candidates
        .map((path) => (typeof path === "string" ? hostCalloutPath(path.trim()) : ""))
        .filter(Boolean);

      if (session) {
        const held = session.port;
        let synced = false;
        for (const path of normalized) {
          if (path !== held) {
            results.push({
              path,
              ok: false,
              skipped: true,
              detail: `skipped (daemon holds ${held})`,
            });
            continue;
          }
          try {
            await session.client.request("sync", {});
            results.push({ path, ok: true, detail: "sync ok" });
            synced = true;
          } catch (err) {
            const message = err instanceof Error ? err.message : String(err);
            results.push({ path, ok: false, detail: message });
          }
        }
        return { ok: synced, results, port: session.port };
      }

      for (const path of normalized) {
        try {
          await openSession(path, { writeCache: false });
          results.push({ path, ok: true, detail: "sync ok" });
          return { ok: true, results, port: session?.port ?? path };
        } catch (err) {
          const message = err instanceof Error ? err.message : String(err);
          results.push({ path, ok: false, detail: message });
        }
      }
      return { ok: false, results, port: null };
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
    /**
     * Reaction runner listener. Not the IPC fanout, so a one-off serial
     * command cannot double-fire HTTP reactions.
     * @param {((line: string) => void) | null} fn
     */
    setReactionHandler(fn) {
      reactionHandler = fn;
    },
    /**
     * @param {(reason: "drop" | "release") => void} fn
     * `drop` is an unplanned close. `release` is daemon release or yield.
     */
    onLinkDown(fn) {
      linkDownSubs.add(fn);
      return () => linkDownSubs.delete(fn);
    },
    /**
     * Close the tty and tell subscribers. Refuses while a play is active
     * unless `force` is set (upload yield).
     * @param {boolean} [force]
     * @returns {Promise<boolean>} false when play blocked the release
     */
    async release(force = false) {
      if (playActive && !force) return false;
      await dropSession("release");
      return true;
    },
    async close() {
      await dropSession(null);
    },
  };
}
