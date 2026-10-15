import { Button } from "../components/Button";
import { clamp } from "../lib/math";
import { ConfigSection } from "../components/ConfigSection";
import { ServoTrack } from "../components/ServoTrack";
import { DEFAULT_SERVO_RANGES, isCenter, type Range, SERVO_NAMES } from "../servos";
import { useStatus } from "../status/status";
import { type Calibration } from "./useCalibration";
import { useSetupServo } from "./useSetupServo";

const JOINT_COPY = [
  "Pitch only. Watch cable slack to the head. Stop before the head hits the neck piece. Down is toward the laptop; up is away.",
  "Yaw left and right. Stop when the cables pull taut. Do not twist until the loom binds.",
  "Lowest is forearm horizontal. Highest is upper arm horizontal. On this servo, higher angle is up.",
  "Same physical stops, inverted scale: higher angle is down. Physical lowest (forearm horizontal) is toward the high end of the bar; physical highest (upper arm horizontal) is toward the low end.",
  "Rotate until the right hand sits over the bell at the extreme. Set the center where the torso faces the laptop.",
];
const NUDGES = [-10, -5, -1, 1, 5, 10];

const percent = (angle: number) => `${(angle / 180) * 100}%`;

export function FindRanges({ calibration }: { calibration: Calibration }) {
  const { setStatus } = useStatus();
  const moveServos = useSetupServo();
  const { joint, angles, ranges, centers } = calibration;
  const angle = angles[joint];
  const [min, max] = ranges[joint];

  const withAngle = (value: number) =>
    angles.map((each, index) => (index === joint ? value : each));

  async function nudge(delta: number) {
    const next = clamp(angle + delta, 0, 180);

    if (next === angle) {
      return;
    }

    calibration.setAngles(withAngle(next));

    if (!(await moveServos({ index: joint, angle: next }))) {
      calibration.setAngles(withAngle(angle));
    }
  }

  function setRange(range: Range, resetCenter = false) {
    calibration.setRanges(ranges.map((each, index) => (index === joint ? range : each)));
    const center =
      !resetCenter && isCenter(centers[joint], range) ? centers[joint] : (range[0] + range[1]) / 2;

    calibration.setCenters(centers.map((each, index) => (index === joint ? center : each)));
  }

  function setCenter() {
    if (!isCenter(angle, ranges[joint])) {
      setStatus("Center must be between min and max.", "err");

      return;
    }

    calibration.setCenters(centers.map((each, index) => (index === joint ? angle : each)));
  }

  function setLimit(bound: "min" | "max") {
    if (bound === "min" && angle >= max) {
      setStatus("Min must be less than max.", "err");

      return;
    }

    if (bound === "max" && angle <= min) {
      setStatus("Max must be greater than min.", "err");

      return;
    }

    setRange(bound === "min" ? [angle, max] : [min, angle]);
  }

  return (
    <ConfigSection title="Find safe ranges">
      <p className="setup-copy">
        Move one joint at a time with the buttons. Stop before cables pull taut or parts collide.
        The bar is a guide only. Stock limits are pre-filled. Switching tabs leaves the other joints
        where they are. Once the limits are set, align the joint and set its center between them.
      </p>
      <div className="joint-tabs">
        {SERVO_NAMES.map((name, index) => (
          <Button
            key={name}
            className={index === joint ? "active" : undefined}
            aria-pressed={index === joint}
            onClick={() => calibration.setJoint(index)}
          >
            {name}
          </Button>
        ))}
      </div>
      <p className="setup-copy">{JOINT_COPY[joint]}</p>
      <p className="hint">{`Center: ${centers[joint]}° · safe range: ${min}–${max}°`}</p>
      <div className="form-group">
        <div className="calib-angle-readout">
          <span data-testid="calib-angle">{angle}</span>°
        </div>
        <div className="calib-guide" aria-hidden="true">
          <ServoTrack
            scale={[0, 90, 180]}
            background={
              <>
                <div
                  className="servo-safe-band"
                  style={{ left: percent(min), width: percent(max - min) }}
                ></div>
                <div className="calib-marker" style={{ left: percent(angle) }}></div>
              </>
            }
          />
        </div>
      </div>
      <div className="calib-nudge">
        {NUDGES.map((delta) => (
          <Button key={delta} onClick={() => nudge(delta)}>
            {delta < 0 ? `−${-delta}°` : `+${delta}°`}
          </Button>
        ))}
      </div>
      <div className="calib-minmax">
        <Button onClick={() => setLimit("min")}>{`Set min (${min}°)`}</Button>
        <Button onClick={() => setLimit("max")}>{`Set max (${max}°)`}</Button>
      </div>
      <div className="calib-minmax">
        <Button onClick={setCenter}>{`Set center (${centers[joint]}°)`}</Button>
        <Button onClick={() => setRange(DEFAULT_SERVO_RANGES[joint], true)}>
          Reset to default
        </Button>
      </div>
    </ConfigSection>
  );
}
