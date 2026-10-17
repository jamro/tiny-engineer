import { Button } from "../components/Button";
import { clamp } from "../lib/math";
import { ConfigSection } from "../components/ConfigSection";
import { ServoTrack } from "../components/ServoTrack";
import { useStatus } from "../status/status";
import type { Range } from "../servos";
import { type Calibration, GUIDED_STEPS, mirrorMax, rangeMid } from "./useCalibration";
import { useSetupServo } from "./useSetupServo";

const NUDGES = [-10, -5, -1, 1, 5, 10];

const percent = (angle: number) => `${(angle / 180) * 100}%`;

function startAngleFor(step: (typeof GUIDED_STEPS)[number], ranges: Range[]): number {
  const [min, max] = ranges[step.joint];

  if (step.kind === "hand" && step.bound === "max") {
    return max;
  }

  if (step.kind === "min" || (step.kind === "hand" && step.bound === "min")) {
    return min;
  }

  return rangeMid(ranges[step.joint]);
}

function confirmLabelFor(step: (typeof GUIDED_STEPS)[number], isLast: boolean): string {
  if (isLast) {
    return "Save and continue";
  }

  if (step.kind === "center") {
    return "Confirm center";
  }

  if (step.bound === "max") {
    return "Confirm max";
  }

  return "Confirm min";
}

export function GuidedCalibration({
  calibration,
  onComplete,
}: {
  calibration: Calibration;
  /** Optional ranges override so the last confirm can save before React re-renders. */
  onComplete: (ranges?: Range[]) => void | Promise<void>;
}) {
  const { setStatus } = useStatus();
  const moveServos = useSetupServo();
  const { guidedStep, guidedIndex, guidedDrafts, angles, ranges, centers } = calibration;
  const joint = guidedStep.joint;
  const angle = angles[joint];
  const [min, max] = ranges[joint];
  const sessionCenter = centers[joint];

  const withAngle = (value: number) =>
    angles.map((each, index) => (index === joint ? value : each));

  async function nudge(delta: number) {
    const next = clamp(angle + delta, 0, 180);

    if (next === angle) {
      return;
    }

    calibration.setAngles(withAngle(next));
    calibration.setGuidedDraft(guidedIndex, next);

    if (!(await moveServos({ index: joint, angle: next }))) {
      calibration.setAngles(withAngle(angle));
      calibration.setGuidedDraft(guidedIndex, angle);
    }
  }

  async function demoMirroredRange(jointIndex: number, demoMax: number, center: number) {
    if (!(await moveServos({ index: jointIndex, angle: demoMax }))) {
      return false;
    }

    calibration.setJointAngle(jointIndex, demoMax);

    if (!(await moveServos({ index: jointIndex, angle: center }))) {
      return false;
    }

    calibration.setJointAngle(jointIndex, center);

    return true;
  }

  async function goToStep(index: number) {
    const step = GUIDED_STEPS[index];
    const start = guidedDrafts[index] ?? startAngleFor(step, ranges);

    if (!(await moveServos({ index: step.joint, angle: start }))) {
      return false;
    }

    calibration.setJointAngle(step.joint, start);
    calibration.setGuidedDraft(index, start);

    return true;
  }

  async function advance(rangesForSave?: Range[]) {
    if (calibration.isLastGuidedStep) {
      await onComplete(rangesForSave);

      return;
    }

    const nextIndex = guidedIndex + 1;

    if (!(await goToStep(nextIndex))) {
      return;
    }

    calibration.guidedNext();
  }

  async function confirm() {
    if (guidedStep.kind === "center") {
      calibration.setJointCenter(joint, angle);
      await advance();

      return;
    }

    if (guidedStep.kind === "min") {
      const center = sessionCenter ?? rangeMid(ranges[joint]);

      if (angle >= center) {
        setStatus("Min must be below the center so the mirrored max stays valid.", "err");

        return;
      }

      const nextMax = mirrorMax(center, angle);

      if (nextMax > 180) {
        setStatus(
          "Mirrored max must not exceed 180°. Move the center lower or min closer to center.",
          "err",
        );

        return;
      }

      calibration.setJointRange(joint, [angle, nextMax]);

      if (!(await demoMirroredRange(joint, nextMax, center))) {
        return;
      }

      await advance();

      return;
    }

    const bound = guidedStep.bound ?? "min";
    const nextRange: [number, number] = bound === "min" ? [angle, max] : [min, angle];

    if (nextRange[0] >= nextRange[1]) {
      setStatus(
        bound === "min" ? "Min must be less than max." : "Max must be greater than min.",
        "err",
      );

      return;
    }

    const nextRanges = ranges.map((each, index) => (index === joint ? nextRange : each));

    calibration.setJointRange(joint, nextRange);
    await advance(nextRanges);
  }

  async function back() {
    if (guidedIndex <= 0) {
      calibration.goHub();

      return;
    }

    const prevIndex = guidedIndex - 1;

    if (!(await goToStep(prevIndex))) {
      return;
    }

    calibration.guidedBack();
  }

  const confirmLabel = confirmLabelFor(guidedStep, calibration.isLastGuidedStep);

  return (
    <ConfigSection title={guidedStep.title}>
      <p className="hint" data-testid="calib-progress">
        Servo calibration · {guidedIndex + 1} of {GUIDED_STEPS.length}
      </p>
      <div
        className="calib-progress-track"
        role="progressbar"
        aria-label="Servo calibration progress"
        aria-valuemin={1}
        aria-valuemax={GUIDED_STEPS.length}
        aria-valuenow={guidedIndex + 1}
      >
        <div
          className="calib-progress-bar"
          style={{ width: `${((guidedIndex + 1) / GUIDED_STEPS.length) * 100}%` }}
        ></div>
      </div>
      <p className="setup-copy">{guidedStep.copy}</p>
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
                  style={{ left: percent(min), width: percent(Math.max(0, max - min)) }}
                ></div>
                <div className="calib-marker" style={{ left: percent(angle) }}></div>
              </>
            }
          />
        </div>
        <p className="calib-range-label">
          Saved range:{" "}
          <strong>
            {min}°–{max}°
          </strong>
        </p>
      </div>
      <p className="calib-target">
        Target: <strong>{guidedStep.target}</strong>
      </p>
      <div className="calib-nudge">
        {NUDGES.map((delta) => (
          <Button key={delta} onClick={() => nudge(delta)}>
            {delta < 0 ? `−${-delta}°` : `+${delta}°`}
          </Button>
        ))}
      </div>
      <div className="setup-footer">
        <Button onClick={back}>Back</Button>
        <Button variant="primary" onClick={confirm}>
          {confirmLabel}
        </Button>
      </div>
    </ConfigSection>
  );
}
