import { useState } from "preact/hooks";
import type { AnimationReply, Settings } from "../api/types";
import { Button } from "../components/Button";
import { ConfigSection } from "../components/ConfigSection";
import { FacePoseControls } from "../components/FacePoseControls";
import { useRobot } from "../robot";
import { savedFacePose, type Range } from "../servos";
import { useStatus } from "../status/status";

export function FacePoseSection({ settings, ranges }: { settings: Settings; ranges: Range[] }) {
  const { applySettings } = useRobot();
  const { perform } = useStatus();
  const [pose, setPose] = useState(() => savedFacePose(settings));
  const [previewing, setPreviewing] = useState(false);
  // A new key remounts the controls, so the next nudge syncs all joints again.
  const [controlsKey, setControlsKey] = useState(0);
  const isSet = settings.face_pose.length > 0;

  async function playAnimation(name: string) {
    const result = await perform<AnimationReply>({
      path: "/anim",
      request: { method: "POST", params: { name } },
      pending: "Setting animation…",
      failure: "Failed",
    });

    return result?.ok === true;
  }

  async function move(index: number, angle: number) {
    // A running preview would fight the manual move: no move while it runs.
    if (previewing) {
      if (!(await playAnimation("none"))) {
        return false;
      }

      setPreviewing(false);
    }

    const result = await perform({
      path: "/test/servo",
      request: { method: "POST", json: { index, angle } },
      pending: "Moving servo…",
      failure: "Move failed",
    });

    return result?.ok === true;
  }

  async function save(facePose: number[], done: string) {
    await perform<Settings>({
      path: "/settings",
      request: { method: "POST", json: { face_pose: facePose } },
      pending: "Saving face pose…",
      failure: "Save failed",
      done,
      onSuccess: (next) => {
        applySettings(next);
        setPose(savedFacePose(next));

        if (facePose.length === 0) {
          setControlsKey((key) => key + 1);
        }
      },
    });
  }

  async function clear() {
    if (!confirm("Clear the face me pose? Talking goes back to the default pose.")) {
      return;
    }

    await save([], "Face pose cleared.");
  }

  async function togglePreview() {
    if (await playAnimation(previewing ? "none" : "talking")) {
      setPreviewing(!previewing);
    }
  }

  return (
    <ConfigSection title="Face me" className="face-me-section">
      <p className="hint">
        Optional. Pick a joint and nudge it until the robot <strong>looks at you</strong>. Do this
        for head, neck and body, then save. The talking animation then faces you. Status:{" "}
        <strong id="face-pose-status">{isSet ? "Set" : "Not set (default pose)"}</strong>
      </p>
      <FacePoseControls
        key={controlsKey}
        pose={pose}
        ranges={ranges}
        onPose={setPose}
        onMove={move}
      />
      {isSet && (
        <p className="hint">
          The pose follows the saved servo limits. Check it again after a servo calibration.
        </p>
      )}
      <div className="servo-actions">
        <Button onClick={togglePreview}>{previewing ? "Stop preview" : "Preview talking"}</Button>
        {isSet && <Button onClick={clear}>Clear</Button>}
        <Button variant="primary" onClick={() => save(pose, "Face pose saved.")}>
          Save face pose
        </Button>
      </div>
    </ConfigSection>
  );
}
