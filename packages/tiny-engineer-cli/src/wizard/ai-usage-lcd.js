import { CLAUDE_BUDGET_LABEL, CURSOR_BUDGET_LABEL } from "../integrations/ai-usage-lcd.js";

/** IDEs whose hooks can post remaining plan budget to the LCD. */
export const AI_USAGE_LCD_IDES = new Set(["cursor", "claude-code"]);

/** @type {Record<string, string>} */
const IDE_LABEL = {
  cursor: "Cursor",
  "claude-code": "Claude Code",
};

/** @type {Record<string, string>} */
const LCD_LABEL = {
  cursor: CURSOR_BUDGET_LABEL,
  "claude-code": CLAUDE_BUDGET_LABEL,
};

/**
 * @param {string[]} selected
 * @returns {string[]}
 */
export function budgetIdeIds(selected) {
  return selected.filter((id) => AI_USAGE_LCD_IDES.has(id));
}

/**
 * @param {string} id
 * @returns {string}
 */
function ideLabel(id) {
  return IDE_LABEL[id] ?? id;
}

/**
 * Prompt only for IDEs that support the readout. Caller must pass a non-empty list.
 * @param {string[]} ides
 * @returns {string}
 */
export function aiUsageLcdPrompt(ides) {
  const names = ides.map(ideLabel);
  const who = names.length > 1 ? names.join(" and ") : names[0];
  return `\nAI budget on the LCD1602 after ${who} turns? Needs the lcd1602 extension and the lcd_cabinet mod.`;
}

/**
 * @param {string[]} selected IDEs that were chosen and do not support the readout
 * @returns {string}
 */
export function aiUsageLcdSkipMessage(selected) {
  return `AI budget monitoring is not available for ${selected.join(", ")}. Skipping that option.`;
}

/**
 * Sign-in steps for the IDEs that were actually selected.
 * @param {string[]} ides
 * @returns {string[]}
 */
export function aiUsageLcdGuide(ides) {
  /** @type {string[]} */
  const lines = [
    "LCD AI budget is enabled. Finish these steps or the screen stays blank (the hook still exits 0):",
    "1. On the robot, turn on the lcd1602 extension and fit the lcd_cabinet mod.",
  ];
  let step = 2;
  for (const id of ides) {
    const block = GUIDE[id];
    if (!block) continue;
    lines.push(`${step}. ${block[0]}`);
    for (const line of block.slice(1)) lines.push(line);
    step += 1;
  }
  const shown = ides.map((id) => LCD_LABEL[id]).filter(Boolean).join(" or ");
  lines.push(
    `${step}. After a finished turn the LCD shows ${shown} and the remaining percent with two decimals.`,
  );
  return lines;
}

/** @type {Record<string, string[]>} */
const GUIDE = {
  cursor: [
    "Cursor sign-in. Tiny Engineer does not ask for a token.",
    "   - Open Cursor, sign in under Settings → Account, then quit Cursor and open it again.",
    '   - The hook reads the session already on this machine: CURSOR_SESSION_TOKEN if you set it, otherwise the macOS Keychain item "cursor-access-token", otherwise cursorAuth/accessToken in the Cursor or Cursor Nightly state database.',
    "   - An API key is not a web session. If the LCD stays blank, sign in again in Cursor. Only if that login is still missing, set CURSOR_SESSION_TOKEN to the id::jwt session cookie. Do not commit it.",
  ],
  "claude-code": [
    "Claude Code sign-in. Use /login, not an API key.",
    "   - In Claude Code run /login and finish the browser sign-in. This has to be a Claude.ai subscription login.",
    '   - On macOS the hook reads the Keychain item "Claude Code-credentials". claudeAiOauth.accessToken must be a non-empty string. If that is missing, it reads ~/.claude/.credentials.json with the same field. Tiny Engineer only reads the login. It does not refresh it and it does not write it.',
    "   - In Claude Code run /cost. If that shows usage, the next finished turn (Stop) writes 100 minus the used percent to the LCD.",
    "   - If /login succeeded but the screen stays blank, run /login again. An empty access token is ignored. An API-key login has no plan windows, so the screen will not update.",
  ],
};
