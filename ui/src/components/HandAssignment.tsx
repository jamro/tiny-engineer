import type { Settings } from "../api/types";
import { useRobot } from "../robot";
import { useStatus } from "../status/status";
import { useResettableState } from "../useResettableState";
import { Button } from "./Button";
import { ConfigSection } from "./ConfigSection";

export function HandAssignment({ settings }: { settings: Settings }) {
  const { applySettings } = useRobot();
  const { perform } = useStatus();
  const [swapped, setSwapped] = useResettableState(settings.swap_hands, Boolean);

  async function save() {
    await perform<Settings>({
      path: "/settings",
      request: { method: "POST", json: { swap_hands: swapped } },
      pending: "Saving hand assignment…",
      failure: "Hand assignment save failed",
      done: "Hand assignment saved. Restart the robot to apply it before moving the hands.",
      onSuccess: applySettings,
    });
  }

  return (
    <ConfigSection title="Hand servo assignment" apply="reboot">
      <p className="setup-copy">
        If the hands move on the wrong side, swap their channels here. Left and right are from the
        robot’s perspective. Restart after saving, then check each hand and its calibration.
      </p>
      <div className="form-group">
        <label htmlFor="hand-channel">Left hand servo channel</label>
        <select
          id="hand-channel"
          value={swapped ? "3" : "2"}
          onChange={(event) => setSwapped(event.currentTarget.value === "3")}
        >
          <option value="2">2 — right hand on 3 (default)</option>
          <option value="3">3 — right hand on 2 (swapped)</option>
        </select>
        <p className="hint">
          {`Saved assignment: left on ${settings.swap_hands ? 3 : 2}, right on ${settings.swap_hands ? 2 : 3}. Applies after restart.`}
        </p>
      </div>
      <Button disabled={swapped === settings.swap_hands} onClick={save}>
        Save hand assignment
      </Button>
    </ConfigSection>
  );
}
