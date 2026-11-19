import { useState } from "preact/hooks";
import { clamp } from "../lib/math";
import { degToNorm, FACE_JOINTS, normToDeg, type Range } from "../servos";
import { Button } from "./Button";
import { ServoTrack } from "./ServoTrack";

const NUDGES = [-5, -1, 1, 5];

const wholeDegrees = (n: number, range: Range) => Math.round(normToDeg(n, range));

// Pick a joint, then nudge it in whole degrees like the guided calibration. The
// bar spans the joint's saved min..max. The pose stays in -1..1 units; every nudge
// moves the robot at once, inside the saved range.
export function FacePoseControls({
  pose,
  ranges,
  onPose,
  onMove,
}: {
  pose: readonly number[];
  ranges: readonly Range[];
  onPose: (pose: number[]) => void;
  onMove: (index: number, angle: number) => Promise<boolean>;
}) {
  const [slot, setSlot] = useState(0);
  const { index, name, low, high } = FACE_JOINTS[slot];
  const range = ranges[index];
  const [min, max] = range;
  const angle = wholeDegrees(pose[slot], range);
  const percent = (degrees: number) => `${((degrees - min) / Math.max(1, max - min)) * 100}%`;

  async function nudge(delta: number) {
    const target = clamp(angle + delta, min, max);

    if (target === angle) {
      return;
    }

    const next = [...pose];

    next[slot] = degToNorm(target, range);
    onPose(next);

    if (!(await onMove(index, target))) {
      onPose([...pose]);
    }
  }

  return (
    <div className="face-pose">
      <div className="face-pose-joints" role="group" aria-label="Joint">
        {FACE_JOINTS.map((joint, each) => (
          <Button
            key={joint.name}
            variant={each === slot ? "secondary" : undefined}
            aria-pressed={each === slot}
            onClick={() => setSlot(each)}
          >
            {`${joint.name} ${wholeDegrees(pose[each], ranges[joint.index])}°`}
          </Button>
        ))}
      </div>
      <div className="calib-angle-readout">
        <span data-testid="face-angle">{angle}</span>°
      </div>
      <div className="calib-guide" aria-hidden="true">
        <ServoTrack
          scale={[min, Math.round((min + max) / 2), max]}
          background={
            <>
              <div className="servo-safe-band"></div>
              <div className="calib-marker" style={{ left: percent(angle) }}></div>
            </>
          }
        />
      </div>
      <p className="calib-range-label">
        {name}: <strong>−</strong> turns {low}, <strong>+</strong> turns {high}. Saved range:{" "}
        <strong>
          {min}°–{max}°
        </strong>
      </p>
      <div className="calib-nudge face-nudge">
        {NUDGES.map((delta) => (
          <Button key={delta} onClick={() => nudge(delta)}>
            {delta < 0 ? `−${-delta}°` : `+${delta}°`}
          </Button>
        ))}
      </div>
    </div>
  );
}
