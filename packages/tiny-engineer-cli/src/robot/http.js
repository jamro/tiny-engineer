/**
 * fetch() against the robot: joins base URL and path and adds the Bearer token when set.
 * @param {string} baseUrl
 * @param {string} path
 * @param {RequestInit} init
 * @param {string | null} [token]
 */
export function robotFetch(baseUrl, path, init, token = null) {
  const headers = { ...init.headers };
  if (token) headers.Authorization = `Bearer ${token}`;
  return fetch(`${baseUrl.replace(/\/+$/, "")}${path}`, { ...init, headers });
}
