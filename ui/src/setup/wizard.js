import { postQuietly } from "../api.js";
import { device } from "../device.js";
import { $ } from "../dom.js";
import { applyServoRanges, rangesToParams } from "../servo-ranges.js";
import { syncSetupUi } from "../shell.js";
import { clearStatus, isBusy, perform, setStatus } from "../status.js";
import { calibration } from "./calibration.js";
import { ledMapping } from "./led.js";

const STEPS = [
  { id: "servos", title: "Servos", next: "Next: Screen" },
  { id: "oled", title: "Screen", next: "Next: RGB mapping" },
  { id: "led", title: "RGB mapping", next: "Next: Speaker" },
  { id: "speaker", title: "Speaker", next: "Next: Network" },
  { id: "network", title: "Network" },
];

const nextButton = $("#setup-next");
let stepIndex = 0;
let oledRotate180 = false;

const currentStep = () => STEPS[stepIndex];

function goTo(id) {
  stepIndex = STEPS.findIndex((step) => step.id === id);
}

function previewOled() {
  postQuietly("/setup/oled", { rotate_180: oledRotate180 ? 1 : 0 });
}

function applyOledRotation(settings) {
  if (typeof settings.oled_rotate_180 === "boolean") {
    oledRotate180 = settings.oled_rotate_180;
  }
}

function saveSettings(params, pending) {
  return perform({
    path: "/settings",
    params,
    pending,
    failure: "Save failed",
  });
}

export function renderWizard() {
  const step = currentStep();
  const placingHorns = step.id === "servos" && calibration.phase === "horns";

  $("#setup-progress-label").textContent =
    `Step ${stepIndex + 1} of ${STEPS.length} · ${step.title}`;
  $("#setup-progress-bar").style.width = `${((stepIndex + 1) / STEPS.length) * 100}%`;

  for (const { id } of STEPS) {
    $(`#setup-step-${id}`).hidden = id !== step.id;
  }

  $("#setup-phase-horns").hidden = !placingHorns;
  $("#setup-phase-ranges").hidden = step.id !== "servos" || placingHorns;
  $("#setup-footer").hidden = placingHorns;
  $("#setup-back").hidden = placingHorns;
  nextButton.hidden = !step.next || placingHorns;
  nextButton.textContent = step.next ?? "Next";
  nextButton.disabled = false;

  if (step.id === "servos") {
    nextButton.disabled = !calibration.valid;

    if (!placingHorns) {
      calibration.render();
    }
  }

  if (step.id === "led") {
    nextButton.disabled = !ledMapping.valid;
    ledMapping.render();
  }

  if (step.id === "network") {
    $("#config-wifi-ssid").focus();
  }
}

export function resetWizard() {
  stepIndex = 0;
  calibration.reset();
  ledMapping.reset();
  renderWizard();
}

export function applyWizardSettings(settings) {
  $("#config-wifi-hostname").value = settings.hostname || "";
  ledMapping.applySavedOrder(settings.rgb_order);
  applyOledRotation(settings);

  if (device.inSetup) {
    renderWizard();
  }
}

const advance = {
  async servos() {
    if (!calibration.valid) {
      return;
    }

    const result = await saveSettings(rangesToParams(calibration.ranges), "Saving servo ranges…");

    if (!result?.ok) {
      return;
    }

    applyServoRanges(result.data);
    goTo("oled");
    previewOled();
    clearStatus();
  },

  async oled() {
    const result = await saveSettings(
      { oled_rotate_180: oledRotate180 ? 1 : 0 },
      "Saving screen rotation…",
    );

    if (!result?.ok) {
      return;
    }

    applyOledRotation(result.data);

    goTo("led");
    clearStatus();
  },

  async led() {
    if (!ledMapping.valid) {
      return;
    }

    const result = await saveSettings({ rgb_order: ledMapping.order }, "Saving LED mapping…");

    if (!result?.ok) {
      return;
    }

    ledMapping.applySavedOrder(result.data.rgb_order);
    ledMapping.release();
    goTo("speaker");
    clearStatus();
  },

  speaker() {
    goTo("network");
  },
};

const retreat = {
  servos() {
    calibration.phase = "horns";
  },
  oled() {
    postQuietly("/setup/oled");
    goTo("servos");
    calibration.phase = "ranges";
  },
  led() {
    ledMapping.release();
    goTo("oled");
    previewOled();
  },
  speaker() {
    goTo("led");
  },
  network() {
    goTo("speaker");
  },
};

nextButton.addEventListener("click", async () => {
  if (isBusy()) {
    return;
  }

  await advance[currentStep().id]?.();
  renderWizard();
});

$("#setup-back").addEventListener("click", () => {
  retreat[currentStep().id]();
  renderWizard();
});

$("#setup-horns-done").addEventListener("click", () => {
  calibration.phase = "ranges";
  calibration.joint = 0;
  calibration.angles[0] = 90;
  renderWizard();
});

$("#setup-oled-rotate").addEventListener("click", () => {
  if (isBusy()) {
    return;
  }

  oledRotate180 = !oledRotate180;
  previewOled();
});

$("#setup-audio-play").addEventListener("click", async () => {
  const result = await perform({
    path: "/setup/audio",
    pending: "Playing…",
    failure: "Playback failed",
  });

  if (result?.ok) {
    setStatus("Done.", "ok");
  }
});

$("#config-wifi-password-toggle").addEventListener("click", (event) => {
  const field = $("#config-wifi-password");
  const reveal = field.type === "password";

  field.type = reveal ? "text" : "password";
  event.currentTarget.textContent = reveal ? "Hide" : "Show";
});

$("#config-wifi-connect").addEventListener("click", async () => {
  if (isBusy()) {
    return;
  }

  const ssid = $("#config-wifi-ssid").value.trim();
  const passwordField = $("#config-wifi-password");
  const hostnameField = $("#config-wifi-hostname");

  hostnameField.value = hostnameField.value.trim();

  if (!ssid) {
    setStatus("Enter a WiFi network name.", "err");

    return;
  }

  if (!hostnameField.reportValidity()) {
    return;
  }

  const result = await perform({
    path: "/settings",
    params: {
      wifi_ssid: ssid,
      wifi_password: passwordField.value,
      hostname: hostnameField.value,
    },
    pending: "Testing WiFi credentials…",
    failure: "WiFi connection failed",
  });

  if (!result?.ok) {
    return;
  }

  const { wifi_connect_success: connected, wifi_ip: ip, wifi_hostname: hostname } = result.data;

  if (!connected) {
    setStatus(result.data.error || "WiFi connection failed", "err");

    return;
  }

  passwordField.value = "";
  device.wifiConfigured = true;
  device.provisioning = false;
  document.body.classList.remove("setup-mode");
  syncSetupUi();
  const links = [ip, hostname].filter(Boolean).map((host) => `http://${host}`);

  setStatus(
    links.length
      ? `WiFi connected. Rejoin your home network and open ${links.join(" or ")}.`
      : "WiFi connected. Rejoin your home network.",
    "ok",
  );
});
