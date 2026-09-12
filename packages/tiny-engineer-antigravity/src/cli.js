import { getBaseUrl, getToken, loadDotEnv } from "./env.js";
import { animationForEvent } from "./map.js";
import { postAnim } from "./post.js";

function readStdin() {
  return new Promise((resolve) => {
    if (process.stdin.isTTY) {
      resolve("");
      return;
    }
    const chunks = [];
    process.stdin.setEncoding("utf8");
    process.stdin.on("data", (c) => chunks.push(c));
    process.stdin.on("end", () => resolve(chunks.join("")));
    process.stdin.on("error", () => resolve(""));
  });
}

/**
 * Determine the JSON response required by Antigravity's hook contract.
 * @param {string} hookType
 * @returns {Record<string, any>}
 */
function defaultResponseForHook(hookType) {
  if (hookType === "PreToolUse") {
    return { decision: "allow" };
  }
  if (hookType === "Stop") {
    return { decision: "allow" };
  }
  return {};
}

/**
 * @param {string[]} argv
 */
export async function run(argv) {
  loadDotEnv();

  const hookType = argv[2] || "unknown";
  const rawStdin = await readStdin();

  let payload = {};
  if (rawStdin.trim()) {
    try {
      payload = JSON.parse(rawStdin);
    } catch {
      // Ignore parse failure; proceed with empty payload
    }
  }

  const anim = animationForEvent(hookType, payload);
  if (anim) {
    await postAnim(getBaseUrl(), anim, getToken());
  }

  // Always output the required Antigravity contract response on stdout
  const response = defaultResponseForHook(hookType);
  console.log(JSON.stringify(response));
  process.exitCode = 0;
}
