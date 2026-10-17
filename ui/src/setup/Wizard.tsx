import { useEffect, useState } from "preact/hooks";
import type { Settings } from "../api/types";
import { useRobot } from "../robot";
import { Button } from "../components/Button";
import { useStatus } from "../status/status";
import { useResettableState } from "../useResettableState";
import { FindRanges } from "./FindRanges";
import { LedStep } from "./LedStep";
import { NetworkStep } from "./NetworkStep";
import { PlaceParts } from "./PlaceParts";
import { ScreenStep } from "./ScreenStep";
import { SpeakerStep } from "./SpeakerStep";
import { useCalibration } from "./useCalibration";
import { releaseLed, useLedMapping } from "./useLedMapping";

const STEPS = [
  { id: "servos", title: "Servos" },
  { id: "oled", title: "Screen" },
  { id: "led", title: "RGB mapping" },
  { id: "speaker", title: "Speaker" },
  { id: "network", title: "Connection" },
] as const;

type StepId = (typeof STEPS)[number]["id"];

export function Wizard({ settings }: { settings: Settings }) {
  const { applySettings } = useRobot();
  const { perform } = useStatus();
  const [stepIndex, setStepIndex] = useState(0);
  const [oledRotate180, setOledRotate180] = useResettableState(settings.oled_rotate_180, Boolean);
  const calibration = useCalibration(settings);
  const led = useLedMapping(settings.rgb_order);
  const step = STEPS[stepIndex];
  const nextStep = STEPS.at(stepIndex + 1);
  const placingParts = step.id === "servos" && calibration.phase === "horns";
  const ready: Partial<Record<StepId, boolean>> = { servos: calibration.valid, led: led.valid };

  // A hold left over from an earlier visit would keep the LED lit.
  useEffect(() => {
    void releaseLed();
  }, []);

  async function save(json: Partial<Settings>, pending: string) {
    const result = await perform<Settings>({
      path: "/settings",
      request: { method: "POST", json },
      pending,
      failure: "Save failed",
      onSuccess: applySettings,
    });

    return result?.ok === true;
  }

  // What each step saves before the wizard moves on.
  const saves: Partial<Record<StepId, () => Promise<boolean>>> = {
    servos: () =>
      save(
        {
          servo_mins: calibration.ranges.map(([min]) => min),
          servo_maxs: calibration.ranges.map(([, max]) => max),
        },
        "Saving servo ranges…",
      ),
    oled: () => save({ oled_rotate_180: oledRotate180 }, "Saving screen rotation…"),
    led: () => save({ rgb_order: led.order }, "Saving LED mapping…"),
  };

  async function next() {
    const saved = (await saves[step.id]?.()) ?? true;

    if (saved) {
      setStepIndex(stepIndex + 1);
    }
  }

  function back() {
    if (step.id === "servos") {
      calibration.setPhase("horns");

      return;
    }

    if (step.id === "oled") {
      calibration.setPhase("ranges");
    }

    setStepIndex(stepIndex - 1);
  }

  return (
    <div>
      <h2 className="page-title">Tiny Engineer setup</h2>
      <p className="page-desc">{`Step ${stepIndex + 1} of ${STEPS.length} · ${step.title}`}</p>
      <div className="setup-progress-track">
        <div
          className="setup-progress-bar"
          style={{ width: `${((stepIndex + 1) / STEPS.length) * 100}%` }}
        ></div>
      </div>
      {step.id === "servos" &&
        (placingParts ? (
          <PlaceParts calibration={calibration} />
        ) : (
          <FindRanges calibration={calibration} />
        ))}
      {step.id === "oled" && (
        <ScreenStep rotate180={oledRotate180} onRotate={() => setOledRotate180(!oledRotate180)} />
      )}
      {step.id === "led" && <LedStep mapping={led} />}
      {step.id === "speaker" && <SpeakerStep />}
      {step.id === "network" && <NetworkStep settings={settings} />}
      {!placingParts && (
        <div className="setup-footer">
          <Button onClick={back}>Back</Button>
          {nextStep && (
            <Button variant="primary" disabled={ready[step.id] === false} onClick={next}>
              {`Next: ${nextStep.title}`}
            </Button>
          )}
        </div>
      )}
    </div>
  );
}
