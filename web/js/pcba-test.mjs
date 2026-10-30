export const WEB_TESTER = "https://tinyengineer.dev/pcba_test";
export const PYTHON_COMMAND = "python3 scripts/pcba_test.py";
export const INCOMPATIBLE =
  "This browser cannot run the test. Use " + PYTHON_COMMAND;

export function canTest(scope) {
  return Boolean(
    scope &&
      scope.isSecureContext &&
      scope.navigator &&
      scope.navigator.serial
  );
}

export function createHost() {
  return {
    shown: "",
    menu: false,
    volume: 6,
    step: "",
    granted: false,
    connected: false,
    grantAsks: 0,
    heard: false,
    opening: false,
    reopenTries: 0,
    session: 0,
  };
}

export function connectTarget(port, connected) {
  return connected ? port : null;
}

export function prepText() {
  return [
    "PREPARE",
    "",
    "1  Take the board.",
    "2  Plug in the ESP32.",
    "   Match the USB mark on the board.",
    "3  Plug the speaker into the JST speaker port.",
    "4  Connect USB to the board.",
    "   Use the board socket, not the ESP32.",
  ].join("\n");
}

export const PORT_LOST = "Port lost. Unplug the board and press Connect again.";

export async function releasePort(port, host) {
  if (host) {
    host.closing = true;
    host.connected = false;
  }
  const reader = host && host.reader;
  if (reader) {
    try {
      await reader.cancel();
    } catch (_err) {
      // The stream already ended.
    }
    try {
      reader.releaseLock();
    } catch (_err) {
      // Lock already released.
    }
    host.reader = null;
  }
  if (port && (port.readable || port.writable)) {
    try {
      await port.close();
    } catch (_err) {
      // The port dropped before close.
    }
  }
}

export async function activatePort(port) {
  if (port.readable || port.writable) {
    try {
      await port.close();
    } catch (_err) {
      // Already closed after a USB reset.
    }
  }
  await port.open({ baudRate: 115200 });
  if (port.setSignals) {
    await port.setSignals({ dataTerminalReady: false, requestToSend: false });
  }
  const writer = port.writable.getWriter();
  try {
    await writer.write(new Uint8Array([0x3f]));
  } finally {
    writer.releaseLock();
  }
}

export function grant(host) {
  host.granted = true;
  host.connected = true;
}

export function onDisconnect(host) {
  host.connected = false;
}

export function onPortReturn(host) {
  if (!host.granted) {
    host.grantAsks += 1;
    return false;
  }
  host.connected = true;
  return true;
}

const READY_COACH = "The board is connected.";
const BANNER_COACH =
  "The module LED stays steady white while the test image has power. That lamp is not the board verdict.";
const PCA_COACH = "PCA9685 did not answer. This board fails before the speaker test.";
const WAIT_KEYS = "Enter = good\nn = bad";
const TONE_KEYS = "+ = louder\n- = quieter";
const TONE_LEVELS = 10;
const TONE_LEVEL_DEFAULT = 6;
const FAIL_KEYS = "r = try this step again\nc = keep the fail, go on\na = start this board over";
const STEPS = [
  ["tone", "SPEAKER", "Speaker on the JST speaker port. Expect a repeating tone."],
  [
    "servos",
    "SERVOS",
    "Signal is on all five servo outputs. Take the testing board. Plug it into one column at a time. Columns are HEAD, NECK, ARM L, ARM R, and BODY. Check the LEDs on every column. Red stays on and yellow pulses.",
  ],
  [
    "oe",
    "OE OUTPUT ENABLE",
    "The red LED stays on. Only the yellow LED goes out. No need to test each servo column.",
  ],
];

function stepSentence(name) {
  const found = STEPS.find((row) => row[0] === name);
  return found ? found[2] : "";
}

function stepTitle(name) {
  const found = STEPS.find((row) => row[0] === name);
  return found ? found[1] : name;
}

function prose(text) {
  return text
    .split(". ")
    .filter(Boolean)
    .map((part) => (part.endsWith(".") ? part : part + "."))
    .join("\n");
}

function coachStep(name, volume) {
  const index = STEPS.findIndex((row) => row[0] === name);
  const body = `${index + 1}/${STEPS.length}  ${stepTitle(name)}\n\n${prose(stepSentence(name))}`;
  if (name !== "tone") {
    return `${body}\n\n${WAIT_KEYS}`;
  }
  const level = volume == null ? TONE_LEVEL_DEFAULT : volume;
  return `${body}\nVolume ${level}/${TONE_LEVELS}.\n\n${TONE_KEYS}\n${WAIT_KEYS}`;
}

function coachFail(name, token) {
  if (name === "pca") {
    return `PCA\n\n${prose(PCA_COACH)}\n\n${token}`;
  }
  const sentence = stepSentence(name);
  if (!sentence) {
    return token;
  }
  return `${stepTitle(name)}\n\n${prose(sentence)}\n\n${FAIL_KEYS}\n\n${token}`;
}

export function feed(host, line) {
  const text = String(line || "").trim();
  if (!text) {
    return false;
  }
  host.heard = true;
  host.reopenTries = 0;
  if (text.startsWith("STEP ")) {
    const name = text.slice(5);
    host.step = name;
    host.shown = stepSentence(name) ? coachStep(name, host.volume) : text;
    host.menu = false;
  } else if (text.startsWith("VOL ")) {
    const level = Number(text.slice(4));
    if (!Number.isInteger(level) || level < 1 || level > TONE_LEVELS) {
      return false;
    }
    host.volume = level;
    if (host.step === "tone") {
      host.shown = coachStep("tone", level);
    }
    host.menu = false;
  } else if (text.startsWith("FAIL ")) {
    const name = text.slice(5);
    host.shown = coachFail(name, text);
    host.menu = Boolean(stepSentence(name));
  } else if (text.startsWith("RESULT ")) {
    host.shown = text;
    host.menu = false;
  } else if (text === "READY") {
    host.shown = `CONNECTED\n\n${prose(READY_COACH)}\n\nEnter = continue`;
    host.menu = false;
  } else if (text.startsWith("PCBA-TEST")) {
    host.shown = `${BANNER_COACH}\n${text}`;
    host.menu = false;
  } else if (text === "STOP") {
    host.shown = text;
    host.menu = false;
  } else {
    return false;
  }
  return true;
}

export function display(host) {
  return host.shown;
}

export function splitCard(text) {
  const lines = String(text || "").split("\n");
  const keys = [];
  const body = [];
  let title = "";
  for (const raw of lines) {
    const line = raw.trim();
    if (!line) {
      continue;
    }
    const key = line.match(/^(\S+)\s+=\s+(.+)$/);
    if (key && key[1].length <= 12 && !key[1].includes(".")) {
      keys.push({ key: key[1], label: key[2] });
      continue;
    }
    if (!title) {
      if (line.length > 48) {
        title = "BOARD";
        body.push({ kind: "line", text: line });
      } else {
        title = line;
      }
      continue;
    }
    const step = line.match(/^(\d+)\s{2,}(.+)$/);
    if (step) {
      body.push({ kind: "step", n: step[1], text: step[2] });
      continue;
    }
    if (raw.startsWith("   ")) {
      body.push({ kind: "note", text: line });
      continue;
    }
    const kind = line === PORT_LOST || line.startsWith("FAIL ") || line.startsWith("RESULT FAIL")
      ? "alert"
      : "line";
    body.push({ kind, text: line });
  }
  return { title: title || "PREPARE", body, keys };
}

export function keyToken(label) {
  return label === "Enter" ? "enter" : label;
}

export function encodeKey(key) {
  if (key === "enter") {
    return "\n";
  }
  if (key === "+" || key === "=") {
    return "+";
  }
  if (key === "n" || key === "r" || key === "c" || key === "a" || key === "q" || key === "-") {
    return key;
  }
  return "";
}

export function acceptKeyEvent(event) {
  if (event.repeat || event.ctrlKey || event.metaKey || event.altKey) {
    return "";
  }
  const key = event.key === "Enter" ? "enter" : event.key;
  return encodeKey(key);
}

function ledClass(card, text) {
  if (text.includes(PORT_LOST) || text.includes("RESULT FAIL") || text.includes("FAIL ")) {
    return "led led-err";
  }
  if (text.includes("RESULT PASS")) {
    return "led led-ok";
  }
  if (card.title === "CONNECTED" || /^\d\/\d/.test(card.title)) {
    return "led led-ok led-pulse";
  }
  return "led led-warn";
}

function lineRole(text) {
  if (text.startsWith("Volume ")) {
    return "meta";
  }
  if (text.startsWith("No need")) {
    return "aside";
  }
  if (text.startsWith("Columns are ")) {
    return "columns";
  }
  if (
    text.startsWith("Expect ") ||
    text.startsWith("Check the LEDs") ||
    text.startsWith("Red stays") ||
    text.startsWith("The red LED") ||
    text.startsWith("Only the yellow")
  ) {
    return "look";
  }
  return "line";
}

function columnList() {
  const wrap = document.createElement("div");
  wrap.className = "pcba-columns";
  const caption = document.createElement("p");
  caption.className = "pcba-columns-label";
  caption.textContent = "Columns";
  const list = document.createElement("ol");
  for (const name of ["HEAD", "NECK", "ARM L", "ARM R", "BODY"]) {
    const item = document.createElement("li");
    item.textContent = name;
    list.appendChild(item);
  }
  wrap.append(caption, list);
  return wrap;
}

function paintBody(step, items) {
  step.replaceChildren();
  let index = 0;
  while (index < items.length) {
    const item = items[index];
    if (item.kind === "step") {
      const p = document.createElement("p");
      p.className = "pcba-step";
      const n = document.createElement("span");
      n.className = "pcba-step-n";
      n.textContent = item.n;
      const label = document.createElement("span");
      label.textContent = item.text;
      p.append(n, label);
      step.appendChild(p);
      index += 1;
      continue;
    }
    const role = item.kind === "line" ? lineRole(item.text) : item.kind;
    if (role === "look") {
      const box = document.createElement("div");
      box.className = "pcba-look";
      while (index < items.length && items[index].kind === "line" && lineRole(items[index].text) === "look") {
        const p = document.createElement("p");
        p.textContent = items[index].text;
        box.appendChild(p);
        index += 1;
      }
      step.appendChild(box);
      continue;
    }
    if (role === "columns") {
      step.appendChild(columnList());
      index += 1;
      continue;
    }
    const p = document.createElement("p");
    p.className = role === "note" ? "pcba-note" : role === "aside" ? "pcba-aside" : role === "alert" ? "pcba-alert" : role === "meta" ? "pcba-meta" : "pcba-line";
    p.textContent = item.text;
    step.appendChild(p);
    index += 1;
  }
}

function render(host) {
  const step = document.getElementById("step");
  const title = document.getElementById("pcba-title");
  const keys = document.getElementById("pcba-keys");
  const led = document.getElementById("pcba-led");
  const text = display(host) || prepText();
  const card = splitCard(text);
  if (title) {
    title.textContent = card.title;
  }
  if (step) {
    const signature = card.body.map((item) => item.kind + "\n" + item.text).join("\n");
    if (step.dataset.sig !== signature) {
      step.dataset.sig = signature;
      paintBody(step, card.body);
    }
  }
  if (keys) {
    const signature = card.keys.map((item) => item.key + "\n" + item.label).join("\n")
      + (host.connected ? "\n1" : "\n0");
    keys.hidden = card.keys.length === 0;
    if (keys.dataset.sig !== signature) {
      keys.dataset.sig = signature;
      keys.replaceChildren();
      card.keys.forEach((item) => {
        const row = document.createElement("p");
        const button = document.createElement("button");
        button.type = "button";
        button.className = "pcba-key";
        button.dataset.key = keyToken(item.key);
        button.textContent = item.key;
        button.disabled = !host.connected;
        if (button.dataset.key === "enter") {
          row.className = "pcba-key-default";
        }
        row.append(button, document.createTextNode(item.label));
        keys.appendChild(row);
      });
    }
  }
  if (led) {
    led.className = ledClass(card, text);
  }
}

function delay(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function showConnect() {
  if (typeof document === "undefined") {
    return;
  }
  const connect = document.getElementById("connect");
  const disconnect = document.getElementById("disconnect");
  if (connect) {
    connect.hidden = false;
    connect.disabled = false;
  }
  if (disconnect) {
    disconnect.hidden = true;
  }
}

function showLost(host) {
  onDisconnect(host);
  host.opening = false;
  host.shown = `${prepText()}\n\n${PORT_LOST}`;
  render(host);
  showConnect();
}

function armWatch(host) {
  clearTimeout(host.watch);
  const session = host.session;
  host.watch = setTimeout(() => {
    if (!host.heard && host.session === session) {
      showLost(host);
    }
  }, 2000);
}

async function readLoop(port, host) {
  const session = host.session;
  let reader;
  try {
    reader = port.readable.getReader();
    host.reader = reader;
  } catch (_err) {
    if (host.session === session) {
      await recover(port, host);
    }
    return;
  }
  const decoder = new TextDecoder();
  let buffer = "";
  let lost = false;
  try {
    while (true) {
      const { value, done } = await reader.read();
      if (done) {
        lost = true;
        break;
      }
      buffer += decoder.decode(value, { stream: true });
      let nl = buffer.indexOf("\n");
      while (nl >= 0) {
        const line = buffer.slice(0, nl).replace(/\r$/, "");
        buffer = buffer.slice(nl + 1);
        feed(host, line);
        render(host);
        nl = buffer.indexOf("\n");
      }
    }
  } catch (_err) {
    lost = true;
  } finally {
    if (host.reader === reader) {
      host.reader = null;
    }
    try {
      reader.releaseLock();
    } catch (_err) {
      // Reader already released after the port dropped.
    }
  }
  if (lost && !host.closing && host.session === session) {
    await recover(port, host);
  }
}

function showDisconnect() {
  if (typeof document === "undefined") {
    return;
  }
  const connect = document.getElementById("connect");
  const disconnect = document.getElementById("disconnect");
  if (connect) {
    connect.hidden = true;
  }
  if (disconnect) {
    disconnect.hidden = false;
  }
}

async function recover(port, host) {
  const session = host.session;
  if (host.closing) {
    return;
  }
  onDisconnect(host);
  if (host.reopenTries >= 2) {
    showLost(host);
    return;
  }
  host.reopenTries += 1;
  await delay(600);
  if (host.session !== session || host.closing) {
    return;
  }
  try {
    await openGranted(port, host);
  } catch (_err) {
    if (host.session === session) {
      await recover(port, host);
    }
  }
}

async function openGranted(port, host) {
  if (host.opening) {
    return;
  }
  const session = host.session;
  host.opening = true;
  clearTimeout(host.watch);
  host.heard = false;
  try {
    await activatePort(port);
  } catch (err) {
    if (host.session === session) {
      host.connected = false;
      host.opening = false;
    }
    throw err;
  }
  if (host.session !== session) {
    return;
  }
  host.opening = false;
  host.connected = true;
  host.granted = true;
  if (!host.shown || host.shown.includes(PORT_LOST)) {
    host.shown = prepText();
  }
  render(host);
  showDisconnect();
  readLoop(port, host);
  armWatch(host);
}

function boot() {
  if (typeof document === "undefined") {
    return;
  }
  const host = createHost();
  const incompatible = document.getElementById("incompatible");
  const connect = document.getElementById("connect");
  const disconnect = document.getElementById("disconnect");
  let grantedPort = null;

  if (!canTest(window)) {
    return;
  }
  if (incompatible) {
    incompatible.hidden = true;
  }
  if (connect) {
    connect.hidden = false;
  }
  host.shown = prepText();
  render(host);

  connect?.addEventListener("click", async () => {
    host.session += 1;
    const session = host.session;
    host.closing = true;
    host.opening = false;
    host.reopenTries = 0;
    clearTimeout(host.watch);
    try {
      await releasePort(grantedPort, host);
    } catch (_err) {
      // The old port is already gone.
    }
    if (host.session !== session) {
      return;
    }
    host.closing = false;
    let port = connectTarget(grantedPort, host.connected);
    try {
      if (!port) {
        port = await navigator.serial.requestPort();
      }
    } catch (err) {
      if (err && err.name === "NotFoundError") {
        return;
      }
      showLost(host);
      return;
    }
    if (host.session !== session) {
      return;
    }
    grantedPort = port;
    try {
      await openGranted(port, host);
    } catch (_err) {
      if (host.session === session) {
        await recover(port, host);
      }
    }
  });

  disconnect?.addEventListener("click", async () => {
    disconnect.disabled = true;
    clearTimeout(host.watch);
    try {
      await releasePort(grantedPort, host);
    } catch (_err) {
      // Refresh anyway. The next load starts from Connect.
    }
    location.reload();
  });

  navigator.serial.addEventListener("disconnect", (event) => {
    if (event.target === grantedPort) {
      onDisconnect(host);
      render(host);
    }
  });

  navigator.serial.addEventListener("connect", async (event) => {
    if (host.opening || host.connected || host.closing || !host.granted) {
      return;
    }
    grantedPort = event.target;
    host.session += 1;
    const session = host.session;
    host.reopenTries = 0;
    try {
      await openGranted(grantedPort, host);
    } catch (_err) {
      if (host.session === session) {
        await recover(grantedPort, host);
      }
    }
  });

  let portWrite = Promise.resolve();

  function writeEncoded(encoded) {
    const job = portWrite.then(async () => {
      if (!host.connected || !grantedPort?.writable) {
        return;
      }
      const writer = grantedPort.writable.getWriter();
      try {
        await writer.write(new TextEncoder().encode(encoded));
      } finally {
        writer.releaseLock();
      }
    });
    portWrite = job.catch(() => {});
    return job;
  }

  document.getElementById("pcba-keys")?.addEventListener("pointerdown", (event) => {
    if (event.button !== 0) {
      return;
    }
    const button = event.target.closest("button[data-key]");
    if (!button || button.disabled) {
      return;
    }
    const encoded = encodeKey(button.dataset.key);
    if (!encoded) {
      return;
    }
    writeEncoded(encoded);
  });

  document.addEventListener("keydown", (event) => {
    if (!host.connected || !grantedPort?.writable) {
      return;
    }
    const encoded = acceptKeyEvent(event);
    if (!encoded) {
      return;
    }
    event.preventDefault();
    writeEncoded(encoded);
  });
}

boot();
