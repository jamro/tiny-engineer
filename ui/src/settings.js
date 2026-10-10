import { apiGetJson } from "./api.js";
import { applyConfigSettings } from "./config.js";
import { device } from "./device.js";
import { applyServoRanges } from "./servo-ranges.js";
import { applyWizardSettings } from "./setup/wizard.js";
import { setStatus } from "./status.js";

export async function loadSettings() {
  try {
    const settings = await apiGetJson("/settings");

    if (!settings.ok) {
      return;
    }

    device.wifiConfigured = Boolean(settings.wifi_configured);
    applyConfigSettings(settings);
    applyServoRanges(settings);
    applyWizardSettings(settings);
  } catch {
    setStatus("Could not load settings", "err");
  }
}
