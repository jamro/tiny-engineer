/**
 * NDJSON peer over a socket. Responses with `id` resolve `request()`.
 * Other messages go to listeners.
 * @param {import("node:net").Socket} socket
 * @param {string} [initial]
 */
export function createPeer(socket, initial = "") {
  let buf = initial;
  let seq = 0;
  /** @type {Map<number, (msg: Record<string, unknown>) => void>} */
  const waiters = new Map();
  /** @type {Set<(msg: Record<string, unknown>) => void>} */
  const listeners = new Set();
  /** @type {Record<string, unknown>[]} */
  const queued = [];

  /**
   * @param {Record<string, unknown>} msg
   */
  const emit = (msg) => {
    if (msg.id != null && waiters.has(/** @type {number} */ (msg.id))) {
      const fn = waiters.get(/** @type {number} */ (msg.id));
      waiters.delete(/** @type {number} */ (msg.id));
      fn?.(msg);
      return;
    }
    if (listeners.size === 0) queued.push(msg);
    else for (const fn of listeners) fn(msg);
  };

  const drain = () => {
    let nl = buf.indexOf("\n");
    while (nl >= 0) {
      const line = buf.slice(0, nl);
      buf = buf.slice(nl + 1);
      nl = buf.indexOf("\n");
      if (!line.trim()) continue;
      try {
        emit(JSON.parse(line));
      } catch {
        // ignore a broken line
      }
    }
  };

  socket.on("data", (chunk) => {
    buf += chunk.toString("utf8");
    drain();
  });
  drain();

  return {
    socket,
    /**
     * @param {Record<string, unknown>} msg
     */
    send(msg) {
      socket.write(`${JSON.stringify(msg)}\n`);
    },
    /**
     * @param {string} method
     * @param {Record<string, unknown>} [params]
     * @param {number} [timeoutMs]
     * @returns {Promise<Record<string, unknown>>}
     */
    request(method, params = {}, timeoutMs = 5000) {
      const id = ++seq;
      return new Promise((resolve, reject) => {
        const timer = setTimeout(() => {
          waiters.delete(id);
          reject(new Error(`daemon request timed out (${method})`));
        }, timeoutMs);
        waiters.set(id, (msg) => {
          clearTimeout(timer);
          resolve(msg);
        });
        this.send({ id, method, params });
      });
    },
    /**
     * @param {(msg: Record<string, unknown>) => void} fn
     */
    on(fn) {
      listeners.add(fn);
      while (queued.length) fn(queued.shift());
      return () => listeners.delete(fn);
    },
  };
}
