import { Button } from "../components/Button";
import { ConfigSection } from "../components/ConfigSection";
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
        Assemble parts at electrical 90°, or guide safe ranges on the finished robot. Skip if ranges
        already look good.
      </p>
      <div className="setup-hub-actions">
        <Button id="setup-calibrate" variant="primary" onClick={beginGuided}>
          Calibrate ranges
        </Button>
        <Button id="setup-skip-calib" onClick={onSkip}>
          Skip calibration
        </Button>
        <Button id="setup-assembly" onClick={calibration.startAssembly}>
          Assembly: 90° + identify
        </Button>
      </div>
    </ConfigSection>
  );
}
