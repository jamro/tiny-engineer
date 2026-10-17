import { useState } from "preact/hooks";
import { Button } from "../components/Button";
import { ConfigSection } from "../components/ConfigSection";
import { SERVO_NAMES } from "../servos";
import { type Calibration, CENTER, IDENTIFY_DELTA } from "./useCalibration";
import { useSetupServo } from "./useSetupServo";

export function PlaceParts({ calibration }: { calibration: Calibration }) {
  const moveServos = useSetupServo();
  const [identified, setIdentified] = useState(() => SERVO_NAMES.map(() => false));
  // Session-only: remount on leave assembly clears the gate (design §4).
  const [centered, setCentered] = useState(false);

  async function centerAll() {
    if (await moveServos({ all: CENTER })) {
      calibration.setAngles(SERVO_NAMES.map(() => CENTER));
      setCentered(true);
    }
  }

  async function identify(index: number) {
    if (!centered) {
      return;
    }

    const sequence = [CENTER + IDENTIFY_DELTA, CENTER - IDENTIFY_DELTA, CENTER];

    for (const angle of sequence) {
      if (!(await moveServos({ index, angle }))) {
        await moveServos({ index, angle: CENTER });
        calibration.setJointAngle(index, CENTER);

        return;
      }

      calibration.setJointAngle(index, angle);
    }

    setIdentified((current) => current.map((done, i) => (i === index ? true : done)));
  }

  return (
    <ConfigSection title="Assembly · center shafts">
      <p className="setup-copy">
        All joints use the full 0–180° electrical scale. <strong>90°</strong> is shaft center, not
        the middle of the safe band.
      </p>
      <p className="setup-copy">
        <strong>Move all to 90°</strong>, identify each plug with a small wiggle, then press parts
        onto the shafts: <strong>head toward the laptop</strong>, <strong>neck straight</strong>,{" "}
        <strong>hands down</strong>, <strong>body centered</strong>. Snug the screws. Return here
        when done — range calibration is a separate step from the hub.
      </p>
      <Button id="setup-move-90" variant="primary" onClick={centerAll}>
        Move all to 90°
      </Button>
      <div className="setup-identify">
        {SERVO_NAMES.map((name, index) => (
          <Button
            key={name}
            className={identified[index] ? "done" : undefined}
            disabled={!centered}
            onClick={() => identify(index)}
          >
            {identified[index] ? `✓ ${name}` : `Identify ${name}`}
          </Button>
        ))}
      </div>
      <Button id="setup-assembly-done" onClick={calibration.goHub}>
        Done · back to hub
      </Button>
    </ConfigSection>
  );
}
