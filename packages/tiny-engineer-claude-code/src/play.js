import { robotFetch } from "./post.js";

// The robot answers once the clip has finished playing.
const PLAY_TIMEOUT_MS = 10 * 60 * 1000;

/**
 * @param {string[]} argv arguments after `play`
 * @returns {{ wavPath?: string, name?: string, url?: string, error?: string }}
 */
export function parsePlayArgs(argv) {
  /** @type {{ wavPath?: string, name?: string, url?: string }} */
  const opts = {};

  for (let i = 0; i < argv.length; i++) {
    const arg = argv[i];

    if (arg === "--name" || arg === "--url") {
      const value = argv[++i];
      if (!value || value.startsWith("-")) return { error: `${arg} requires a value` };
      opts[arg.slice(2)] = value;
      continue;
    }
    if (arg.startsWith("-") || opts.wavPath) return { error: `Unknown argument: ${arg}` };
    opts.wavPath = arg;
  }

  if (!opts.wavPath) return { error: "play needs a WAV file" };
  return opts;
}

/**
 * POST a WAV to /play (or /play/<name>). Resolves with the robot's reply.
 * @param {{ baseUrl: string, wav: Uint8Array, name?: string, token?: string | null }} request
 * @returns {Promise<{ ok: boolean, body: string }>}
 */
export async function postPlay({ baseUrl, wav, name, token }) {
  const response = await robotFetch(
    baseUrl,
    name ? `/play/${encodeURIComponent(name)}` : "/play",
    {
      method: "POST",
      headers: { "Content-Type": "audio/wav" },
      body: wav,
      signal: AbortSignal.timeout(PLAY_TIMEOUT_MS),
    },
    token,
  );

  return { ok: response.ok, body: await response.text() };
}
