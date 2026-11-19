import { useState } from "preact/hooks";
import { useRobot } from "../robot";
import { Button } from "../components/Button";
import { ServoTrack } from "../components/ServoTrack";
import { DEFAULT_SERVO_RANGES, inRange, SERVO_NAMES, savedRanges } from "../servos";
import { useStatus } from "../status/status";
import { FacePoseSection } from "./FacePoseSection";

export function ServoControl() {
  const { settings } = useRobot();
  const { perform } = useStatus();
  // Until the settings load, the stock ranges stand in.
  const ranges = settings ? savedRanges(settings) : DEFAULT_SERVO_RANGES;
  const [index, setIndex] = useState(0);
  const [angle, setAngle] = useState("90");
  const [min, max] = ranges[index];
  const mid = (min + max) / 2;
  const value = parseFloat(angle);
  const safe = inRange(value, ranges[index]);

  function selectServo(next: number) {
    const [nextMin, nextMax] = ranges[next];

    setIndex(next);

    if (!inRange(value, ranges[next])) {
      setAngle(String((nextMin + nextMax) / 2));
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
              scale={[min, Math.round(mid), max]}
            >
              <input
                type="range"
                className="servo-slider"
                aria-label="Angle slider"
                min={min}
                max={max}
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
              value={angle}
              onInput={(event) => setAngle(event.currentTarget.value)}
            />
          </div>
        </div>
        <div className="servo-actions">
          <Button
            onClick={() => {
              setAngle(String(mid));
              void move(String(mid));
            }}
          >
            Center
          </Button>
          <Button type="submit" variant="primary">
            Move servo
          </Button>
        </div>
      </form>
      {settings && <FacePoseSection settings={settings} ranges={ranges} />}
    </section>
  );
}
