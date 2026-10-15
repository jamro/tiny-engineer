import type { TargetedSubmitEvent } from "preact";
import type { Settings } from "../api/types";
import { useRobot } from "../robot";
import { isCenter, SERVO_NAMES } from "../servos";
import { useStatus } from "../status/status";
import { useResettableState } from "../useResettableState";
import { Button } from "./Button";
import { ConfigSection } from "./ConfigSection";

export function ServoLimits({ settings, index }: { settings: Settings; index: number }) {
  const { applySettings } = useRobot();
  const { perform } = useStatus();
  const [draft, setDraft] = useResettableState(settings, (saved) => ({
    min: String(saved.servo_mins[index]),
    max: String(saved.servo_maxs[index]),
  }));
  const min = Number(draft.min);
  const max = Number(draft.max);
  const valid =
    draft.min !== "" &&
    draft.max !== "" &&
    Number.isInteger(min) &&
    Number.isInteger(max) &&
    min >= 0 &&
    max <= 180 &&
    min < max;
  const changed = min !== settings.servo_mins[index] || max !== settings.servo_maxs[index];
  const keepsCenter = isCenter(settings.servo_centers[index], [min, max]);
  const center = keepsCenter ? settings.servo_centers[index] : (min + max) / 2;

  async function save(event: TargetedSubmitEvent<HTMLFormElement>) {
    event.preventDefault();

    if (!valid || !changed) {
      return;
    }

    await perform<Settings>({
      path: "/settings",
      request: {
        method: "POST",
        json: {
          servo_mins: settings.servo_mins.map((each, i) => (i === index ? min : each)),
          servo_maxs: settings.servo_maxs.map((each, i) => (i === index ? max : each)),
          servo_centers: settings.servo_centers.map((each, i) => (i === index ? center : each)),
        },
      },
      pending: "Saving servo limits…",
      failure: "Limits save failed",
      done: `${SERVO_NAMES[index]} limits saved: ${min}–${max}°.`,
      onSuccess: applySettings,
    });
  }

  return (
    <ConfigSection title="Servo limits" apply="now" className="servo-limits">
      <p className="setup-copy">
        {`Set the safe movement limits for ${SERVO_NAMES[index]}. Save before moving beyond the current range. Use limits you have checked on the assembled robot.`}
      </p>
      <form onSubmit={save}>
        <div className="servo-limits-fields">
          <div className="form-group">
            <label htmlFor="servo-min">Min (°)</label>
            <input
              id="servo-min"
              type="number"
              min="0"
              max="180"
              step="1"
              required
              aria-describedby="servo-limits-hint"
              value={draft.min}
              onInput={(event) => setDraft({ ...draft, min: event.currentTarget.value })}
            />
          </div>
          <div className="form-group">
            <label htmlFor="servo-max">Max (°)</label>
            <input
              id="servo-max"
              type="number"
              min="0"
              max="180"
              step="1"
              required
              aria-describedby="servo-limits-hint"
              value={draft.max}
              onInput={(event) => setDraft({ ...draft, max: event.currentTarget.value })}
            />
          </div>
        </div>
        <p id="servo-limits-hint" className={valid ? "hint" : "hint warn"}>
          {!valid
            ? "Use whole degrees from 0 to 180, with min less than max."
            : keepsCenter
              ? `Center stays at ${center}°. Limits survive power-off.`
              : `Center will reset to ${center}° to stay between the new limits.`}
        </p>
        <Button type="submit" variant="primary" disabled={!valid || !changed}>
          Save limits
        </Button>
      </form>
    </ConfigSection>
  );
}
