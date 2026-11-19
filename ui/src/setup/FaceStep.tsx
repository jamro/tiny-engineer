import { ConfigSection } from "../components/ConfigSection";
import { FacePoseControls } from "../components/FacePoseControls";
import type { Range } from "../servos";
import { useSetupServo } from "./useSetupServo";

// Optional: Skip keeps whatever face pose the robot already has.
export function FaceStep({
  pose,
  ranges,
  onPose,
}: {
  pose: readonly number[];
  ranges: readonly Range[];
  onPose: (pose: number[]) => void;
}) {
  const moveServo = useSetupServo();

  return (
    <ConfigSection title="Face me">
      <p className="setup-copy">
        Optional. Put the robot where it will sit on your desk. The robot moves to the pose shown
        below. Pick head, neck or body and nudge it until the robot <strong>looks at you</strong>.
        The talking animation then faces you. Tap <strong>Skip</strong> to keep the default pose;
        you can set it later on the Servo page.
      </p>
      <FacePoseControls
        pose={pose}
        ranges={ranges}
        onPose={onPose}
        onMove={(index, angle) => moveServo({ index, angle })}
        moveOnMount
      />
    </ConfigSection>
  );
}
