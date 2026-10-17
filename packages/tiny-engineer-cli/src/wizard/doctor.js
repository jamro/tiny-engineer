import { readFileSync } from "node:fs";
import { join } from "node:path";

import { DEFAULT_URL, getBaseUrl, getToken, resolveTransport } from "../config/env.js";
import { all as allIntegrations } from "../integrations/registry.js";
import { robotHealth } from "../robot/transport.js";
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
 * @param {string} baseUrl
 * @returns {Promise<{ ok: boolean, detail: string }>}
 */
export async function checkHealth(baseUrl) {
  return robotHealth({ url: baseUrl, transport: "http" });
}

/**
 * @param {import("../util/output.js").Output} out
 * @param {string[]} argv
 * @param {{ projectRoot?: string }} [opts]
 */
export async function runDoctor(out, argv, { projectRoot = process.cwd() } = {}) {
  const args = parseCommonArgs(argv);
  if (args.error) {
    out.error(args.error);
    process.exitCode = 1;
    return;
  }
  if (args.help) {
    console.log(`Usage: tiny-engineer doctor [--transport http|serial] [--url] [--port]

Report robot transport/URL/token, detected IDE configs, and health.
`);
    return;
  }
  if (args.rest.length > 0) {
    out.error(`Unexpected argument: ${args.rest[0]}`);
    process.exitCode = 1;
    return;
  }

  const resolved = resolveTransport(args.transport);
  if (resolved.error) {
    out.error(resolved.error);
    process.exitCode = 1;
    return;
  }

  const url = args.url ?? getBaseUrl() ?? DEFAULT_URL;
  const token = getToken();

  out.ok(`transport: ${resolved.transport}`);
  if (resolved.transport === "http") {
    out.ok(`url: ${url}`);
    out.info(`token: ${token ? "set" : "not set"}`);
  } else if (args.port) {
    out.info(`port: ${args.port}`);
  }

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

  const health = await robotHealth({
    url: args.url,
    transport: args.transport,
    port: args.port,
  });
  if (health.ok) {
    out.ok(`health: ok (${health.detail.slice(0, 80)})`);
  } else {
    out.error(`health: ${health.detail}`);
  }
  process.exitCode = 0;
}
