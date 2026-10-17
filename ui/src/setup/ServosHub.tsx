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

    if (!(await moveServos({ index: first.joint, angle: mid }))) {
      return;
    }

    calibration.startGuided();
    calibration.setJointAngle(first.joint, mid);
    calibration.setGuidedDraft(0, mid);
  }

  return (
    <ConfigSection title="Servo calibration">
      <p className="setup-copy">Choose the path that matches your robot’s current state.</p>
      <div className="setup-hub-actions">
        <Button
          id="setup-calibrate"
          aria-label="Calibrate servos"
          variant="primary"
          onClick={beginGuided}
        >
          <span className="btn-title">Calibrate servos</span>
          <span className="btn-hint">Set safe movement limits on the assembled robot</span>
        </Button>
        <Button
          id="setup-assembly"
          aria-label="Assembly support"
          variant="secondary"
          onClick={calibration.startAssembly}
        >
          <span className="btn-title">Assembly support</span>
          <span className="btn-hint">Center shafts and identify plugs before seating parts</span>
        </Button>
        <Button id="setup-skip-calib" aria-label="Skip" onClick={onSkip}>
          <span className="btn-title">Skip</span>
          <span className="btn-hint">Keep the servo limits already loaded</span>
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
