import test from "node:test";
import assert from "node:assert/strict";
import { EventEmitter } from "node:events";
import { mkdtempSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

import {
  connectSerialRobot,
  rankSerialPorts,
  SerialRobotClient,
} from "../src/robot/serial/client.js";
import { cobsDecodeFrame, cobsEncodeFrame } from "../src/robot/serial/cobs.js";
import {
  clearPortCache,
  readPortCache,
  writePortCache,
} from "../src/robot/serial/port-cache.js";
import { packMessage, parseMessage, SerialMsgType } from "../src/robot/serial/protocol.js";

test("port cache hit / miss / clear", () => {
  const dir = mkdtempSync(join(tmpdir(), "te-cache-"));
  const cachePath = join(dir, "serial-port.json");
  try {
    assert.equal(readPortCache(cachePath), null);
    writePortCache("/dev/cu.usbmodem1", cachePath);
    assert.equal(readPortCache(cachePath), "/dev/cu.usbmodem1");
    clearPortCache(cachePath);
    assert.equal(readPortCache(cachePath), null);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("rankSerialPorts prefers Espressif VID", () => {
  const ranked = rankSerialPorts([
    { path: "/dev/tty.Bluetooth", manufacturer: "Bluetooth" },
    { path: "/dev/cu.usbmodem1", vendorId: "303A", manufacturer: "Espressif" },
    { path: "/dev/ttyUSB0", manufacturer: "FTDI" },
  ]);
  assert.equal(ranked[0], "/dev/cu.usbmodem1");
});

/**
 * @param {string} path
 * @param {{ failOpen?: boolean }} [opts]
 */
function mockClientFactory(path, { failOpen = false } = {}) {
  return new SerialRobotClient(path, {
    openPort: () => {
      const port = new EventEmitter();
      port.isOpen = false;
      port.open = (cb) => {
        if (failOpen) {
          cb(new Error("open failed"));
          return;
        }
        port.isOpen = true;
        cb(null);
      };
      port.close = (cb) => {
        port.isOpen = false;
        cb();
      };
      port.write = (frame, cb) => {
        queueMicrotask(() => {
          const decoded = cobsDecodeFrame(frame);
          const msg = parseMessage(decoded);
          if (!msg) return;
          const body = Buffer.from(
            JSON.stringify({ proto: 1, provisioning: false, link_mode: "usb" }),
            "utf8",
          );
          const reply = packMessage(SerialMsgType.Sync, msg.id, body);
          port.emit("data", cobsEncodeFrame(reply));
        });
        cb(null);
      };
      return /** @type {any} */ (port);
    },
  });
}

test("connectSerialRobot uses cache hit then invalidates on fail", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-cache-"));
  const cachePath = join(dir, "serial-port.json");
  const prevPort = process.env.TINY_ENGINEER_PORT;
  delete process.env.TINY_ENGINEER_PORT;

  try {
    writePortCache("/dev/cached", cachePath);

    const hit = await connectSerialRobot({
      cachePath,
      list: async () => [],
      openClient: (path) => mockClientFactory(path),
    });
    assert.equal(hit.source, "cache");
    assert.equal(hit.port, "/dev/cached");
    await hit.client.close();

    writePortCache("/dev/stale", cachePath);
    await assert.rejects(
      () =>
        connectSerialRobot({
          cachePath,
          list: async () => [],
          openClient: (path) => mockClientFactory(path, { failOpen: true }),
        }),
      /no Tiny Engineer serial ports found/,
    );
    assert.equal(readPortCache(cachePath), null);
  } finally {
    if (prevPort === undefined) delete process.env.TINY_ENGINEER_PORT;
    else process.env.TINY_ENGINEER_PORT = prevPort;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("connectSerialRobot autodetects when cache miss", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-cache-"));
  const cachePath = join(dir, "serial-port.json");
  const prevPort = process.env.TINY_ENGINEER_PORT;
  delete process.env.TINY_ENGINEER_PORT;

  try {
    const result = await connectSerialRobot({
      cachePath,
      list: async () => [{ path: "/dev/cu.usbmodemTE", vendorId: "303A", manufacturer: "Espressif" }],
      openClient: (path) => mockClientFactory(path),
    });
    assert.equal(result.source, "autodetect");
    assert.equal(result.port, "/dev/cu.usbmodemTE");
    assert.equal(readPortCache(cachePath), "/dev/cu.usbmodemTE");
    await result.client.close();
  } finally {
    if (prevPort === undefined) delete process.env.TINY_ENGINEER_PORT;
    else process.env.TINY_ENGINEER_PORT = prevPort;
    rmSync(dir, { recursive: true, force: true });
  }
});
