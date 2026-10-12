import test from "node:test";
import assert from "node:assert/strict";
import { mkdtempSync, readFileSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import {
  clearSerialCache,
  readSerialCache,
  setSerialCacheFileForTests,
  writeSerialCache,
} from "../src/robot/serial-cache.js";
import {
  detectSerialPort,
  filterSerialCandidates,
  looksLikeCdcPath,
  normalizePortPath,
  preferCalloutPath,
} from "../src/robot/serial-detect.js";

test("preferCalloutPath maps macOS tty. to cu.; leaves Linux ACM alone", () => {
  assert.equal(preferCalloutPath("/dev/tty.usbmodem1"), "/dev/cu.usbmodem1");
  assert.equal(preferCalloutPath("/dev/cu.usbmodem1"), "/dev/cu.usbmodem1");
  assert.equal(preferCalloutPath("/dev/ttyACM0"), "/dev/ttyACM0");
  assert.equal(preferCalloutPath("COM3"), "COM3");
});

test("normalizePortPath uppercases Windows COM", () => {
  assert.equal(normalizePortPath("com12"), "COM12");
  assert.equal(normalizePortPath("/dev/tty.usbmodem1"), "/dev/cu.usbmodem1");
});

test("looksLikeCdcPath accepts COM / usbmodem / ttyACM; rejects bluetooth", () => {
  assert.equal(looksLikeCdcPath("COM3"), true);
  assert.equal(looksLikeCdcPath("com12"), true);
  assert.equal(looksLikeCdcPath("/dev/cu.usbmodem1142201"), true);
  assert.equal(looksLikeCdcPath("/dev/ttyACM0"), true);
  assert.equal(looksLikeCdcPath("/dev/ttyACM12"), true);
  assert.equal(looksLikeCdcPath("/dev/cu.Bluetooth-Incoming-Port"), false);
  assert.equal(looksLikeCdcPath("/dev/tty.debug-console"), false);
  assert.equal(looksLikeCdcPath("/dev/ttyUSB0"), false);
});

test("filterSerialCandidates prefers Espressif VID", () => {
  const ports = [
    { path: "/dev/cu.Bluetooth-Incoming-Port" },
    { path: "/dev/cu.usbmodem1", vendorId: "303A", productId: "1001" },
    { path: "/dev/tty.usbmodem1", vendorId: "303A", productId: "1001" },
    { path: "/dev/cu.usbserial-xyz", vendorId: "10C4" },
  ];
  const filtered = filterSerialCandidates(ports);
  assert.deepEqual(
    filtered.map((p) => p.path),
    ["/dev/cu.usbmodem1"],
  );
});

test("filterSerialCandidates maps macOS tty.usbmodem to cu.", () => {
  const filtered = filterSerialCandidates([
    { path: "/dev/tty.debug-console" },
    { path: "/dev/tty.usbmodem1142201", vendorId: "303a", productId: "1001" },
  ]);
  assert.deepEqual(
    filtered.map((p) => p.path),
    ["/dev/cu.usbmodem1142201"],
  );
});

test("filterSerialCandidates keeps Linux ttyACM Espressif", () => {
  const filtered = filterSerialCandidates([
    { path: "/dev/ttyS0" },
    { path: "/dev/ttyUSB0", vendorId: "10C4" },
    { path: "/dev/ttyACM0", vendorId: "303A", productId: "1001" },
  ]);
  assert.deepEqual(
    filtered.map((p) => p.path),
    ["/dev/ttyACM0"],
  );
});

test("filterSerialCandidates keeps Windows COM Espressif", () => {
  const filtered = filterSerialCandidates([
    { path: "COM1" },
    { path: "com5", vendorId: "0x303A", productId: "0x1001" },
    { path: "COM3", manufacturer: "Bluetooth" },
  ]);
  assert.deepEqual(
    filtered.map((p) => p.path),
    ["COM5"],
  );
});

test("filterSerialCandidates matches manufacturer when VID missing", () => {
  const filtered = filterSerialCandidates([
    { path: "COM8", manufacturer: "Espressif" },
    { path: "COM9", friendlyName: "USB JTAG/serial debug unit (COM9)" },
    { path: "COM1" },
  ]);
  assert.deepEqual(
    filtered.map((p) => p.path).sort(),
    ["COM8", "COM9"],
  );
});

test("filterSerialCandidates falls back to CDC paths", () => {
  const ports = [
    { path: "/dev/cu.usbmodem99" },
    { path: "/dev/cu.Bluetooth-Incoming-Port" },
    { path: "COM3" },
    { path: "/dev/ttyACM1" },
  ];
  const filtered = filterSerialCandidates(ports);
  assert.deepEqual(
    filtered.map((p) => p.path).sort(),
    ["/dev/cu.usbmodem99", "/dev/ttyACM1", "COM3"].sort(),
  );
});

test("filterSerialCandidates ranks USB-Serial/JTAG PID ahead of other Espressif", () => {
  const filtered = filterSerialCandidates([
    { path: "/dev/ttyACM1", vendorId: "303A", productId: "0002" },
    { path: "/dev/ttyACM0", vendorId: "303A", productId: "1001" },
  ]);
  assert.equal(filtered[0].path, "/dev/ttyACM0");
});

test("detectSerialPort returns unique health match and writes cache", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-serial-cache-"));
  const cacheFile = join(dir, "serial-port.json");
  setSerialCacheFileForTests(cacheFile);
  clearSerialCache();

  try {
    const path = await detectSerialPort({
      skipCache: true,
      listFn: async () => [
        { path: "/dev/cu.usbmodem-a", vendorId: "303A" },
        { path: "/dev/cu.usbmodem-b", vendorId: "303A" },
      ],
      requestFn: async (p) => {
        if (p === "/dev/cu.usbmodem-b") {
          return { ok: true, status: 200, body: '{"ok":true,"version":"v1"}' };
        }
        return { ok: false, error: "robot unreachable: timeout" };
      },
    });
    assert.equal(path, "/dev/cu.usbmodem-b");
    assert.equal(readSerialCache(), "/dev/cu.usbmodem-b");
    const disk = JSON.parse(readFileSync(cacheFile, "utf8"));
    assert.equal(disk.path, "/dev/cu.usbmodem-b");
  } finally {
    setSerialCacheFileForTests(undefined);
    rmSync(dir, { recursive: true, force: true });
  }
});

test("detectSerialPort uses cache hit without scanning others", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-serial-cache-"));
  setSerialCacheFileForTests(join(dir, "serial-port.json"));
  writeSerialCache("/dev/cu.cached");

  let listCalls = 0;
  let probed = [];
  try {
    const path = await detectSerialPort({
      listFn: async () => {
        listCalls++;
        return [{ path: "/dev/cu.cached", vendorId: "303A" }];
      },
      requestFn: async (p) => {
        probed.push(p);
        return { ok: true, status: 200, body: '{"ok":true}' };
      },
    });
    assert.equal(path, "/dev/cu.cached");
    assert.equal(listCalls, 1);
    assert.deepEqual(probed, ["/dev/cu.cached"]);
  } finally {
    setSerialCacheFileForTests(undefined);
    rmSync(dir, { recursive: true, force: true });
  }
});

test("detectSerialPort cache hit via macOS tty list path", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-serial-cache-"));
  setSerialCacheFileForTests(join(dir, "serial-port.json"));
  writeSerialCache("/dev/cu.usbmodem1142201");

  try {
    const path = await detectSerialPort({
      listFn: async () => [
        { path: "/dev/tty.usbmodem1142201", vendorId: "303A", productId: "1001" },
      ],
      requestFn: async (p) => {
        if (p === "/dev/cu.usbmodem1142201") {
          return { ok: true, status: 200, body: '{"ok":true}' };
        }
        return { ok: false, error: "robot unreachable: wrong path" };
      },
    });
    assert.equal(path, "/dev/cu.usbmodem1142201");
  } finally {
    setSerialCacheFileForTests(undefined);
    rmSync(dir, { recursive: true, force: true });
  }
});

test("detectSerialPort clears bad cache and rediscovers", async () => {
  const dir = mkdtempSync(join(tmpdir(), "te-serial-cache-"));
  setSerialCacheFileForTests(join(dir, "serial-port.json"));
  writeSerialCache("/dev/cu.stale");

  try {
    const path = await detectSerialPort({
      listFn: async () => [
        { path: "/dev/cu.stale", vendorId: "303A" },
        { path: "/dev/cu.fresh", vendorId: "303A" },
      ],
      requestFn: async (p) => {
        if (p === "/dev/cu.fresh") {
          return { ok: true, status: 200, body: '{"ok":true}' };
        }
        return { ok: false, error: "robot unreachable: gone" };
      },
    });
    assert.equal(path, "/dev/cu.fresh");
    assert.equal(readSerialCache(), "/dev/cu.fresh");
  } finally {
    setSerialCacheFileForTests(undefined);
    rmSync(dir, { recursive: true, force: true });
  }
});

test("detectSerialPort errors on zero or multiple matches", async () => {
  await assert.rejects(
    () =>
      detectSerialPort({
        skipCache: true,
        listFn: async () => [{ path: "/dev/cu.usbmodem-x", vendorId: "303A" }],
        requestFn: async () => ({ ok: false, error: "robot unreachable: x" }),
      }),
    /no Tiny Engineer reply on/,
  );

  await assert.rejects(
    () =>
      detectSerialPort({
        skipCache: true,
        listFn: async () => [],
        requestFn: async () => ({ ok: false, error: "x" }),
      }),
    /no Tiny Engineer serial port found/,
  );

  await assert.rejects(
    () =>
      detectSerialPort({
        skipCache: true,
        listFn: async () => [
          { path: "/dev/cu.a", vendorId: "303A" },
          { path: "/dev/cu.b", vendorId: "303A" },
        ],
        requestFn: async () => ({ ok: true, status: 200, body: '{"ok":true}' }),
      }),
    /multiple Tiny Engineer ports/,
  );
});
