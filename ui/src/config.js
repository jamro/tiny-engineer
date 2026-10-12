import { token } from "./api.js";
import { $ } from "./dom.js";
import { REBOOT_GATE, showRebootGate } from "./shell.js";
import { clearStatus, perform, setStatus } from "./status.js";
import { wifiNetworks } from "./wifi-networks.js";

const EYES_STYLES = ["classic", "kaomoji", "cover", "dots"];
const DEFAULT_VOLUME = 70;

const hostnameField = $("#config-hostname");
const sleepField = $("#config-sleep");
const continuousField = $("#config-continuous");
const volumeField = $("#config-volume");
const volumeSlider = $("#config-volume-slider");
const welcomeToggle = $("#config-welcome");
const serialLogToggle = $("#config-serial-log");
const servoHoldToggle = $("#config-servo-hold");
const wifiEnabledToggle = $("#config-wifi-enabled");
const loadingSelect = $("#config-loading");
const eyesStyleSelect = $("#config-eyes-style");

// The field shows a mask while a token is saved; typing replaces it, the toggle stages removal.
const accessToken = {
  MASK: "********",
  field: $("#config-access-token"),
  toggle: $("#config-access-token-toggle"),
  status: $("#config-access-token-status"),
  configured: false,
  masked: false,
  clearPending: false,

  loadFromServer(isSet) {
    this.configured = Boolean(isSet);
    this.clearPending = false;
    this.masked = this.configured;
    this.render();
  },

  // Value for the access_token param: "" clears, undefined leaves the saved token alone.
  get pendingValue() {
    if (this.clearPending) {
      return "";
    }

    if (!this.masked && this.field.value) {
      return this.field.value;
    }

    return undefined;
  },

  render() {
    const { field, toggle, status } = this;

    field.disabled = this.clearPending;
    toggle.hidden = !this.configured && !this.clearPending;

    if (this.clearPending) {
      field.value = "";
      field.placeholder = "Token will be removed on Save";
      toggle.textContent = "Undo";
      status.textContent = "Will remove on save";
    } else if (this.configured) {
      if (this.masked) {
        field.value = this.MASK;
      }

      field.placeholder = "Enter a new token to replace";
      toggle.textContent = "Remove token";
      status.textContent = "Auth enabled — click field to replace";
    } else {
      if (!this.masked) {
        field.value = "";
      }

      field.placeholder = "Enter access token";
      status.textContent = "Auth disabled";
    }
  },
};

function setVolume(value) {
  const parsed = parseInt(value, 10);
  const volume = Number.isNaN(parsed) ? DEFAULT_VOLUME : Math.min(100, Math.max(0, parsed));

  volumeField.value = volume;
  volumeSlider.value = volume;
  $("#config-volume-label").textContent = `${volume}%`;
}

function updateWelcomeMotionHint() {
  $("#config-welcome-motion-hint").hidden =
    loadingSelect.value !== "sleep_inertia" || welcomeToggle.checked;
}

const loadingMode = (settings) =>
  settings.loading === "sleep_inertia" ? "sleep_inertia" : "progress";

// Last values applied from the robot (not live form edits).
const applied = {
  hostname: "",
  loading: "progress",
  wifiEnabled: true,
};

export function applyConfigSettings(
  settings,
  fallback = { hostname: "", continuous_timeout: 5, volume: DEFAULT_VOLUME },
) {
  hostnameField.value = settings.hostname || fallback.hostname;
  sleepField.value = settings.sleep_timeout;
  continuousField.value = settings.continuous_timeout ?? fallback.continuous_timeout;
  setVolume(settings.volume ?? fallback.volume);
  welcomeToggle.checked = settings.welcome !== false;
  serialLogToggle.checked = Boolean(settings.serial_log);
  servoHoldToggle.checked = Boolean(settings.servo_hold);
  wifiEnabledToggle.checked = settings.wifi_enabled !== false;
  loadingSelect.value = loadingMode(settings);
  eyesStyleSelect.value = EYES_STYLES.includes(settings.eyes_style)
    ? settings.eyes_style
    : "classic";
  accessToken.loadFromServer(settings.access_token_set);
  applied.hostname = hostnameField.value;
  applied.loading = loadingSelect.value;
  applied.wifiEnabled = wifiEnabledToggle.checked;
  updateWelcomeMotionHint();
}

function saveMessage(before, saved) {
  const hostChanged = (saved.hostname || before.hostname) !== before.hostname;
  const loadingChanged = loadingMode(saved) !== before.loading;
  const wifiEnabledChanged = (saved.wifi_enabled !== false) !== before.wifiEnabled;

  const parts = [];
  if (hostChanged) {
    parts.push("hostname");
  }
  if (loadingChanged) {
    parts.push("loading screen");
  }
  if (wifiEnabledChanged) {
    parts.push("Wi-Fi mode");
  }

  if (parts.length === 0) {
    return "Settings saved.";
  }

  if (parts.length === 1) {
    const label =
      parts[0] === "hostname"
        ? "Hostname"
        : parts[0] === "loading screen"
          ? "Loading screen"
          : "Wi-Fi mode";
    return `Saved. ${label} applies after reboot.`;
  }

  if (parts.length === 2) {
    const first = parts[0] === "hostname" ? "Hostname" : parts[0];
    return `Saved. ${first} and ${parts[1]} apply after reboot.`;
  }

  return "Saved. Hostname, loading screen, and Wi-Fi mode apply after reboot.";
}

async function saveConfig() {
  const host = hostnameField.value.trim();
  const before = { ...applied };
  const newToken = accessToken.pendingValue;
  const params = {
    sleep_timeout: sleepField.value,
    hostname: host,
    volume: volumeField.value,
    welcome: welcomeToggle.checked ? 1 : 0,
    serial_log: serialLogToggle.checked ? 1 : 0,
    servo_hold: servoHoldToggle.checked ? 1 : 0,
    wifi_enabled: wifiEnabledToggle.checked ? 1 : 0,
    continuous_timeout: continuousField.value,
    loading: loadingSelect.value,
    eyes_style: eyesStyleSelect.value,
  };

  if (newToken !== undefined) {
    params.access_token = newToken;
  }

  await perform({
    path: "/settings",
    params,
    pending: "Saving…",
    failure: "Save failed",
    onSuccess: async (saved) => {
      const wifiEnabledChanged =
        (saved.wifi_enabled !== false) !== before.wifiEnabled;
      const statusText = saveMessage(before, saved);

      applyConfigSettings(saved, {
        hostname: host,
        continuous_timeout: params.continuous_timeout,
        volume: params.volume,
      });

      if (newToken !== undefined) {
        token.set(newToken);
      }

      const wifiChanged = wifiNetworks.pending;
      const wifiError = await wifiNetworks.save();

      if (wifiError) {
        setStatus(`Settings saved. WiFi networks not saved: ${wifiError}`, "err");

        return;
      }

      if (wifiEnabledChanged && saved.wifi_enabled === false) {
        clearStatus();
        showRebootGate(true, REBOOT_GATE.serialOnly);

        return;
      }

      const wifiNote = wifiChanged ? " WiFi networks apply after reboot." : "";

      setStatus(statusText + wifiNote, "ok");
    },
  });
}

async function factoryReset() {
  const confirmed = confirm(
    "Reset settings to factory defaults? WiFi credentials will be cleared. Servo ranges, RGB LED mapping, and screen rotation stay. Power-cycle the device to reopen setup AP mode and configure WiFi again.",
  );

  if (!confirmed) {
    return;
  }

  const result = await perform({
    path: "/settings/reset",
    pending: "Resetting…",
    failure: "Factory reset failed",
  });

  if (!result?.ok) {
    return;
  }

  token.clear();
  accessToken.loadFromServer(false);
  clearStatus();
  showRebootGate(true, REBOOT_GATE.factoryReset);
}

volumeSlider.addEventListener("input", () => setVolume(volumeSlider.value));
volumeField.addEventListener("input", () => setVolume(volumeField.value));
loadingSelect.addEventListener("change", updateWelcomeMotionHint);
welcomeToggle.addEventListener("change", updateWelcomeMotionHint);

accessToken.field.addEventListener("focus", () => {
  if (!accessToken.masked) {
    return;
  }

  accessToken.field.value = "";
  accessToken.masked = false;
});
accessToken.field.addEventListener("blur", () => {
  if (accessToken.clearPending || !accessToken.configured) {
    return;
  }

  if (accessToken.field.value.trim() !== "") {
    return;
  }

  accessToken.masked = true;
  accessToken.render();
});
accessToken.toggle.addEventListener("click", () => {
  accessToken.clearPending = !accessToken.clearPending;
  accessToken.masked = !accessToken.clearPending;
  accessToken.render();
});

$("#config-form").addEventListener("submit", (event) => {
  event.preventDefault();
  saveConfig();
});
$("#config-factory-reset").addEventListener("click", factoryReset);
