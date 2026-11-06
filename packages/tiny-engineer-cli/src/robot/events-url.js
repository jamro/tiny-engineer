/**
 * Build the outbound-events WebSocket URL from an HTTP base URL.
 * Device has no TLS on port 81 — `https:` bases are rejected.
 * @param {string} baseUrl
 * @param {string | null} [token]
 * @returns {string}
 */
export function eventsWebSocketUrl(baseUrl, token = null) {
  const http = new URL(baseUrl);
  if (http.protocol === "https:") {
    throw new Error(
      "watch HTTP transport requires an http: robot URL (device has no TLS on port 81)",
    );
  }
  if (http.protocol !== "http:") {
    throw new Error(`watch HTTP transport requires an http: robot URL, got ${http.protocol}`);
  }
  const ws = new URL(http.href);
  ws.protocol = "ws:";
  ws.port = "81";
  ws.pathname = "/events";
  ws.search = "";
  ws.hash = "";
  if (token) {
    ws.search = `token=${encodeURIComponent(token)}`;
  }
  return ws.toString();
}

/**
 * Redact the token query for logs/errors.
 * @param {string} url
 * @returns {string}
 */
export function redactEventsUrl(url) {
  try {
    const parsed = new URL(url);
    if (parsed.searchParams.has("token")) {
      parsed.searchParams.set("token", "***");
    }
    return parsed.toString();
  } catch {
    return url.replace(/([?&]token=)[^&]*/i, "$1***");
  }
}
