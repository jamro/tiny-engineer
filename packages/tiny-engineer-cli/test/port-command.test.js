import assert from "node:assert/strict";
import test from "node:test";

import { formatPortScan, probeCandidates, runPort } from "../src/commands/port.js";
import { createOutput } from "../src/util/output.js";

const ports = [
  { path: "/dev/tty.Bluetooth-Incoming-Port", manufacturer: "Bluetooth" },
  { path: "/dev/tty.usbmodem142201", vendorId: "303A", manufacturer: "Espressif" },
  { path: "/dev/ttyUSB0", manufacturer: "FTDI" },
];

test("formatPortScan names the autodetect winner and skips score 0", () => {
  const text = formatPortScan({ ports, envPort: null, cachedPort: null });
  assert.match(text, /env: \(unset\)/);
  assert.match(text, /cache: \(none\)/);
  assert.match(text, /open: \/dev\/tty\.usbmodem142201 \(autodetect\)/);
  assert.match(text, /autodetect: \/dev\/tty\.usbmodem142201 score 150/);
  assert.match(text, /tty\.usbmodem142201 {2}score 150 {2}vid 303a, cdc, serial path/);
  assert.match(text, /ttyUSB0 {2}score 10 {2}serial path/);
  assert.match(text, /Bluetooth-Incoming-Port {2}score 0 {2}skipped/);
});

test("formatPortScan prefers env over cache and still reports autodetect", () => {
  const text = formatPortScan({
    ports,
    envPort: "/dev/cu.explicit",
    cachedPort: "/dev/cu.cached",
  });
  assert.match(text, /open: \/dev\/cu\.explicit \(env\)/);
  assert.match(text, /cache: \/dev\/cu\.cached/);
  assert.match(text, /autodetect: \/dev\/tty\.usbmodem142201 score 150/);
});

test("formatPortScan uses the cache when env is unset", () => {
  const text = formatPortScan({ ports, envPort: null, cachedPort: "/dev/cu.cached" });
  assert.match(text, /open: \/dev\/cu\.cached \(cache\)/);
});

test("probeCandidates matches env-only then cache then autodetect order", () => {
  assert.deepEqual(
    probeCandidates({ ports, envPort: "/dev/cu.explicit", cachedPort: "/dev/cu.cached" }),
    [{ path: "/dev/cu.explicit", source: "env" }],
  );
  assert.deepEqual(
    probeCandidates({ ports, envPort: null, cachedPort: "/dev/cu.cached" }).map((row) => row.path),
    ["/dev/cu.cached", "/dev/tty.usbmodem142201", "/dev/ttyUSB0"],
  );
});

test("port --probe reports sync via daemon helper and does not open locally", async () => {
  /** @type {string[]} */
  const lines = [];
  /** @type {string[][]} */
  const probed = [];
  const prev = process.exitCode;
  process.exitCode = 0;
  await runPort(createOutput({ log() {}, error() {} }), ["--probe"], {
    list: async () => ports,
    envPort: null,
    cachedPort: "/dev/cu.cached",
    write: (text) => lines.push(text),
    async probe(candidates) {
      probed.push(candidates);
      return {
        ok: true,
        results: [
          { path: "/dev/cu.cached", ok: false, detail: "open failed" },
          { path: "/dev/tty.usbmodem142201", ok: true, detail: "sync ok" },
        ],
        port: "/dev/tty.usbmodem142201",
      };
    },
  });
  assert.deepEqual(probed, [
    ["/dev/cu.cached", "/dev/tty.usbmodem142201", "/dev/ttyUSB0"],
  ]);
  const probe = lines.join("");
  assert.match(probe, /probe \/dev\/cu\.cached \(cache\) open failed/);
  assert.match(probe, /probe \/dev\/tty\.usbmodem142201 \(autodetect\) sync ok/);
  assert.equal(process.exitCode ?? 0, 0);
  process.exitCode = prev;
});

test("port rejects an unknown argument", async () => {
  /** @type {string[]} */
  const errors = [];
  const prev = process.exitCode;
  await runPort(createOutput({ log() {}, error: (m) => errors.push(String(m)) }), ["--nope"], {
    list: async () => [],
  });
  assert.match(errors.join("\n"), /unknown port argument --nope/);
  assert.equal(process.exitCode, 1);
  process.exitCode = prev;
});
