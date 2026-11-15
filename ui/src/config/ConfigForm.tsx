import type { TargetedSubmitEvent } from "preact";
import { token } from "../api/client";
import type { LoadingMode, Settings } from "../api/types";
import { ApplyBadge } from "../components/ApplyBadge";
import { Button } from "../components/Button";
import { ConfigSection } from "../components/ConfigSection";
import { HostnameField } from "../components/HostnameField";
import { clamp } from "../lib/math";
import { useRobot } from "../robot";
import { useStatus } from "../status/status";
import { useResettableState } from "../useResettableState";
import { AccessTokenField } from "./AccessTokenField";
import { useAccessToken } from "./useAccessToken";
import { useWifiNetworks } from "./useWifiNetworks";
import { WifiNetworksSection } from "./WifiNetworksSection";

const EYES_STYLES = ["classic", "kaomoji", "cover", "dots", "kawaii"];

// An emptied number field reads NaN; the stock volume stands in until the user types again.
const clampVolume = (value: number) => (Number.isNaN(value) ? 70 : clamp(value, 0, 100));

function draftFrom(settings: Settings) {
  return {
    hostname: settings.hostname,
    sleepTimeout: String(settings.sleep_timeout),
    continuousTimeout: String(settings.continuous_timeout),
    volume: settings.volume,
    welcome: settings.welcome,
    serialLog: settings.serial_log,
    loading: settings.loading,
    eyesStyle: settings.eyes_style,
  };
}

// Hostname and loading screen are read at boot, so a change to either waits for a reboot.
function saveMessage(previous: Settings, saved: Settings) {
  const hostChanged = saved.hostname !== previous.hostname;
  const loadingChanged = saved.loading !== previous.loading;

  if (hostChanged && loadingChanged) {
    return "Saved. Hostname and loading screen apply after reboot.";
  }

  if (hostChanged) {
    return "Saved. Hostname applies after reboot.";
  }

  if (loadingChanged) {
    return "Saved. Loading screen applies after reboot.";
  }

  return "Settings saved.";
}

export function ConfigForm({ settings }: { settings: Settings }) {
  const { applySettings } = useRobot();
  const { perform, setStatus } = useStatus();
  const [draft, setDraft] = useResettableState(settings, draftFrom);
  const accessToken = useAccessToken(settings);
  const networks = useWifiNetworks(settings.wifi_networks, applySettings);
  const set = (fields: Partial<typeof draft>) =>
    setDraft((previous) => ({ ...previous, ...fields }));

  async function save(event: TargetedSubmitEvent<HTMLFormElement>) {
    event.preventDefault();

    const newToken = accessToken.pendingValue;

    await perform<Settings>({
      path: "/settings",
      request: {
        method: "POST",
        json: {
          sleep_timeout: Number(draft.sleepTimeout),
          hostname: draft.hostname,
          volume: draft.volume,
          welcome: draft.welcome,
          serial_log: draft.serialLog,
          continuous_timeout: Number(draft.continuousTimeout),
          loading: draft.loading,
          eyes_style: draft.eyesStyle,
          ...(newToken === undefined ? {} : { access_token: newToken }),
        },
      },
      pending: "Saving…",
      failure: "Save failed",
      onSuccess: async (saved) => {
        applySettings(saved);

        if (newToken !== undefined) {
          token.set(newToken);
        }

        const wifiChanged = networks.pending;
        const wifiError = await networks.save();

        if (wifiError) {
          setStatus(`Settings saved. WiFi networks not saved: ${wifiError}`, "err");

          return;
        }

        const wifiNote = wifiChanged ? " WiFi networks apply after reboot." : "";

        setStatus(saveMessage(settings, saved) + wifiNote, "ok");
      },
    });
  }

  return (
    <form onSubmit={save}>
      <ConfigSection title="Network" apply="reboot">
        <HostnameField
          id="config-hostname"
          value={draft.hostname}
          onInput={(hostname) => set({ hostname })}
        />
      </ConfigSection>
      <WifiNetworksSection networks={networks} />
      <ConfigSection title="Timeouts" apply="now">
        <div className="form-group">
          <label htmlFor="config-sleep">Sleep timeout (minutes)</label>
          <input
            type="number"
            id="config-sleep"
            min="1"
            max="1440"
            step="1"
            required
            value={draft.sleepTimeout}
            onInput={(event) => set({ sleepTimeout: event.currentTarget.value })}
          />
          <p className="hint">
            Minutes in animation <code>none</code> before OLED blanks
          </p>
        </div>
        <div className="form-group">
          <label htmlFor="config-continuous">Continuous anim timeout (minutes)</label>
          <input
            type="number"
            id="config-continuous"
            min="1"
            max="1440"
            step="1"
            required
            value={draft.continuousTimeout}
            onInput={(event) => set({ continuousTimeout: event.currentTarget.value })}
          />
          <p className="hint">Max time for typing / reading / thinking / talking</p>
        </div>
      </ConfigSection>
      <ConfigSection title="Audio" apply="now">
        <div className="form-group">
          <label htmlFor="config-volume">
            Volume <span>{`${draft.volume}%`}</span>
          </label>
          <div className="range-row">
            <input
              type="range"
              aria-label="Volume slider"
              min="0"
              max="100"
              value={draft.volume}
              onInput={(event) => set({ volume: clampVolume(event.currentTarget.valueAsNumber) })}
            />
            <input
              type="number"
              id="config-volume"
              min="0"
              max="100"
              step="1"
              required
              value={draft.volume}
              onInput={(event) => set({ volume: clampVolume(event.currentTarget.valueAsNumber) })}
            />
          </div>
        </div>
      </ConfigSection>
      <ConfigSection title="Display" head>
        <div className="form-group">
          <div className="field-head">
            <label htmlFor="config-eyes-style">Eyes style</label>
            <ApplyBadge when="now" />
          </div>
          <select
            id="config-eyes-style"
            value={draft.eyesStyle}
            onChange={(event) => set({ eyesStyle: event.currentTarget.value })}
          >
            {EYES_STYLES.map((style) => (
              <option key={style} value={style}>
                {style[0].toUpperCase() + style.slice(1)}
              </option>
            ))}
          </select>
          <p className="hint">
            Classic is the default procedural eyes. Kaomoji maps each animation to a face from the
            expression library. Cover fills each half of the screen for a mask with eye holes;
            animations change height with black margins. Dots draws a 12x12 circle per eye; size and
            position follow the same poses as classic. Add new styles in firmware under{" "}
            <code>display/eyes/styles/</code>.
          </p>
        </div>
      </ConfigSection>
      <ConfigSection title="Boot behavior">
        <p className="config-section-desc">Loading screen changes take effect on next boot.</p>
        <div className="form-group">
          <div className="field-head">
            <label htmlFor="config-loading">Boot loading screen</label>
            <ApplyBadge when="reboot" />
          </div>
          <select
            id="config-loading"
            value={draft.loading}
            onChange={(event) => set({ loading: event.currentTarget.value as LoadingMode })}
          >
            <option value="progress">Progress bar</option>
            <option value="sleep_inertia">Sleep inertia (no IP display)</option>
          </select>
          <p className="hint" hidden={draft.loading !== "sleep_inertia" || draft.welcome}>
            Head motion during boot needs Welcome enabled.
          </p>
        </div>
        <div className="form-group">
          <div className="field-head">
            <span>Welcome on boot</span>
            <ApplyBadge when="now" />
          </div>
          <div className="toggle-row">
            <input
              type="checkbox"
              id="config-welcome"
              checked={draft.welcome}
              onChange={(event) => set({ welcome: event.currentTarget.checked })}
            />
            <label htmlFor="config-welcome">Play welcome when Wi-Fi connects</label>
          </div>
        </div>
        <div className="form-group">
          <div className="field-head">
            <span>Serial logging</span>
            <ApplyBadge when="now" />
          </div>
          <div className="toggle-row">
            <input
              type="checkbox"
              id="config-serial-log"
              checked={draft.serialLog}
              onChange={(event) => set({ serialLog: event.currentTarget.checked })}
            />
            <label htmlFor="config-serial-log">Log to USB serial</label>
          </div>
        </div>
      </ConfigSection>
      <ConfigSection title="Security">
        <AccessTokenField {...accessToken.field} />
      </ConfigSection>
      <Button type="submit" variant="primary">
        Save settings
      </Button>
    </form>
  );
}
