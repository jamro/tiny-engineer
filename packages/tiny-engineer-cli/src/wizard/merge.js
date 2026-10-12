import { copyFileSync, existsSync, mkdirSync, readFileSync, writeFileSync } from "node:fs";
import { dirname } from "node:path";

/**
 * @param {string} path
 * @returns {object | null}
 */
export function readJsonFile(path) {
  if (!existsSync(path)) return null;
  try {
    const raw = readFileSync(path, "utf8");
    const parsed = JSON.parse(raw);
    return parsed && typeof parsed === "object" ? parsed : null;
  } catch {
    return null;
  }
}

/**
 * Write JSON; create parent dirs; backup once to `path.bak` if file exists and bak does not.
 * @param {string} path
 * @param {object} data
 */
export function writeJsonFile(path, data) {
  mkdirSync(dirname(path), { recursive: true });
  if (existsSync(path) && !existsSync(`${path}.bak`)) {
    copyFileSync(path, `${path}.bak`);
  }
  writeFileSync(path, `${JSON.stringify(data, null, 2)}\n`, "utf8");
}

/**
 * @param {string} envPath
 * @param {string} key
 * @param {string} value
 * @param {{ force?: boolean }} [opts]
 * @returns {"created" | "updated" | "skipped"}
 */
function mergeEnvAssignment(envPath, key, value, { force = false } = {}) {
  mkdirSync(dirname(envPath), { recursive: true });
  let raw = "";
  if (existsSync(envPath)) {
    raw = readFileSync(envPath, "utf8");
  }

  const keyRe = new RegExp(`^\\s*${key}\\s*=`);
  const lines = raw ? raw.split(/\r?\n/) : [];
  let found = false;
  const next = lines.map((line) => {
    if (!keyRe.test(line)) return line;
    found = true;
    if (!force) return line;
    return `${key}=${value}`;
  });

  if (found && !force) return "skipped";

  if (!found) {
    if (next.length && next[next.length - 1] !== "") next.push("");
    next.push(`${key}=${value}`);
    writeFileSync(envPath, `${next.join("\n").replace(/\n+$/, "\n")}`, "utf8");
    return raw ? "updated" : "created";
  }

  writeFileSync(envPath, `${next.join("\n").replace(/\n+$/, "\n")}`, "utf8");
  return "updated";
}

/**
 * Merge TINY_ENGINEER_URL into a `.env` file.
 * @param {string} envPath
 * @param {string} url
 * @param {{ force?: boolean }} [opts]
 * @returns {"created" | "updated" | "skipped"}
 */
export function mergeEnvUrl(envPath, url, { force = false } = {}) {
  return mergeEnvAssignment(envPath, "TINY_ENGINEER_URL", url, { force });
}

/**
 * Merge TINY_ENGINEER_SERIAL into a `.env` file.
 * @param {string} envPath
 * @param {string} serialPath path or `auto`
 * @param {{ force?: boolean }} [opts]
 * @returns {"created" | "updated" | "skipped"}
 */
export function mergeEnvSerial(envPath, serialPath, { force = false } = {}) {
  return mergeEnvAssignment(envPath, "TINY_ENGINEER_SERIAL", serialPath, { force });
}

/**
 * Remove one KEY=… line from `.env` (exact key name).
 * @param {string} envPath
 * @param {string} key
 * @returns {"removed" | "absent" | "unchanged"}
 */
export function stripEnvKey(envPath, key) {
  if (!existsSync(envPath)) return "absent";
  const keyRe = new RegExp(`^\\s*${key}\\s*=`);
  const raw = readFileSync(envPath, "utf8");
  const lines = raw.split(/\r?\n/);
  const kept = lines.filter((line) => !keyRe.test(line));
  if (kept.length === lines.length) return "unchanged";

  while (kept.length > 0 && kept[kept.length - 1] === "") kept.pop();
  if (kept.length === 0) {
    writeFileSync(envPath, "", "utf8");
  } else {
    writeFileSync(envPath, `${kept.join("\n")}\n`, "utf8");
  }
  return "removed";
}

const ENV_TE_LINE = /^\s*TINY_ENGINEER_(URL|TOKEN|SERIAL)\s*=/;

/**
 * Remove TINY_ENGINEER_URL / TOKEN / SERIAL lines from `.env`.
 * Leaves the file if other content remains; does not delete a non-empty file.
 * @param {string} envPath
 * @returns {"removed" | "absent" | "unchanged"}
 */
export function stripEnvTinyEngineer(envPath) {
  if (!existsSync(envPath)) return "absent";
  const raw = readFileSync(envPath, "utf8");
  const lines = raw.split(/\r?\n/);
  const kept = lines.filter((line) => !ENV_TE_LINE.test(line));
  if (kept.length === lines.length) return "unchanged";

  // Trim trailing empty lines but keep a final newline when content remains.
  while (kept.length > 0 && kept[kept.length - 1] === "") kept.pop();
  if (kept.length === 0) {
    writeFileSync(envPath, "", "utf8");
  } else {
    writeFileSync(envPath, `${kept.join("\n")}\n`, "utf8");
  }
  return "removed";
}
