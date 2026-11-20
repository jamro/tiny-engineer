import { useEffect, useRef, useState } from "preact/hooks";
import { clamp } from "../lib/math";
import { degToNorm, FACE_JOINTS, faceDegrees, moveFacePose, type Range } from "../servos";
import { Button } from "./Button";
import { ServoTrack } from "./ServoTrack";

const NUDGES = [-5, -1, 1, 5];

// Pick a joint, then nudge it in whole degrees like the guided calibration. The
// bar spans the joint's saved min..max. The pose stays in -1..1 units; every nudge
// moves the robot at once, inside the saved range. Before the first nudge (or on
// mount with moveOnMount) all three joints go to the shown pose. A new
// `resync` value (another control moved a joint) makes the next nudge sync again.
// Save moves all joints itself (moveFacePose), so it never depends on this sync.
export function FacePoseControls({
  pose,
  ranges,
  onPose,
  onMove,
  moveOnMount = false,
  resync = 0,
}: {
  pose: readonly number[];
  ranges: readonly Range[];
  onPose: (pose: number[]) => void;
  onMove: (index: number, angle: number) => Promise<boolean>;
  moveOnMount?: boolean;
  resync?: number;
}) {
  const [slot, setSlot] = useState(0);
  const syncRef = useRef<Promise<boolean> | null>(null);
  const syncedResyncRef = useRef(resync);
  const { index, name, low, high } = FACE_JOINTS[slot];
  const range = ranges[index];
  const [min, max] = range;
  const angle = faceDegrees(pose[slot], range);
  const percent = (degrees: number) => `${((degrees - min) / Math.max(1, max - min)) * 100}%`;

  // One shared sync; a failed one is tried again on the next nudge, and a new
  // `resync` value (another control moved a joint) starts a new one.
  async function sync() {
    if (syncedResyncRef.current !== resync) {
      syncedResyncRef.current = resync;
      syncRef.current = null;
    }

    syncRef.current ??= moveFacePose(pose, ranges, onMove);

    if (await syncRef.current) {
      return true;
    }

    syncRef.current = null;

    return false;
  }

  useEffect(() => {
    if (moveOnMount) {
      void sync();
    }
    // Mount only: later pose changes come from nudges, which move the robot.
    // eslint-disable-next-line react-hooks/exhaustive-deps, @eslint-react/exhaustive-deps
  }, []);

  async function nudge(delta: number) {
    const target = clamp(angle + delta, min, max);

    if (target === angle || !(await sync())) {
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
            {`${joint.name} ${faceDegrees(pose[each], ranges[joint.index])}°`}
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
