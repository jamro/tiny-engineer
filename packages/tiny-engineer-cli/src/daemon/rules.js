/**
 * @typedef {{ id: number, on: string, run: string[], source?: string, timeoutMs?: number }} ReactionRule
 */

/**
 * @param {unknown} value
 * @returns {{ ok: true, rules: ReactionRule[] } | { ok: false, error: string }}
 */
export function parseRulesDocument(value) {
  if (!value || typeof value !== "object" || Array.isArray(value)) {
    return { ok: false, error: "invalid rules.json" };
  }
  const doc = /** @type {Record<string, unknown>} */ (value);
  if (doc.v !== 1 || !Array.isArray(doc.rules)) {
    return { ok: false, error: "invalid rules.json" };
  }
  /** @type {ReactionRule[]} */
  const rules = [];
  for (let i = 0; i < doc.rules.length; i += 1) {
    const parsed = parseRule(doc.rules[i], i);
    if (!parsed.ok) return parsed;
    rules.push(parsed.rule);
  }
  return { ok: true, rules };
}

/**
 * @param {unknown} value
 * @param {number} id
 * @returns {{ ok: true, rule: ReactionRule } | { ok: false, error: string }}
 */
function parseRule(value, id) {
  if (!value || typeof value !== "object" || Array.isArray(value)) {
    return { ok: false, error: "invalid rules.json" };
  }
  const raw = /** @type {Record<string, unknown>} */ (value);
  if (typeof raw.on !== "string" || !Array.isArray(raw.run) || raw.run.length === 0) {
    return { ok: false, error: "invalid rules.json" };
  }
  if (!raw.run.every((part) => typeof part === "string")) {
    return { ok: false, error: "invalid rules.json" };
  }
  /** @type {ReactionRule} */
  const rule = { id, on: raw.on, run: /** @type {string[]} */ (raw.run) };
  if ("source" in raw) {
    if (typeof raw.source !== "string") return { ok: false, error: "invalid rules.json" };
    rule.source = raw.source;
  }
  if ("timeoutMs" in raw) {
    if (typeof raw.timeoutMs !== "number" || !Number.isInteger(raw.timeoutMs) || raw.timeoutMs <= 0) {
      return { ok: false, error: "invalid rules.json" };
    }
    rule.timeoutMs = raw.timeoutMs;
  }
  return { ok: true, rule };
}

/**
 * Last good set. A bad document keeps the previous rules and records an error.
 */
export function createRuleStore() {
  /** @type {ReactionRule[]} */
  let rules = [];
  /** @type {string | null} */
  let fileError = null;
  return {
    get rules() {
      return rules;
    },
    get fileError() {
      return fileError;
    },
    /**
     * @param {string} text
     */
    loadText(text) {
      let value;
      try {
        value = JSON.parse(text);
      } catch {
        fileError = "invalid rules.json";
        return;
      }
      const parsed = parseRulesDocument(value);
      if (!parsed.ok) {
        fileError = parsed.error;
        return;
      }
      rules = parsed.rules;
      fileError = null;
    },
    loadMissing() {
      rules = [];
      fileError = null;
    },
  };
}

/**
 * @param {ReactionRule[]} rules
 * @param {unknown} event
 * @returns {ReactionRule[]}
 */
export function matchRules(rules, event) {
  if (!event || typeof event !== "object") return [];
  const record = /** @type {Record<string, unknown>} */ (event);
  if (record.v !== 1 || typeof record.type !== "string") return [];
  return rules.filter((rule) => {
    if (rule.on !== record.type) return false;
    if (rule.source !== undefined && rule.source !== record.source) return false;
    return true;
  });
}
