import { useState } from "preact/hooks";
import type { Settings } from "../api/types";
import { isCenter, savedRanges, SERVO_NAMES } from "../servos";
import { useResettableState } from "../useResettableState";

export const CENTER = 90;

// Servo step state: "horns" while the printed parts go on, then "ranges" to find each joint's limits.
export function useCalibration(settings: Settings) {
  const [phase, setPhase] = useState<"horns" | "ranges">("horns");
  const [joint, setJoint] = useState(0);
  const [angles, setAngles] = useState(() => SERVO_NAMES.map(() => CENTER));
  const [ranges, setRanges] = useResettableState(settings, savedRanges);
  const [centers, setCenters] = useResettableState(settings.servo_centers, (saved) => saved);

  return {
    phase,
    setPhase,
    joint,
    setJoint,
    angles,
    setAngles,
    ranges,
    setRanges,
    centers,
    setCenters,
    valid: ranges.every(([min, max], i) => min < max && isCenter(centers[i], [min, max])),
    placePartsDone: () => {
      setPhase("ranges");
      setJoint(0);
      setAngles(angles.map((angle, index) => (index === 0 ? CENTER : angle)));
    },
  };
}

export type Calibration = ReturnType<typeof useCalibration>;
