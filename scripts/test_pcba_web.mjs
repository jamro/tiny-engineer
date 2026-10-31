import assert from "node:assert/strict";
import { readdir, readFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

import {
  PYTHON_COMMAND,
  canTest,
  createHost,
  PORT_LOST,
  activatePort,
  releasePort,
  display,
  encodeKey,
  acceptKeyEvent,
  keyToken,
  feed,
  grant,
  connectTarget,
  prepText,
  splitCard,
  onDisconnect,
  onPortReturn,
} from "../web/js/pcba-test.mjs";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const webRoot = path.join(root, "web");

function assertGate() {
  assert.equal(
    canTest({ isSecureContext: true, navigator: {} }),
    false
  );
  assert.equal(
    canTest({ isSecureContext: false, navigator: { serial: {} } }),
    false
  );
  assert.equal(
    canTest({ isSecureContext: true, navigator: { serial: {} } }),
    true
  );
}

async function walkHtml(dir, found = []) {
  const entries = await readdir(dir, { withFileTypes: true });
  for (const entry of entries) {
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      await walkHtml(full, found);
    } else if (entry.name.endsWith(".html")) {
      found.push(full);
    }
  }
  return found;
}

async function assertNoInboundLinks() {
  const files = await walkHtml(webRoot);
  const page = path.join(webRoot, "pcba_test", "index.html");
  for (const file of files) {
    if (file === page) {
      continue;
    }
    const text = await readFile(file, "utf8");
    assert.equal(text.includes("pcba_test"), false, file);
  }
  const html = await readFile(page, "utf8");
  assert.match(html, /Compatible with main control board rev 3/);
  assert.match(html, /The ESP32 must already run the test firmware/);
  assert.match(
    html,
    /https:\/\/github.com\/jamro\/tiny-engineer\/blob\/main\/docs\/hardware\/pcba-test\.md#get-an-esp32-with-the-test-firmware/
  );
  assert.match(html, /This browser cannot run the test/);
  assert.match(
    html,
    /https:\/\/github.com\/jamro\/tiny-engineer\/blob\/main\/docs\/hardware\/pcba-test\.md/
  );
  assert.match(html, new RegExp(PYTHON_COMMAND.replace(/[.]/g, "\\.")));
  assert.match(html, /id="connect"[^>]*hidden/);
  assert.match(html, /id="disconnect"[^>]*hidden/);
  assert.equal(html.includes("Save CSV"), false);
  assert.equal(html.includes('id="save"'), false);
}

function assertSession() {
  const host = createHost();
  grant(host);
  feed(host, "READY");
  assert.match(display(host), /CONNECTED/);
  assert.match(display(host), /The board is connected/);
  assert.match(display(host), /Enter = continue/);
  assert.doesNotMatch(display(host), /speaker/i);
  assert.doesNotMatch(display(host), /1\/3/);
  feed(host, "PCBA-TEST v1");
  assert.match(display(host), /The module LED stays steady white while the test image has power/);
  assert.match(display(host), /That lamp is not the board verdict/);
  assert.match(display(host), /PCBA-TEST v1/);
  feed(host, "STEP tone");
  assert.match(display(host), /1\/3/);
  assert.match(display(host), /SPEAKER/);
  assert.match(display(host), /Speaker on the JST speaker port/);
  assert.doesNotMatch(display(host), /Lamps do not count/);
  assert.match(display(host), /Expect a repeating tone/);
  assert.match(display(host), /Volume 6\/10/);
  assert.match(display(host), /\+ = louder/);
  assert.match(display(host), /- = quieter/);
  assert.match(display(host), /Enter = good/);
  assert.match(display(host), /n = bad/);
  assert.doesNotMatch(display(host), /STEP tone/);
  feed(host, "STEP servos");
  assert.match(display(host), /2\/3/);
  assert.match(display(host), /SERVOS/);
  assert.doesNotMatch(display(host), /\+ = louder/);
  assert.doesNotMatch(display(host), /Volume/);
  assert.match(display(host), /Signal is on all five servo outputs/);
  assert.match(display(host), /Plug it into one column at a time/);
  assert.match(display(host), /Columns are HEAD, NECK, ARM L, ARM R, and BODY/);
  assert.match(display(host), /Check the LEDs on every column/);
  assert.match(display(host), /Red stays on and yellow pulses/);
  assert.doesNotMatch(display(host), /No key between columns/);
  assert.doesNotMatch(display(host), /STEP servos/);
  feed(host, "STEP oe");
  assert.match(display(host), /3\/3/);
  assert.match(display(host), /OE OUTPUT ENABLE/);
  assert.match(display(host), /The red LED stays on/);
  assert.match(display(host), /Only the yellow LED goes out/);
  assert.match(display(host), /No need to test each servo column/);
  assert.doesNotMatch(display(host), /PWM/);
  assert.doesNotMatch(display(host), /5 V/);
  assert.doesNotMatch(display(host), /STEP oe/);
  assert.equal(keyToken("Enter"), "enter");
  assert.equal(encodeKey(keyToken("Enter")), "\n");
  assert.equal(encodeKey(keyToken("+")), "+");
  assert.equal(encodeKey(keyToken("n")), "n");
  assert.equal(encodeKey("enter"), "\n");
  assert.equal(encodeKey("n"), "n");
  assert.equal(encodeKey("+"), "+");
  assert.equal(encodeKey("="), "+");
  assert.equal(encodeKey("-"), "-");
  assert.equal(acceptKeyEvent({ key: "Enter", repeat: true }), "");
  assert.equal(acceptKeyEvent({ key: "a", ctrlKey: true }), "");
  assert.equal(acceptKeyEvent({ key: "a", metaKey: true }), "");
  assert.equal(acceptKeyEvent({ key: "r", altKey: true }), "");
  assert.equal(acceptKeyEvent({ key: "Enter" }), "\n");
  assert.equal(acceptKeyEvent({ key: "a" }), "a");
  assert.equal(acceptKeyEvent({ key: "+", shiftKey: true }), "+");
  feed(host, "STEP tone");
  feed(host, "VOL 8");
  assert.match(display(host), /Volume 8\/10/);
  feed(host, "FAIL servos");
  assert.match(display(host), /Red stays on and yellow pulses/);
  assert.match(display(host), /r = try this step again/);
  assert.match(display(host), /c = keep the fail, go on/);
  assert.match(display(host), /a = start this board over/);
  assert.match(display(host), /FAIL servos/);
  feed(host, "FAIL pca");
  assert.match(display(host), /PCA9685 did not answer at I2C 0x40/);
  assert.match(display(host), /this board stops before the speaker test/);
  assert.match(display(host), /3\.3 V, not 5 V/);
  assert.doesNotMatch(display(host), /r = try this step again/);
  assert.doesNotMatch(display(host), /c = keep the fail, go on/);
  assert.doesNotMatch(display(host), /a = start this board over/);
  feed(host, "RESULT FAIL pca");
  assert.match(display(host), /PCA9685 did not answer at I2C 0x40/);
  assert.match(display(host), /this board stops before the speaker test/);
  assert.match(display(host), /3\.3 V, not 5 V/);
  assert.match(display(host), /RESULT FAIL pca/);
  assert.match(display(host), /unplug USB for the next board, or q to stop/);
  assert.doesNotMatch(display(host), /r = try this step again/);
  assert.equal(splitCard(display(host)).title, "PCA");
  feed(host, "RESULT PASS");
  assert.match(display(host), /RESULT PASS/);
  const asks = host.grantAsks;
  onDisconnect(host);
  assert.equal(host.connected, false);
  assert.equal(onPortReturn(host), true);
  assert.equal(host.grantAsks, asks);
  assert.equal(host.connected, true);

  const fresh = createHost();
  assert.equal(onPortReturn(fresh), false);
  assert.equal(fresh.grantAsks, 1);
}

function assertConnectAsksAgain() {
  const port = { readable: {} };
  assert.equal(connectTarget(port, true), port);
  assert.equal(connectTarget(port, false), null);
}

function assertCardSplit() {
  const prep = splitCard(prepText());
  assert.equal(prep.title, "PREPARE");
  assert.equal(prep.keys.length, 0);
  assert.equal(prep.body[0].n, "1");
  assert.equal(prep.body[0].text, "Take the board.");
  const tone = splitCard(
    "1/3  SPEAKER\n\nSpeaker on the JST speaker port.\nExpect a repeating tone.\nVolume 6/10.\n\n+ = louder\n- = quieter\nEnter = good\nn = bad"
  );
  assert.equal(tone.title, "1/3  SPEAKER");
  assert.deepEqual(tone.keys.map((item) => item.key), ["+", "-", "Enter", "n"]);
  assert.equal(tone.body.some((item) => item.text.startsWith("Volume")), true);
}

async function assertActivatePort() {
  const prep = prepText();
  assert.match(prep, /PREPARE/);
  assert.match(prep, /Take the board/);
  assert.match(prep, /JST speaker port/);
  assert.match(prep, /not the ESP32/);
  const signals = [];
  const writes = [];
  let closed = 0;
  const port = {
    readable: {},
    writable: {
      getWriter() {
        return {
          async write(data) {
            writes.push(Array.from(data));
          },
          releaseLock() {},
        };
      },
    },
    async open() {
      this.readable = {};
    },
    async close() {
      closed += 1;
      this.readable = null;
      this.writable = this.writable;
    },
    async setSignals(value) {
      signals.push(value);
    },
  };
  const released = { closed: 0, cancelled: 0 };
  const live = {
    readable: {},
    writable: {},
    async close() {
      released.closed += 1;
      this.readable = null;
      this.writable = null;
    },
  };
  const session = {
    reader: {
      async cancel() {
        released.cancelled += 1;
      },
      releaseLock() {},
    },
  };
  await releasePort(live, session);
  assert.equal(released.cancelled, 1);
  assert.equal(released.closed, 1);
  assert.equal(session.closing, true);
  assert.equal(session.connected, false);
  assert.equal(session.reader, null);
  await activatePort(port);
  assert.equal(closed, 1);
  assert.deepEqual(signals, [{ dataTerminalReady: false, requestToSend: false }]);
  assert.deepEqual(writes, [[0x3f]]);
  assert.match(PORT_LOST, /press Connect again/);
}

assertGate();
await assertNoInboundLinks();
assertSession();
assertCardSplit();
assertConnectAsksAgain();
await assertActivatePort();
console.log("pcba web ok");
