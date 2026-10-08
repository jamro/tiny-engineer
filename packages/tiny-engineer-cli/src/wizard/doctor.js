import { readFileSync } from "node:fs";
import { join } from "node:path";

import { DEFAULT_URL, getBaseUrl, getToken } from "../config/env.js";
import { all as allIntegrations } from "../integrations/registry.js";
import { robotFetch } from "../robot/client.js";
import { readJsonFile } from "./merge.js";

/**
 * @param {unknown} value
 * @returns {string}
 */
function collectCommands(value) {
  if (typeof value === "string") return value;
  if (Array.isArray(value)) return value.map(collectCommands).join("\n");
  if (value && typeof value === "object") {
    return Object.values(value).map(collectCommands).join("\n");
  }
  return "";
}

/**
 * @param {string} projectRoot
 * @param {string} ideId
 * @param {string} relativePath
 * @returns {boolean}
 */
export function configHasHook(projectRoot, ideId, relativePath) {
  const data = readJsonFile(join(projectRoot, relativePath));
  if (!data) return false;
  const blob = collectCommands(data);
  return blob.includes(`hook ${ideId}`) || blob.includes(`tiny-engineer-${ideId === "claude-code" ? "claude-code" : ideId === "cursor" ? "cursor" : "antigravity"}`);
}

/**
 * @param {string} baseUrl
 * @returns {Promise<{ ok: boolean, detail: string }>}
 */
export async function checkHealth(baseUrl) {
  try {
    const res = await robotFetch(
      baseUrl,
      "/health",
      { method: "GET", signal: AbortSignal.timeout(2000) },
      getToken(),
    );
    const body = await res.text();
    if (res.ok) return { ok: true, detail: body.trim() || `HTTP ${res.status}` };
    return { ok: false, detail: body.trim() || `HTTP ${res.status}` };
  } catch (err) {
    const detail =
      err instanceof Error ? err.message : String(err);
    return { ok: false, detail };
  }
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 * @param {{ projectRoot?: string }} [opts]
 */
export async function runDoctor(out, argv, { projectRoot = process.cwd() } = {}) {
  for (const arg of argv) {
    if (arg === "-h" || arg === "--help") {
      console.log(`Usage: tiny-engineer doctor

Report robot URL/token, detected IDE configs, and /health.
`);
      return;
    }
    if (arg.startsWith("-")) {
      out.error(`Unknown argument: ${arg}`);
      process.exitCode = 1;
      return;
    }
  }

  const url = getBaseUrl() || DEFAULT_URL;
  const token = getToken();

  out.ok(`url: ${url}`);
  out.info(`token: ${token ? "set" : "not set"}`);

  for (const integration of allIntegrations()) {
    const detected = Boolean(integration.detect?.(projectRoot));
    const rel = integration.configRelativePath?.() ?? "";
    const wired = rel ? configHasHook(projectRoot, integration.id, rel) : false;
    const mark = wired ? "hooks ok" : detected ? "detected, hooks missing" : "not detected";
    out.info(`${integration.id}: ${mark}${rel ? ` (${rel})` : ""}`);
  }

  // Surface .env presence without printing secrets.
  try {
    readFileSync(join(projectRoot, ".env"), "utf8");
    out.info(".env: present");
  } catch {
    out.info(".env: absent");
  }

  const health = await checkHealth(url);
  if (health.ok) {
    out.ok(`health: ok (${health.detail.slice(0, 80)})`);
  } else {
    out.error(`health: ${health.detail}`);
    // Non-fatal for doctor — still exit 0 unless user expects otherwise.
    // Plan: print ok/fail (not fatal).
  }
  process.exitCode = 0;
}
