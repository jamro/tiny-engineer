import type { TargetedSubmitEvent } from "preact";
import type { Settings } from "../api/types";
import { Button } from "../components/Button";
import { ConfigSection } from "../components/ConfigSection";
import { useRobot } from "../robot";
import { useStatus } from "../status/status";
import { useResettableState } from "../useResettableState";

const PUSH_BUTTON_PINS = [6, 7, 20, 21] as const;

type ButtonSound = "none" | "bell" | "click";

const BUTTON_SOUNDS: { value: ButtonSound; label: string }[] = [
  { value: "none", label: "None" },
  { value: "bell", label: "Bell" },
  { value: "click", label: "Click" },
];

const BUTTON_EVENT_ROWS = [
  {
    key: "down",
    api: "button.down",
    when: "Press",
    draft: "soundDown",
  },
  {
    key: "up",
    api: "button.up",
    when: "Release",
    draft: "soundUp",
  },
  {
    key: "click",
    api: "button.click",
    when: "Short press (after 350 ms)",
    draft: "soundClick",
  },
  {
    key: "double",
    api: "button.double_click",
    when: "Two short presses",
    draft: "soundDoubleClick",
  },
  {
    key: "long",
    api: "button.long_click",
    when: "Hold 800 ms",
    draft: "soundLongClick",
  },
] as const;

function draftFrom(settings: Settings) {
  return {
    pushButton: settings.push_button,
    pushButtonPin: settings.push_button_pin,
    soundDown: settings.push_button_sound_down,
    soundUp: settings.push_button_sound_up,
    soundClick: settings.push_button_sound_click,
    soundDoubleClick: settings.push_button_sound_double_click,
    soundLongClick: settings.push_button_sound_long_click,
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
          push_button_sound_down: draft.soundDown,
          push_button_sound_up: draft.soundUp,
          push_button_sound_click: draft.soundClick,
          push_button_sound_double_click: draft.soundDoubleClick,
          push_button_sound_long_click: draft.soundLongClick,
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
                  <span>Events</span>
                </div>
                <p className="hint ext-events-lede">
                  Each press publishes the event names below (<code>source</code>{" "}
                  <code>push-button</code>). Optional sound plays on the robot when the speaker is
                  free.
                </p>
                <div className="ext-events" role="group" aria-label="Button events and sounds">
                  <div className="ext-event-row ext-event-head" aria-hidden="true">
                    <span>Emitted event</span>
                    <span>Sound</span>
                  </div>
                  {BUTTON_EVENT_ROWS.map((row) => {
                    const id = `ext-push-button-sound-${row.key}`;
                    const value = draft[row.draft];

                    return (
                      <div key={row.key} className="ext-event-row">
                        <div className="ext-event-meta">
                          <label htmlFor={id}>
                            <code>{row.api}</code>
                          </label>
                          <span className="ext-event-when">{row.when}</span>
                        </div>
                        <select
                          id={id}
                          aria-label={`${row.api} sound`}
                          value={value}
                          onChange={(event) =>
                            set({ [row.draft]: event.currentTarget.value as ButtonSound })
                          }
                        >
                          {BUTTON_SOUNDS.map((option) => (
                            <option key={option.value} value={option.value}>
                              {option.label}
                            </option>
                          ))}
                        </select>
                      </div>
                    );
                  })}
                </div>
                <p className="hint">
                  Out: <code>ws://&lt;host&gt;:81/events</code> and serial <code>Evt</code>. No
                  extra HTTP. Busy speaker skips sound; the event still publishes.
                </p>
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
