import { createInterface } from "node:readline/promises";
import { stdin as input, stdout as output } from "node:process";

/**
 * @param {string} question
 * @param {string} [defaultValue]
 * @returns {Promise<string>}
 */
export async function ask(question, defaultValue = "") {
  const rl = createInterface({ input, output });
  try {
    const hint = defaultValue ? ` [${defaultValue}]` : "";
    const answer = (await rl.question(`${question}${hint}: `)).trim();
    return answer || defaultValue;
  } finally {
    rl.close();
  }
}

/**
 * @param {string} question
 * @param {boolean} [defaultYes]
 * @returns {Promise<boolean>}
 */
export async function confirm(question, defaultYes = true) {
  const suffix = defaultYes ? "Y/n" : "y/N";
  const answer = (await ask(`${question} (${suffix})`, defaultYes ? "y" : "n")).toLowerCase();
  if (!answer) return defaultYes;
  return answer === "y" || answer === "yes";
}

/**
 * Numbered multi-select. Empty enter selects all.
 * @param {string} title
 * @param {{ id: string, label: string }[]} options
 * @returns {Promise<string[]>} selected ids
 */
export async function multiSelect(title, options) {
  if (options.length === 0) return [];
  console.log(title);
  options.forEach((opt, i) => {
    console.log(`  ${i + 1}) ${opt.label} (${opt.id})`);
  });
  const raw = await ask("Select numbers (comma-separated, Enter = all)", "");
  if (!raw) return options.map((o) => o.id);

  /** @type {Set<string>} */
  const selected = new Set();
  for (const part of raw.split(/[,\s]+/)) {
    const n = Number(part);
    if (Number.isInteger(n) && n >= 1 && n <= options.length) {
      selected.add(options[n - 1].id);
    }
  }
  return selected.size > 0 ? [...selected] : options.map((o) => o.id);
}
