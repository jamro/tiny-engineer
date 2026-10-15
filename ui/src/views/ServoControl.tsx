import { useState } from "preact/hooks";
import { useRobot } from "../robot";
import type { Settings } from "../api/types";
import { Button } from "../components/Button";
import { ConfigSection } from "../components/ConfigSection";
import { ServoLimits } from "../components/ServoLimits";
import { ServoTrack } from "../components/ServoTrack";
import { DEFAULT_SERVO_RANGES, inRange, isCenter, SERVO_NAMES, savedRanges } from "../servos";
import { useStatus } from "../status/status";

export function ServoControl() {
  const { settings, applySettings } = useRobot();
  const { perform } = useStatus();
  // Until the settings load, the stock ranges stand in.
  const ranges = settings ? savedRanges(settings) : DEFAULT_SERVO_RANGES;
  const [index, setIndex] = useState(0);
  const [angle, setAngle] = useState("90");
  const [min, max] = ranges[index];
  const mid = (min + max) / 2;
  const center = settings?.servo_centers[index] ?? mid;
  const value = parseFloat(angle);
  const safe = inRange(value, ranges[index]);

  function selectServo(next: number) {
    const [nextMin, nextMax] = ranges[next];

    setIndex(next);

    if (!inRange(value, ranges[next])) {
      setAngle(String(settings?.servo_centers[next] ?? (nextMin + nextMax) / 2));
    }
  }

  async function move(target: string) {
    await perform({
      path: "/test/servo",
      request: { method: "POST", json: { index, angle: Number(target) } },
      pending: "Moving servo…",
      failure: "Move failed",
      done: `Servo ${index} moved to ${target}°.`,
    });
  }

  async function saveCenter(target: number) {
    if (!settings || !isCenter(target, ranges[index])) {
      return;
    }

    await perform<Settings>({
      path: "/settings",
      request: {
        method: "POST",
        json: {
          servo_centers: settings.servo_centers.map((each, i) => (i === index ? target : each)),
        },
      },
      pending: "Saving servo center…",
      failure: "Center save failed",
      done: `${SERVO_NAMES[index]} center saved at ${target}°.`,
      onSuccess: applySettings,
    });
  }

  return (
    <section>
      <h2 className="page-title">Servo control</h2>
      <p className="page-desc">Move a single servo to a target angle.</p>
      <form
        onSubmit={(event) => {
          event.preventDefault();
          void move(angle);
        }}
      >
        <div className="form-group">
          <label htmlFor="servo-index">Servo</label>
          <select
            id="servo-index"
            value={index}
            onChange={(event) => selectServo(Number(event.currentTarget.value))}
          >
            {SERVO_NAMES.map((name, servo) => (
              <option key={name} value={servo}>
                {servo} — {name}
              </option>
            ))}
          </select>
          <p id="servo-range-hint" className={safe ? "hint" : "hint warn"}>
            {safe
              ? `Safe range: ${min}–${max}°`
              : `Outside safe range — firmware clamps to ${min}–${max}°`}
          </p>
        </div>
        <div className="form-group">
          <label htmlFor="servo-angle">Angle</label>
          <div className="range-row servo-range">
            <ServoTrack
              background={<div className="servo-safe-band"></div>}
              scale={[min, mid, max]}
            >
              <input
                type="range"
                className="servo-slider"
                aria-label="Angle slider"
                min={min}
                max={max}
                step="0.5"
                value={angle}
                onInput={(event) => setAngle(event.currentTarget.value)}
              />
            </ServoTrack>
            <input
              type="number"
              id="servo-angle"
              aria-describedby="servo-range-hint"
              required
              min={min}
              max={max}
              step="any"
              value={angle}
              onInput={(event) => setAngle(event.currentTarget.value)}
            />
          </div>
        </div>
        <div className="servo-actions">
          <Button
            onClick={() => {
              setAngle(String(center));
              void move(String(center));
            }}
          >
            Center
          </Button>
          <Button type="submit" variant="primary">
            Move servo
          </Button>
        </div>
      </form>
      {settings && <ServoLimits key={index} settings={settings} index={index} />}
      <ConfigSection title="Center calibration">
        <p className="setup-copy">
          Move the selected joint until it is aligned, then save that angle as its center.
          Animations use this center within the safe range. The saved center survives power-off.
        </p>
        <p className="hint">{`Saved center for ${SERVO_NAMES[index]}: ${center}°`}</p>
        <p className="hint" hidden={isCenter(value, ranges[index])}>
          {`Choose an angle strictly between ${min}° and ${max}° to save a center.`}
        </p>
        <div className="servo-actions">
          <Button disabled={!settings || center === mid} onClick={() => saveCenter(mid)}>
            Reset center to midpoint
          </Button>
          <Button
            variant="primary"
            disabled={!settings || !isCenter(value, ranges[index]) || value === center}
            onClick={() => saveCenter(value)}
          >
            Save angle as center
          </Button>
        </div>
      </ConfigSection>
    </section>
  );
}
