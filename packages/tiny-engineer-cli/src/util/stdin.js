/** Max wait for non-TTY stdin when the pipe never emits `end` (hook JSON is tiny). */
const STDIN_TIMEOUT_MS = 2000;

/**
 * Read stdin (or `stream`) to a string. Empty string when TTY or on error.
 * Resolves with data collected so far if the pipe stays open past `timeoutMs`.
 *
 * @param {{ timeoutMs?: number, stream?: NodeJS.ReadableStream & { isTTY?: boolean } }} [opts]
 * @returns {Promise<string>}
 */
export function readStdin({ timeoutMs = STDIN_TIMEOUT_MS, stream = process.stdin } = {}) {
  return new Promise((resolve) => {
    if (stream.isTTY) {
      resolve("");
      return;
    }

    const chunks = [];
    let settled = false;
    /** @type {ReturnType<typeof setTimeout> | undefined} */
    let timer;

    const onData = (c) => chunks.push(c);
    const onEnd = () => finish(chunks.join(""));
    const onError = () => finish("");

    function finish(value) {
      if (settled) return;
      settled = true;
      clearTimeout(timer);
      stream.off("data", onData);
      stream.off("end", onEnd);
      stream.off("error", onError);
      resolve(value);
    }

    timer = setTimeout(() => finish(chunks.join("")), timeoutMs);
    stream.setEncoding("utf8");
    stream.on("data", onData);
    stream.on("end", onEnd);
    stream.on("error", onError);
  });
}
