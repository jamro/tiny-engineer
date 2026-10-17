import { Button } from "../components/Button";
import { ConfigSection } from "../components/ConfigSection";
import { SERVO_NAMES } from "../servos";
import { type Calibration, CENTER, IDENTIFY_DELTA } from "./useCalibration";
import { useSetupServo } from "./useSetupServo";

export function PlaceParts({ calibration }: { calibration: Calibration }) {
  const moveServos = useSetupServo();

  async function centerAll() {
    if (await moveServos({ all: CENTER })) {
      calibration.setAngles(SERVO_NAMES.map(() => CENTER));
    }
  }

  async function identify(index: number) {
    const sequence = [CENTER + IDENTIFY_DELTA, CENTER - IDENTIFY_DELTA, CENTER];

    for (const angle of sequence) {
      if (!(await moveServos({ index, angle }))) {
        await moveServos({ index, angle: CENTER });
        calibration.setJointAngle(index, CENTER);

        return;
      }

      calibration.setJointAngle(index, angle);
    }
  }

  return (
    <ConfigSection title="Assembly · center shafts">
      <p className="setup-copy">
        All joints use the full 0–180° electrical scale. <strong>90°</strong> is shaft center, not
        the middle of the safe band.
      </p>
      <p className="setup-copy">
        Move all to 90°, identify each plug with a small wiggle, then press parts onto the shafts:
        head toward the laptop, neck straight, hands down, body centered. Snug the screws. Return
        here when done — range calibration is a separate step from the hub.
      </p>
      <Button id="setup-move-90" variant="primary" onClick={centerAll}>
        Move all to 90°
      </Button>
      <div className="setup-identify">
        {SERVO_NAMES.map((name, index) => (
          <Button key={name} onClick={() => identify(index)}>
            {`Identify ${name}`}
          </Button>
        ))}
      </div>
      <Button id="setup-assembly-done" onClick={calibration.goHub}>
        Back to hub
      </Button>
    </ConfigSection>
  );
}
