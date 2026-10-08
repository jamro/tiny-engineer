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
 * Merge TINY_ENGINEER_URL into a `.env` file.
 * @param {string} envPath
 * @param {string} url
 * @param {{ force?: boolean }} [opts]
 * @returns {"created" | "updated" | "skipped"}
 */
export function mergeEnvUrl(envPath, url, { force = false } = {}) {
  mkdirSync(dirname(envPath), { recursive: true });
  let raw = "";
  if (existsSync(envPath)) {
    raw = readFileSync(envPath, "utf8");
  }

  const lines = raw ? raw.split(/\r?\n/) : [];
  let found = false;
  const next = lines.map((line) => {
    if (!/^\s*TINY_ENGINEER_URL\s*=/.test(line)) return line;
    found = true;
    if (!force) return line;
    return `TINY_ENGINEER_URL=${url}`;
  });

  if (found && !force) return "skipped";

  if (!found) {
    if (next.length && next[next.length - 1] !== "") next.push("");
    next.push(`TINY_ENGINEER_URL=${url}`);
    writeFileSync(envPath, `${next.join("\n").replace(/\n+$/, "\n")}`, "utf8");
    return raw ? "updated" : "created";
  }

  writeFileSync(envPath, `${next.join("\n").replace(/\n+$/, "\n")}`, "utf8");
  return "updated";
}

const ENV_TE_LINE = /^\s*TINY_ENGINEER_(URL|TOKEN)\s*=/;

/**
 * Remove TINY_ENGINEER_URL / TINY_ENGINEER_TOKEN lines from `.env`.
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
