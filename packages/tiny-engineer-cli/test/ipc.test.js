import assert from "node:assert/strict";
import { createConnection, createServer } from "node:net";
import test from "node:test";

import { createPeer } from "../src/daemon/ipc.js";

/**
 * @returns {Promise<{
 *   peer: ReturnType<typeof createPeer>,
 *   remote: import("node:net").Socket,
 *   close: () => Promise<void>,
 * }>}
 */
async function pairedPeer() {
  const server = createServer();
  await new Promise((resolve) => {
    server.listen(0, "127.0.0.1", resolve);
  });
  const addr = server.address();
  assert.ok(addr && typeof addr === "object");
  /** @type {import("node:net").Socket} */
  let remote;
  const remoteReady = new Promise((resolve) => {
    server.once("connection", (socket) => {
      remote = socket;
      resolve(undefined);
    });
  });
  const local = createConnection({ host: "127.0.0.1", port: addr.port });
  await Promise.all([
    remoteReady,
    new Promise((resolve, reject) => {
      local.once("connect", resolve);
      local.once("error", reject);
    }),
  ]);
  const peer = createPeer(local);
  return {
    peer,
    get remote() {
      return remote;
    },
    async close() {
      local.destroy();
      remote?.destroy();
      await new Promise((resolve) => server.close(resolve));
    },
  };
}

test("request rejects promptly when the socket closes", async () => {
  const pair = await pairedPeer();
  try {
    const pending = pair.peer.request("ping", {}, 60_000);
    const started = Date.now();
    pair.remote.destroy();
    await assert.rejects(pending, /daemon connection closed/);
    assert.ok(Date.now() - started < 2000);
  } finally {
    await pair.close();
  }
});

test("request times out when the socket stays open", async () => {
  const pair = await pairedPeer();
  try {
    await assert.rejects(
      () => pair.peer.request("ping", {}, 50),
      /daemon request timed out \(ping\)/,
    );
  } finally {
    await pair.close();
  }
});
