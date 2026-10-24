import type { TargetedSubmitEvent } from "preact";
import type { Settings } from "../api/types";
import { Button } from "../components/Button";
import { ConfigSection } from "../components/ConfigSection";
import { useRobot } from "../robot";
import { useStatus } from "../status/status";
import { useResettableState } from "../useResettableState";

const PUSH_BUTTON_PINS = [6, 7, 20, 21] as const;

const BUTTON_EVENTS = [
  "button.down",
  "button.up",
  "button.click",
  "button.double_click",
  "button.long_click",
] as const;

function draftFrom(settings: Settings) {
  return {
    pushButton: settings.push_button,
    pushButtonPin: settings.push_button_pin,
  };
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
        json: {
          push_button: draft.pushButton,
          push_button_pin: draft.pushButtonPin,
        },
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
        Optional hardware on the Extension port. Off by default. Events go out over WebSocket and
        serial when enabled.
      </p>
      <form onSubmit={save}>
        <ConfigSection title="Push button" apply="now">
          <div className="form-group">
            <div className="toggle-row">
              <input
                type="checkbox"
                id="ext-push-button"
                checked={draft.pushButton}
                onChange={(event) => set({ pushButton: event.currentTarget.checked })}
              />
              <label htmlFor="ext-push-button">Enable push button</label>
            </div>
            {draft.pushButton ? (
              <>
                <div className="field-head">
                  <span>Pin</span>
                </div>
                <div className="choice-row" role="radiogroup" aria-label="Extension port pin">
                  {PUSH_BUTTON_PINS.map((pin) => {
                    const id = `ext-push-button-pin-${pin}`;

                    return (
                      <label key={pin} className="choice-option" htmlFor={id}>
                        <input
                          type="radio"
                          id={id}
                          name="ext-push-button-pin"
                          value={pin}
                          checked={draft.pushButtonPin === pin}
                          onChange={() => set({ pushButtonPin: pin })}
                        />
                        <span>GP{pin}</span>
                      </label>
                    );
                  })}
                </div>
                <p className="hint">
                  Wire a momentary switch between GP{draft.pushButtonPin} and GND on the Extension
                  port. Active-low with the ESP32 internal pull-up.
                </p>
                <div className="field-head">
                  <span>API</span>
                </div>
                <div className="ext-api">
                  <p className="hint">No extra HTTP endpoints.</p>
                  <ul className="ext-api-list">
                    {BUTTON_EVENTS.map((name) => (
                      <li key={name}>
                        <code>{name}</code>
                      </li>
                    ))}
                    <li>
                      Out: <code>ws://&lt;host&gt;:81/events</code> and serial <code>Evt</code> (
                      <code>source</code> <code>push-button</code>)
                    </li>
                  </ul>
                  <p className="hint">
                    <code>button.click</code> waits 350&nbsp;ms after a short release;{" "}
                    <code>button.long_click</code> fires after an 800&nbsp;ms hold.
                  </p>
                </div>
              </>
            ) : null}
          </div>
          <Button type="submit" variant="primary">
            Save
          </Button>
        </ConfigSection>
      </form>
    </main>
  );
}
