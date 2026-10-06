import test from "node:test";
import assert from "node:assert/strict";
import { createServer } from "node:http";
import { parsePlayArgs, postPlay } from "../src/play.js";

test("play reads the WAV, animation name and URL", () => {
  assert.deepEqual(parsePlayArgs(["clip.wav", "--name", "thinking", "--url", "http://robot"]), {
    wavPath: "clip.wav",
    name: "thinking",
    url: "http://robot",
  });
});

test("play rejects bad arguments before contacting the robot", () => {
  assert.equal(parsePlayArgs(["--name", "talking"]).error, "play needs a WAV file");
  assert.equal(parsePlayArgs(["clip.wav", "--name"]).error, "--name requires a value");
  assert.equal(parsePlayArgs(["clip.wav", "--name", "--url", "http://robot"]).error, "--name requires a value");
  assert.equal(parsePlayArgs(["a.wav", "b.wav"]).error, "Unknown argument: b.wav");
});

test("postPlay streams the WAV to the named animation's route with the token", async () => {
  let received;
  const server = createServer((req, res) => {
    const chunks = [];
    req.on("data", (chunk) => chunks.push(chunk));
    req.on("end", () => {
      received = { url: req.url, headers: req.headers, body: Buffer.concat(chunks) };
      res.writeHead(200, { "Content-Type": "application/json" });
      res.end('{"ok":true,"played_ms":10}');
    });
  });
  await new Promise((resolve) => server.listen(0, "127.0.0.1", resolve));
  const { port } = server.address();

  try {
    const reply = await postPlay({
      baseUrl: `http://127.0.0.1:${port}/`,
      wav: Buffer.from("RIFFdata"),
      name: "thinking",
      token: "secret",
    });

    assert.deepEqual(reply, { ok: true, body: '{"ok":true,"played_ms":10}' });
    assert.equal(received.url, "/play/thinking");
    assert.equal(received.headers.authorization, "Bearer secret");
    assert.equal(received.body.toString(), "RIFFdata");
  } finally {
    server.close();
  }
});
