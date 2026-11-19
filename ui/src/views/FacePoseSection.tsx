import { useState } from "preact/hooks";
import type { AnimationReply, Settings } from "../api/types";
import { Button } from "../components/Button";
import { ConfigSection } from "../components/ConfigSection";
import { FacePoseControls } from "../components/FacePoseControls";
import { useRobot } from "../robot";
import { moveFacePose, savedFacePose, type Range } from "../servos";
import { useStatus } from "../status/status";

export function FacePoseSection({
  settings,
  ranges,
  manualMoves,
}: {
  settings: Settings;
  ranges: Range[];
  // Counts moves from the Servo control form; a change resyncs the face controls.
  manualMoves: number;
}) {
  const { applySettings } = useRobot();
  const { perform } = useStatus();
  const [pose, setPose] = useState(() => savedFacePose(settings));
  const [previewing, setPreviewing] = useState(false);
  // Preview start / stop moves the joints (stop parks neck and body), so each
  // one makes the next nudge sync all joints again, like a form move.
  const [previewChanges, setPreviewChanges] = useState(0);
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

  // Save stores only a pose the robot reached: all joints move to it first.
  async function saveCurrent() {
    if (await moveFacePose(pose, ranges, move)) {
      await save(pose, "Face pose saved.");
    }
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
      setPreviewChanges((count) => count + 1);
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
        resync={manualMoves + previewChanges}
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
        <Button variant="primary" onClick={saveCurrent}>
          Save face pose
        </Button>
      </div>
    </ConfigSection>
  );
}
