import { refreshAnimation } from "./animations.js";
import { apiFetch, apiGetJson, token } from "./api.js";
import { device } from "./device.js";
import { $, $$ } from "./dom.js";
import { setFirmwareVersion, startHealthPolling, stopHealthPolling } from "./health.js";
import { loadSettings } from "./settings.js";
import { renderWizard, resetWizard } from "./setup/wizard.js";

const VIEWS = {
  "/": "view-home",
  "/animations": "view-animations",
  "/servo": "view-servo",
  "/tests": "view-tests",
  "/config": "view-config",
  "/api": "view-api",
};

const authGate = $("#auth-gate");
const authError = $("#auth-error");
const authTokenField = $("#auth-token");
const rebootGate = $("#reboot-gate");
const rebootGateTitle = $("#reboot-gate-title");
const rebootGateBody = $("#reboot-gate-body");
let unlocked = false;

export const REBOOT_GATE = {
  factoryReset: {
    title: "Factory reset done",
    paragraphs: [
      "Settings and WiFi credentials are cleared. Servo ranges, RGB LED mapping, and screen rotation stay. Power-cycle or press the device reset button.",
      "Then join the robot WiFi shown on the OLED and open the setup page to configure home WiFi. You can retune servo ranges, screen rotation, and LED mapping there.",
    ],
  },
  serialOnly: {
    title: "Wi-Fi disabled",
    paragraphs: [
      "Power-cycle or press the device reset button. After reboot the Wi-Fi radio stays off — no setup AP and no HTTP panel.",
      "Control the robot over USB serial (CLI --serial or the serial JSON API).",
    ],
  },
};

function showPage(path) {
  if (!unlocked) {
    return;
  }

  const viewId = VIEWS[path] ?? "view-home";

  for (const view of $$(".view")) {
    view.classList.toggle("active", view.id === viewId);
  }

  for (const link of $$("nav a")) {
    link.classList.toggle("active", link.dataset.nav === path);
  }

  if (viewId === "view-animations") {
    refreshAnimation();
  }

  if (viewId === "view-home") {
    startHealthPolling();
  } else {
    stopHealthPolling();
  }
}

export function showAuthGate(show) {
  authGate.classList.toggle("show", show);
  rebootGate.classList.remove("show");
  document.body.classList.toggle("locked", show);

  if (!show) {
    return;
  }

  authError.classList.remove("show");
  authTokenField.value = "";
  authTokenField.focus();
}

export function showRebootGate(show, content = REBOOT_GATE.factoryReset) {
  rebootGate.classList.toggle("show", show);
  authGate.classList.remove("show");
  document.body.classList.toggle("locked", show);

  if (!show) {
    return;
  }

  rebootGateTitle.textContent = content.title;
  rebootGateBody.replaceChildren(
    ...content.paragraphs.map((text) => {
      const p = document.createElement("p");
      p.textContent = text;
      return p;
    }),
  );

  unlocked = false;
  stopHealthPolling();
}

export async function syncSetupUi() {
  try {
    const health = await apiGetJson("/health");

    if (!health.ok) {
      return;
    }

    setFirmwareVersion(health);
    device.provisioning = Boolean(health.provisioning);
    device.wifiConfigured = Boolean(health.wifi_configured);
    const { inSetup } = device;

    document.body.classList.toggle("setup-mode", inSetup);
    $("#config-page-title").textContent = inSetup ? "WiFi setup" : "Config";
    $("#config-page-desc").textContent = inSetup
      ? "Calibrate servos, set screen orientation, check the LED and speaker, then join Wi-Fi or choose USB serial only. The robot tests Wi-Fi before saving."
      : "Saved to flash. Most changes apply right away.";

    if (inSetup) {
      renderWizard();
    }
  } catch {
    // Keep the normal UI; the next health poll or reload retries.
  }
}

async function enterApp() {
  unlocked = true;
  showAuthGate(false);
  showRebootGate(false);
  await syncSetupUi();

  if (device.inSetup) {
    resetWizard();
    showPage("/config");
  } else {
    showPage(location.pathname);
  }

  loadSettings();
}

async function tokenAccepted() {
  const response = await apiFetch("/settings");

  return response.status !== 401;
}

async function unlockWithToken(event) {
  event.preventDefault();
  const candidate = authTokenField.value;

  if (!candidate) {
    return;
  }

  token.set(candidate);
  authError.classList.remove("show");

  try {
    if (await tokenAccepted()) {
      enterApp();

      return;
    }
  } catch {
    // Treated as a rejected token below.
  }

  token.clear();
  authError.classList.add("show");
}

export async function boot() {
  document.body.classList.add("locked");
  $("#auth-form").addEventListener("submit", unlockWithToken);

  let auth;

  try {
    auth = await apiGetJson("/auth");
  } catch {
    enterApp();

    return;
  }

  if (!auth.ok || !auth.required) {
    enterApp();

    return;
  }

  if (!token.get()) {
    showAuthGate(true);

    return;
  }

  try {
    if (await tokenAccepted()) {
      enterApp();

      return;
    }

    token.clear();
  } catch {
    // Network trouble: ask for the token again without discarding it.
  }

  showAuthGate(true);
}
