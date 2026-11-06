/**
 * NDJSON peer over a socket. Responses with `id` resolve `request()`.
 * Other messages go to listeners. Socket close/error rejects waiters.
 * @param {import("node:net").Socket} socket
 * @param {string} [initial]
 */
export function createPeer(socket, initial = "") {
  let buf = initial;
  let seq = 0;
  let closed = false;
  /**
   * @typedef {{
   *   resolve: (msg: Record<string, unknown>) => void,
   *   reject: (err: Error) => void,
   *   timer: ReturnType<typeof setTimeout>,
   * }} Waiter
   */
  /** @type {Map<number, Waiter>} */
  const waiters = new Map();
  /** @type {Set<(msg: Record<string, unknown>) => void>} */
  const listeners = new Set();
  /** @type {Record<string, unknown>[]} */
  const queued = [];

  const failClosed = () => {
    if (closed) return;
    closed = true;
    const err = new Error("daemon connection closed");
    for (const [, waiter] of waiters) {
      clearTimeout(waiter.timer);
      waiter.reject(err);
    }
    waiters.clear();
  };

  /**
   * @param {Record<string, unknown>} msg
   */
  const emit = (msg) => {
    if (msg.id != null && waiters.has(/** @type {number} */ (msg.id))) {
      const waiter = waiters.get(/** @type {number} */ (msg.id));
      waiters.delete(/** @type {number} */ (msg.id));
      clearTimeout(waiter?.timer);
      waiter?.resolve(msg);
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
  socket.on("close", failClosed);
  socket.on("error", failClosed);
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
        if (closed) {
          reject(new Error("daemon connection closed"));
          return;
        }
        const timer = setTimeout(() => {
          waiters.delete(id);
          reject(new Error(`daemon request timed out (${method})`));
        }, timeoutMs);
        waiters.set(id, { resolve, reject, timer });
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
