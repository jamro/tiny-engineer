/**
 * localStorage helpers for build-guide pages (shop checklist, print queue, …).
 * Private mode / quota → read null, write no-op.
 */

export const read = (key) => {
  try {
    return window.localStorage?.getItem(key) ?? null;
  } catch {
    return null;
  }
};

export const write = (key, value) => {
  try {
    if (value == null) window.localStorage?.removeItem(key);
    else window.localStorage?.setItem(key, String(value));
  } catch {
    /* private mode / quota */
  }
};

export const loadChecked = (key) => {
  const raw = read(key);
  if (!raw) return new Set();
  try {
    const parsed = JSON.parse(raw);
    if (Array.isArray(parsed)) return new Set(parsed.map(String));
    if (parsed && typeof parsed === "object") {
      return new Set(Object.keys(parsed).filter((k) => parsed[k]));
    }
  } catch {
    /* ignore */
  }
  return new Set();
};

export const saveChecked = (set, key) => {
  write(key, JSON.stringify([...set]));
};
