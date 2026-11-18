import type { TargetedSubmitEvent } from "preact";
import { Link } from "wouter-preact";
import type { Settings } from "../api/types";
import { Button } from "../components/Button";
import { ConfigSection } from "../components/ConfigSection";
import { useRobot } from "../robot";
import { useStatus } from "../status/status";
import { useResettableState } from "../useResettableState";

const PUSH_BUTTON_PINS = [6, 7, 20, 21] as const;

const BUTTON_EVENT_ROWS = [
  { key: "down", api: "button.down", when: "Press" },
  { key: "up", api: "button.up", when: "Release" },
  { key: "click", api: "button.click", when: "Short press (after 350 ms)" },
  { key: "double", api: "button.double_click", when: "Two short presses" },
  { key: "long", api: "button.long_click", when: "Hold 800 ms" },
] as const;

const LCD_BOOT_MODES = [
  { value: "none", label: "Nothing" },
  { value: "last", label: "Last content" },
  { value: "config", label: "Configured lines" },
] as const;

function draftFrom(settings: Settings) {
  return {
    pushButton: settings.push_button,
    pushButtonPin: settings.push_button_pin,
    lcd1602: settings.lcd1602,
    lcd1602BacklightSleep: settings.lcd1602_backlight_sleep,
    lcd1602WelcomeHello: settings.lcd1602_welcome_hello,
    lcd1602Boot: settings.lcd1602_boot,
    lcd1602Line1: settings.lcd1602_line1,
    lcd1602Line2: settings.lcd1602_line2,
  };
}

type ExtensionsDraft = ReturnType<typeof draftFrom>;

// POST /settings is partial — omit unchanged fields so LCD apply does not wipe live text
// when the user only toggles the push button (or any non-LCD field).
function extensionsPatch(draft: ExtensionsDraft, settings: Settings): Record<string, unknown> {
  const patch: Record<string, unknown> = {};

  if (draft.pushButton !== settings.push_button) {
    patch.push_button = draft.pushButton;
  }

  if (draft.pushButtonPin !== settings.push_button_pin) {
    patch.push_button_pin = draft.pushButtonPin;
  }

  if (draft.lcd1602 !== settings.lcd1602) {
    patch.lcd1602 = draft.lcd1602;
  }

  if (draft.lcd1602BacklightSleep !== settings.lcd1602_backlight_sleep) {
    patch.lcd1602_backlight_sleep = draft.lcd1602BacklightSleep;
  }

  if (draft.lcd1602WelcomeHello !== settings.lcd1602_welcome_hello) {
    patch.lcd1602_welcome_hello = draft.lcd1602WelcomeHello;
  }

  if (draft.lcd1602Boot !== settings.lcd1602_boot) {
    patch.lcd1602_boot = draft.lcd1602Boot;
  }

  if (draft.lcd1602Line1 !== settings.lcd1602_line1) {
    patch.lcd1602_line1 = draft.lcd1602Line1;
  }

  if (draft.lcd1602Line2 !== settings.lcd1602_line2) {
    patch.lcd1602_line2 = draft.lcd1602Line2;
  }

  return patch;
}

export function Extensions() {
  const { settings, applySettings } = useRobot();

  if (!settings) {
    return null;
  }

  return <ExtensionsForm settings={settings} applySettings={applySettings} />;
}

function ExtensionsForm({
  settings,
  applySettings,
}: {
  settings: Settings;
  applySettings: (settings: Settings) => void;
}) {
  const { perform, setStatus } = useStatus();
  const [draft, setDraft] = useResettableState(settings, draftFrom);
  const set = (fields: Partial<typeof draft>) =>
    setDraft((previous) => ({ ...previous, ...fields }));

  async function save(event: TargetedSubmitEvent<HTMLFormElement>) {
    event.preventDefault();

    const json = extensionsPatch(draft, settings);

    if (Object.keys(json).length === 0) {
      setStatus("Extensions saved.", "ok");

      return;
    }

    await perform<Settings>({
      path: "/settings",
      request: {
        method: "POST",
        json,
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
        Optional hardware on the Extension port. Off by default. Button events go out over WebSocket
        and serial when enabled. Local reactions live on the Actions page.
      </p>
      <form onSubmit={save}>
        <ConfigSection title="Push button" apply="now">
          <div className="form-group">
            <div className="toggle-row toggle-row-enable">
              <input
                type="checkbox"
                id="ext-push-button"
                checked={draft.pushButton}
                onChange={(event) => set({ pushButton: event.currentTarget.checked })}
              />
              <label htmlFor="ext-push-button">Enable extension</label>
            </div>
            {draft.pushButton ? (
              <div className="ext-details">
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
                  <code>push-button</code>). Bind sounds or anims on{" "}
                  <Link href="/actions">Actions</Link>.
                </p>
                <div className="ext-events" role="group" aria-label="Button events">
                  <div className="ext-event-row ext-event-head" aria-hidden="true">
                    <span>Emitted event</span>
                    <span>When</span>
                  </div>
                  {BUTTON_EVENT_ROWS.map((row) => (
                    <div key={row.key} className="ext-event-row">
                      <div className="ext-event-meta">
                        <code>{row.api}</code>
                      </div>
                      <span className="ext-event-when">{row.when}</span>
                    </div>
                  ))}
                </div>
                <p className="hint">
                  Out: <code>ws://&lt;host&gt;:81/events</code> and serial <code>Evt</code>. No
                  extra HTTP endpoints for the button itself.
                </p>
              </div>
            ) : null}
          </div>
        </ConfigSection>

        <ConfigSection title="LCD 1602" apply="now">
          <div className="form-group">
            <div className="toggle-row toggle-row-enable">
              <input
                type="checkbox"
                id="ext-lcd1602"
                checked={draft.lcd1602}
                onChange={(event) => set({ lcd1602: event.currentTarget.checked })}
              />
              <label htmlFor="ext-lcd1602">Enable extension</label>
            </div>
            {draft.lcd1602 ? (
              <div className="ext-details">
                <div className="toggle-row">
                  <input
                    type="checkbox"
                    id="ext-lcd1602-bl"
                    checked={draft.lcd1602BacklightSleep}
                    onChange={(event) =>
                      set({ lcd1602BacklightSleep: event.currentTarget.checked })
                    }
                  />
                  <label htmlFor="ext-lcd1602-bl">Follow sleep</label>
                </div>
                <div className="toggle-row">
                  <input
                    type="checkbox"
                    id="ext-lcd1602-hello"
                    checked={draft.lcd1602WelcomeHello}
                    onChange={(event) => set({ lcd1602WelcomeHello: event.currentTarget.checked })}
                  />
                  <label htmlFor="ext-lcd1602-hello">Hello on welcome</label>
                </div>
                <div className="field-head">
                  <span>Boot content</span>
                </div>
                <div className="choice-row" role="radiogroup" aria-label="LCD boot content">
                  {LCD_BOOT_MODES.map((mode) => {
                    const id = `ext-lcd1602-boot-${mode.value}`;

                    return (
                      <label key={mode.value} className="choice-option" htmlFor={id}>
                        <input
                          type="radio"
                          id={id}
                          name="ext-lcd1602-boot"
                          value={mode.value}
                          checked={draft.lcd1602Boot === mode.value}
                          onChange={() => set({ lcd1602Boot: mode.value })}
                        />
                        <span>{mode.label}</span>
                      </label>
                    );
                  })}
                </div>
                {draft.lcd1602Boot === "config" ? (
                  <>
                    <label htmlFor="ext-lcd1602-line1">Line 1</label>
                    <input
                      id="ext-lcd1602-line1"
                      type="text"
                      maxLength={64}
                      value={draft.lcd1602Line1}
                      onInput={(event) => set({ lcd1602Line1: event.currentTarget.value })}
                    />
                    <label htmlFor="ext-lcd1602-line2">Line 2</label>
                    <input
                      id="ext-lcd1602-line2"
                      type="text"
                      maxLength={64}
                      value={draft.lcd1602Line2}
                      onInput={(event) => set({ lcd1602Line2: event.currentTarget.value })}
                    />
                  </>
                ) : null}
                <p className="hint">
                  Waveshare LCD1602 I2C on the Extension I2C port (J3): VCC=+3.3V, GND, SDA, SCL.
                  Fixed LCD 0x3E; backlight auto 0x6B (SN3193) or 0x60 (PCA9633 RGB). RGB color via{" "}
                  <code>/extensions/lcd1602/color</code> (PCA9633 only). No outbound events.
                </p>
                <p className="hint">
                  API: <code>/extensions/lcd1602/text</code>,{" "}
                  <code>/extensions/lcd1602/brightness</code>.
                </p>
              </div>
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
