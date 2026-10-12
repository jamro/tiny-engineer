import { readFileSync } from "node:fs";
import { join } from "node:path";

import { DEFAULT_URL, getBaseUrl, getSerialPath, getToken } from "../config/env.js";
import { all as allIntegrations } from "../integrations/registry.js";
import { request, resolveTransport, transportLabel } from "../robot/transport.js";
import { parseCommonArgs } from "../util/args.js";
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
 * @param {import("../robot/transport.js").Transport} transport
 * @returns {Promise<{ ok: boolean, detail: string }>}
 */
export async function checkHealth(transport) {
  const result = await request(transport, {
    method: "GET",
    path: "/health",
    token: getToken(),
    timeoutMs: 2000,
  });
  if (result.ok) {
    return { ok: true, detail: (result.body ?? "").trim() || `HTTP ${result.status}` };
  }
  return { ok: false, detail: result.error || result.body || "health failed" };
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 * @param {{ projectRoot?: string }} [opts]
 */
export async function runDoctor(out, argv, { projectRoot = process.cwd() } = {}) {
  const opts = parseCommonArgs(argv);
  if (opts.error) {
    out.error(opts.error);
    process.exitCode = 1;
    return;
  }
  if (opts.help) {
    console.log(`Usage: tiny-engineer doctor [--url <base>|--serial <path>]

Report robot transport, token, detected IDE configs, and /health.
`);
    return;
  }
  if (opts.rest.length > 0) {
    out.error(`Unknown argument: ${opts.rest[0]}`);
    process.exitCode = 1;
    return;
  }

  const transport = resolveTransport({ url: opts.url, serial: opts.serial });
  const token = getToken();

  out.ok(`transport: ${transport.type} (${transportLabel(transport)})`);
  if (transport.type === "http") {
    out.info(`url default: ${getBaseUrl() || DEFAULT_URL}`);
  } else {
    out.info(`serial env: ${getSerialPath() ? "set" : "not set"}`);
  }
  out.info(`token: ${token ? "set" : "not set"} (HTTP Bearer; unused on serial)`);

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

  const health = await checkHealth(transport);
  if (health.ok) {
    out.ok(`health: ok (${health.detail.slice(0, 80)})`);
  } else {
    out.error(`health: ${health.detail}`);
  }
  process.exitCode = 0;
}
