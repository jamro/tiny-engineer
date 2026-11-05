import test from "node:test";
import assert from "node:assert/strict";
import { EventEmitter } from "node:events";
import { mkdtempSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

import {
  connectSerialRobot,
  darwinTtyPath,
  hostCalloutPath,
  kickDarwinTty,
  rankSerialPorts,
  SerialRobotClient,
  syncAfterOpen,
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

test("hostCalloutPath rewrites macOS tty nodes only", () => {
  assert.equal(hostCalloutPath("/dev/tty.usbmodem142201", "darwin"), "/dev/cu.usbmodem142201");
  assert.equal(hostCalloutPath("/dev/cu.usbmodem142201", "darwin"), "/dev/cu.usbmodem142201");
  assert.equal(hostCalloutPath("/dev/ttyACM0", "linux"), "/dev/ttyACM0");
  assert.equal(hostCalloutPath("/dev/tty.usbmodem142201", "linux"), "/dev/tty.usbmodem142201");
});

test("kickDarwinTty opens the tty sibling and swallows ENXIO", () => {
  assert.equal(darwinTtyPath("/dev/cu.usbmodem142201"), "/dev/tty.usbmodem142201");
  assert.equal(darwinTtyPath("/dev/ttyACM0"), null);

  /** @type {string[]} */
  const opened = [];
  const enxio = Object.assign(new Error("Device not configured"), { code: "ENXIO" });
  kickDarwinTty("/dev/cu.usbmodem142201", {
    openFile(path) {
      opened.push(path);
      throw enxio;
    },
  });
  assert.deepEqual(opened, ["/dev/tty.usbmodem142201"]);

  /** @type {number[]} */
  const closed = [];
  kickDarwinTty("/dev/cu.usbmodem1", {
    openFile() {
      return 7;
    },
    closeFile(fd) {
      closed.push(fd);
    },
  });
  assert.deepEqual(closed, [7]);
});

test("syncAfterOpen retries a dropped first frame", async () => {
  let calls = 0;
  const result = await syncAfterOpen({
    async sync() {
      calls += 1;
      if (calls === 1) throw new Error("serial request timed out (op=sync)");
      return { proto: 1 };
    },
  });
  assert.equal(result.proto, 1);
  assert.equal(calls, 2);
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

test("connectSerialRobot kicks once after a dead sync and skips a healthy one", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-cache-"));
  const cachePath = join(dir, "serial-port.json");
  const prevPort = process.env.TINY_ENGINEER_PORT;
  delete process.env.TINY_ENGINEER_PORT;

  try {
    /** @type {string[]} */
    const kicks = [];
    let syncs = 0;
    const recovered = await connectSerialRobot({
      port: "/dev/cu.usbmodem142201",
      cachePath,
      kick(path) {
        kicks.push(path);
      },
      openClient(path) {
        return {
          portPath: path,
          async open() {},
          async close() {},
          async sync() {
            syncs += 1;
            // syncAfterOpen spends three attempts before openSynced may kick.
            if (syncs <= 3) throw new Error("serial request timed out (op=sync)");
            return { proto: 1 };
          },
        };
      },
    });
    assert.deepEqual(kicks, ["/dev/cu.usbmodem142201"]);
    assert.equal(syncs, 4);
    assert.equal(recovered.source, "flag");
    assert.equal(readPortCache(cachePath), "/dev/cu.usbmodem142201");
    await recovered.client.close();

    let healthyKicks = 0;
    const healthy = await connectSerialRobot({
      port: "/dev/cu.usbmodem142201",
      cachePath,
      kick() {
        healthyKicks += 1;
      },
      openClient(path) {
        return {
          portPath: path,
          async open() {},
          async close() {},
          async sync() {
            return { proto: 1 };
          },
        };
      },
    });
    assert.equal(healthyKicks, 0);
    await healthy.client.close();
  } finally {
    if (prevPort === undefined) delete process.env.TINY_ENGINEER_PORT;
    else process.env.TINY_ENGINEER_PORT = prevPort;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("autodetect kicks a likely robot and still kicks a lone adapter", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-cache-"));
  const cachePath = join(dir, "serial-port.json");
  const prevPort = process.env.TINY_ENGINEER_PORT;
  delete process.env.TINY_ENGINEER_PORT;

  /**
   * @param {string} path
   */
  const deadClient = (path) => ({
    portPath: path,
    async open() {},
    async close() {},
    async sync() {
      throw new Error("serial request timed out (op=sync)");
    },
  });

  try {
    /** @type {string[]} */
    const kicks = [];
    await assert.rejects(
      () => connectSerialRobot({
        cachePath,
        kick(path) {
          kicks.push(path);
        },
        list: async () => [
          { path: "/dev/cu.usbmodem1", vendorId: "303A", manufacturer: "Espressif" },
          { path: "/dev/cu.usbserial1", manufacturer: "FTDI" },
        ],
        openClient: deadClient,
      }),
      /serial request timed out \(op=sync\)/,
    );
    assert.deepEqual(kicks, ["/dev/cu.usbmodem1"]);

    /** @type {string[]} */
    const lone = [];
    await assert.rejects(
      () => connectSerialRobot({
        cachePath,
        kick(path) {
          lone.push(path);
        },
        list: async () => [{ path: "/dev/cu.usbserial1", manufacturer: "FTDI" }],
        openClient: deadClient,
      }),
      /serial request timed out \(op=sync\)/,
    );
    assert.deepEqual(lone, ["/dev/cu.usbserial1"]);
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

test("connectSerialRobot writeCache false skips cache write", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-cache-"));
  const cachePath = join(dir, "serial-port.json");
  const prevPort = process.env.TINY_ENGINEER_PORT;
  delete process.env.TINY_ENGINEER_PORT;

  try {
    const result = await connectSerialRobot({
      port: "/dev/cu.probe",
      cachePath,
      writeCache: false,
      list: async () => [],
      openClient: (path) => mockClientFactory(path),
    });
    assert.equal(result.port, "/dev/cu.probe");
    assert.equal(readPortCache(cachePath), null);
    await result.client.close();
  } finally {
    if (prevPort === undefined) delete process.env.TINY_ENGINEER_PORT;
    else process.env.TINY_ENGINEER_PORT = prevPort;
    rmSync(dir, { recursive: true, force: true });
  }
});
