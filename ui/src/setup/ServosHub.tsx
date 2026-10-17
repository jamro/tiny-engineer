import { Button } from "../components/Button";
import { ConfigSection } from "../components/ConfigSection";
import { isStockServoRanges } from "../servos";
import { GUIDED_STEPS, rangeMid, type Calibration } from "./useCalibration";
import { useSetupServo } from "./useSetupServo";

export function ServosHub({
  calibration,
  onSkip,
}: {
  calibration: Calibration;
  onSkip: () => void;
}) {
  const moveServos = useSetupServo();
  const stockDefaults = isStockServoRanges(calibration.ranges);

  async function beginGuided() {
    const first = GUIDED_STEPS[0];
    const mid = rangeMid(calibration.ranges[first.joint]);

    calibration.startGuided();
    calibration.setJointAngle(first.joint, mid);
    await moveServos({ index: first.joint, angle: mid });
  }

  return (
    <ConfigSection title="Servo calibration">
      <p className="setup-copy">
        <strong>Calibrate</strong> safe limits on the finished robot, use{" "}
        <strong>Assembly support</strong> to seat horns at electrical 90°, or <strong>Skip</strong>{" "}
        if ranges already look good.
      </p>
      <ul className="setup-hub-when">
        <li>
          <strong>Calibrate servos</strong> — robot assembled; walk through Body → Neck → Head →
          Hands
        </li>
        <li>
          <strong>Assembly support</strong> — first build; move to 90° and identify plugs before
          seating parts
        </li>
        <li>
          <strong>Skip</strong> — keep the ranges already loaded (NVS or defaults)
        </li>
      </ul>
      <div className="setup-hub-actions">
        <Button id="setup-calibrate" variant="primary" onClick={beginGuided}>
          Calibrate servos
        </Button>
        <Button id="setup-assembly" variant="secondary" onClick={calibration.startAssembly}>
          Assembly support
        </Button>
        <Button id="setup-skip-calib" onClick={onSkip}>
          Skip
        </Button>
      </div>
      {stockDefaults && (
        <p className="setup-hub-skip-hint">
          Skipping keeps the narrow stock band (<strong>80°–100°</strong> on every joint).
        </p>
      )}
    </ConfigSection>
  );
}
