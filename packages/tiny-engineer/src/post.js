/**
 * POST /anim?name=… with a 2s timeout. Errors are swallowed.
 * @param {string} baseUrl
 * @param {string} animName
 */
export async function postAnim(baseUrl, animName) {
  const base = baseUrl.replace(/\/+$/, "");
  const url = `${base}/anim?name=${encodeURIComponent(animName)}`;
  try {
    await fetch(url, {
      method: "POST",
      signal: AbortSignal.timeout(2000),
    });
  } catch {
    // Robot offline / timeout — never stall the agent.
  }
}
