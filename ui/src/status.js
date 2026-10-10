import { apiPost } from "./api.js";
import { $, $$ } from "./dom.js";

const statusBar = $("#status");
let busy = false;
let lockedControls = [];

export const isBusy = () => busy;

export function setStatus(message, type) {
  statusBar.textContent = message;
  statusBar.className = type ? `show ${type}` : "show";
}

export function clearStatus() {
  statusBar.className = "";
  statusBar.textContent = "";
}

// Re-enables only what it disabled, so controls that were already disabled for their own reasons stay that way.
function setBusy(on) {
  busy = on;

  if (on) {
    lockedControls = [...$$(".btn, [type=submit]")].filter((control) => !control.disabled);
  }

  for (const control of lockedControls) {
    control.disabled = on;
  }
}

// POSTs with the controls locked, reporting progress and failures in the status bar.
// Resolves to the apiPost result, or null when already busy or the request failed to send.
export async function perform({ path, params, pending, failure }) {
  if (busy) {
    return null;
  }

  setBusy(true);
  setStatus(pending, "loading");

  try {
    const result = await apiPost(path, params);

    if (!result.ok) {
      setStatus(result.data.error || failure, "err");
    }

    return result;
  } catch {
    setStatus("Network error", "err");

    return null;
  } finally {
    setBusy(false);
  }
}
