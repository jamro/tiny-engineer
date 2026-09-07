/**
 * POST /anim?name=… with a 1.5s timeout. Errors are swallowed.
 * @param {string} baseUrl
 * @param {string} animName
 * @param {string | null} [token]
 */
export async function postAnim(baseUrl, animName, token = null) {
  const base = baseUrl.replace(/\/+$/, "");
  const url = `${base}/anim?name=${encodeURIComponent(animName)}`;
  /** @type {RequestInit} */
  const init = {
    method: "POST",
    signal: AbortSignal.timeout(1500),
  };
  if (token) {
    init.headers = { Authorization: `Bearer ${token}` };
  }
  try {
    await fetch(url, init);
  } catch {
    // Robot offline / timeout — never stall the agent.
  }
}
