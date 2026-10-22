import type { TargetedSubmitEvent } from "preact";
import type { Settings } from "../api/types";
import { ApplyBadge } from "../components/ApplyBadge";
import { Button } from "../components/Button";
import { ConfigSection } from "../components/ConfigSection";
import { useRobot } from "../robot";
import { useStatus } from "../status/status";
import { useResettableState } from "../useResettableState";

function draftFrom(settings: Settings) {
  return { pushButton: settings.push_button };
}

export function Extensions() {
  const { settings, applySettings } = useRobot();
  const { perform } = useStatus();

  if (!settings) {
    return null;
  }

  return <ExtensionsForm settings={settings} applySettings={applySettings} perform={perform} />;
}

function ExtensionsForm({
  settings,
  applySettings,
  perform,
}: {
  settings: Settings;
  applySettings: (settings: Settings) => void;
  perform: ReturnType<typeof useStatus>["perform"];
}) {
  const [draft, setDraft] = useResettableState(settings, draftFrom);
  const set = (fields: Partial<typeof draft>) =>
    setDraft((previous) => ({ ...previous, ...fields }));

  async function save(event: TargetedSubmitEvent<HTMLFormElement>) {
    event.preventDefault();

    await perform<Settings>({
      path: "/settings",
      request: {
        method: "POST",
        json: { push_button: draft.pushButton },
      },
      pending: "Saving…",
      failure: "Save failed",
      done: "Extensions saved.",
      onSuccess: (saved) => {
        applySettings(saved);
      },
    });
  }

  return (
    <main>
      <h1>Extensions</h1>
      <p className="lede">
        Optional hardware on the extension port. Off by default. Events go out over WebSocket and
        serial when enabled.
      </p>
      <form onSubmit={save}>
        <ConfigSection title="Push button" apply="now">
          <div className="form-group">
            <div className="field-head">
              <span>Push button</span>
              <ApplyBadge when="now" />
            </div>
            <div className="toggle-row">
              <input
                type="checkbox"
                id="ext-push-button"
                checked={draft.pushButton}
                onChange={(event) => set({ pushButton: event.currentTarget.checked })}
              />
              <label htmlFor="ext-push-button">Enable on GPIO20 (J4 pin 4)</label>
            </div>
            <p className="hint">
              Wire the switch between GP20 and GND. Active-low with the ESP32 internal pull-up.
              Publishes <code>button.down</code> / <code>button.up</code> on{" "}
              <code>ws://&lt;host&gt;:81/events</code> and as serial <code>Evt</code> frames.
            </p>
          </div>
          <Button type="submit" variant="primary">
            Save
          </Button>
        </ConfigSection>
      </form>
    </main>
  );
}
