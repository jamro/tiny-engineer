import { Button } from "../components/Button";
import { ConfigSection } from "../components/ConfigSection";
import { FacePoseControls } from "../components/FacePoseControls";
import type { Range } from "../servos";
import {
  CALIBRATION_STEP_COUNT,
  GUIDED_STEPS,
  startAngleFor,
  type Calibration,
} from "./useCalibration";
import { useSetupServo } from "./useSetupServo";

const FACE_PROGRESS = CALIBRATION_STEP_COUNT;

export function FaceStep({
  pose,
  ranges,
  calibration,
  onPose,
  onSave,
}: {
  pose: readonly number[];
  ranges: readonly Range[];
  calibration: Calibration;
  onPose: (pose: number[]) => void;
  onSave: () => void | Promise<void>;
}) {
  const moveServo = useSetupServo();

  async function back() {
    const lastIndex = GUIDED_STEPS.length - 1;
    const step = GUIDED_STEPS[lastIndex];
    const start =
      calibration.guidedDrafts[lastIndex] ?? startAngleFor(step, calibration.ranges);

    if (!(await moveServo({ index: step.joint, angle: start }))) {
      return;
    }

    calibration.setJointAngle(step.joint, start);
    calibration.setGuidedDraft(lastIndex, start);
    calibration.resumeLastGuided();
  }

  return (
    <ConfigSection title="Face me">
      <p className="hint" data-testid="calib-progress">
        Servo calibration · {FACE_PROGRESS} of {CALIBRATION_STEP_COUNT}
      </p>
      <div
        className="calib-progress-track"
        role="progressbar"
        aria-label="Servo calibration progress"
        aria-valuemin={1}
        aria-valuemax={CALIBRATION_STEP_COUNT}
        aria-valuenow={FACE_PROGRESS}
      >
        <div
          className="calib-progress-bar"
          style={{ width: `${(FACE_PROGRESS / CALIBRATION_STEP_COUNT) * 100}%` }}
        ></div>
      </div>
      <p className="setup-copy">
        Sit where you use the robot. Pick head, neck or body and nudge until it looks at you.
      </p>
      <p className="calib-target">
        Target: <strong>Looks at you</strong>
      </p>
      <div className="form-group">
        <FacePoseControls
          pose={pose}
          ranges={ranges}
          onPose={onPose}
          onMove={(index, angle) => moveServo({ index, angle })}
          moveOnMount
        />
      </div>
      <div className="setup-footer">
        <Button onClick={back}>Back</Button>
        <Button variant="primary" onClick={onSave}>
          Save and continue
        </Button>
      </div>
    </ConfigSection>
  );
}
