const BASE = "https://cursor.com";
const DEFAULT_TIMEOUT_MS = 1500;

/**
 * @param {string} cookieValue `id::jwt`
 * @returns {string}
 */
function cookieHeader(cookieValue) {
  return `WorkosCursorSessionToken=${cookieValue.replace(/::/g, "%3A%3A")}`;
}

/**
 * @param {string} cookieValue
 * @param {string} path
 * @param {{ method?: string, body?: unknown, timeoutMs?: number, fetchImpl?: typeof fetch }} [opts]
 * @returns {Promise<Record<string, unknown> | null>}
 */
async function cursorRequest(cookieValue, path, opts = {}) {
  const fetchImpl = opts.fetchImpl ?? globalThis.fetch;
  const timeoutMs = opts.timeoutMs ?? DEFAULT_TIMEOUT_MS;
  /** @type {Record<string, string>} */
  const headers = {
    Cookie: cookieHeader(cookieValue),
    Accept: "application/json",
    "User-Agent": "tiny-engineer-cli",
  };
  /** @type {RequestInit} */
  const init = {
    method: opts.method ?? "GET",
    headers,
    signal: AbortSignal.timeout(timeoutMs),
  };
  if (opts.body !== undefined) {
    headers["Content-Type"] = "application/json";
    headers.Origin = BASE;
    init.body = JSON.stringify(opts.body);
  }
  try {
    const res = await fetchImpl(`${BASE}${path}`, init);
    if (!res.ok) return null;
    const json = await res.json();
    if (!json || typeof json !== "object") return null;
    return /** @type {Record<string, unknown>} */ (json);
  } catch {
    return null;
  }
}

/**
 * Fetch current-cycle limits JSON from cursor.com (fail-open → null).
 *
 * @param {string} cookieValue
 * @param {{ timeoutMs?: number, fetchImpl?: typeof fetch }} [opts]
 * @returns {Promise<Record<string, unknown> | null>}
 */
export async function fetchCursorLimits(cookieValue, opts = {}) {
  const primary = await cursorRequest(cookieValue, "/api/dashboard/get-current-period-usage", {
    method: "POST",
    body: {},
    timeoutMs: opts.timeoutMs,
    fetchImpl: opts.fetchImpl,
  });
  if (primary) return primary;

  return cursorRequest(cookieValue, "/api/usage-summary", {
    method: "GET",
    timeoutMs: opts.timeoutMs,
    fetchImpl: opts.fetchImpl,
  });
}
