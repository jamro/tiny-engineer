import type { ComponentChildren } from "preact";
import { useState } from "preact/hooks";
import type { Settings } from "../api/types";
import { type Range, savedRanges, SERVO_NAMES } from "../servos";
import { useResettableState } from "../useResettableState";

export const CENTER = 90;
export const IDENTIFY_DELTA = 3;

export const JOINT = {
  head: 0,
  neck: 1,
  leftHand: 2,
  rightHand: 3,
  body: 4,
} as const;

export type CalibMode = "hub" | "assembly" | "guided";

export type GuidedKind = "center" | "min" | "hand";

export interface GuidedStep {
  id: string;
  kind: GuidedKind;
  joint: number;
  bound?: "min" | "max";
  title: string;
  target: string;
  copy: ComponentChildren;
}

export const GUIDED_STEPS: GuidedStep[] = [
  {
    id: "body-center",
    kind: "center",
    joint: JOINT.body,
    title: "Body · center",
    target: "Body centered in the chair",
    copy: (
      <>
        Nudge until the body sits <strong>centered in the chair</strong>. Confirm when it looks{" "}
        <strong>straight</strong>.
      </>
    ),
  },
  {
    id: "body-min",
    kind: "min",
    joint: JOINT.body,
    title: "Body · minimum",
    target: "Right hand over the bell",
    copy: (
      <>
        Rotate the body toward the robot’s right until the <strong>right hand</strong> sits{" "}
        <strong>over the bell</strong>. Stop before <strong>cables pull taut</strong>. Max will
        mirror this about the center you set.
      </>
    ),
  },
  {
    id: "neck-center",
    kind: "center",
    joint: JOINT.neck,
    title: "Neck · center",
    target: "Neck facing straight forward",
    copy: (
      <>
        Nudge until the neck yaw faces <strong>straight forward</strong>.
      </>
    ),
  },
  {
    id: "neck-min",
    kind: "min",
    joint: JOINT.neck,
    title: "Neck · minimum",
    target: "Neck toward the robot’s right",
    copy: (
      <>
        Pan toward the electrical minimum (<strong>robot’s right</strong>). Choose a stop before{" "}
        <strong>cables bind</strong>. Max will mirror this about the center you set.
      </>
    ),
  },
  {
    id: "head-center",
    kind: "center",
    joint: JOINT.head,
    title: "Head · center",
    target: "Head looking straight forward",
    copy: (
      <>
        Nudge until the head looks <strong>straight forward</strong>.
      </>
    ),
  },
  {
    id: "head-min",
    kind: "min",
    joint: JOINT.head,
    title: "Head · minimum",
    target: "Head looking at the computer screen",
    copy: (
      <>
        Tilt down until the head looks at the <strong>computer screen</strong>. Stop before{" "}
        <strong>parts collide</strong>. Max will mirror this about the center you set.
      </>
    ),
  },
  {
    id: "hand-right-min",
    kind: "hand",
    joint: JOINT.rightHand,
    bound: "min",
    title: "Right hand · keyboard",
    target: "Right hand just above the keyboard",
    copy: (
      <>
        Set the right hand over the <strong>laptop keyboard</strong> with a little slack (not
        touching). This is the <strong>electrical minimum</strong>.
      </>
    ),
  },
  {
    id: "hand-left-max",
    kind: "hand",
    joint: JOINT.leftHand,
    bound: "max",
    title: "Left hand · keyboard",
    target: "Left hand matching the keyboard height",
    copy: (
      <>
        Match the same <strong>keyboard height</strong> on the left hand. This is the{" "}
        <strong>electrical maximum</strong>.
      </>
    ),
  },
  {
    id: "hand-left-min",
    kind: "hand",
    joint: JOINT.leftHand,
    bound: "min",
    title: "Left hand · upper",
    target: "Left upper arm roughly horizontal",
    copy: (
      <>
        Raise the left hand to the <strong>upper stop</strong> (upper arm roughly horizontal). This
        is the <strong>electrical minimum</strong>.
      </>
    ),
  },
  {
    id: "hand-right-max",
    kind: "hand",
    joint: JOINT.rightHand,
    bound: "max",
    title: "Right hand · upper",
    target: "Right hand matching the upper height",
    copy: (
      <>
        Raise the right hand to the same <strong>upper height</strong>. This is the{" "}
        <strong>electrical maximum</strong>.
      </>
    ),
  },
];

export function mirrorMax(center: number, min: number): number {
  return 2 * center - min;
}

export function rangeMid([min, max]: Range): number {
  return Math.round((min + max) / 2);
}

export function useCalibration(settings: Settings) {
  const [mode, setMode] = useState<CalibMode>("hub");
  const [guidedIndex, setGuidedIndex] = useState(0);
  const [angles, setAngles] = useState(() => SERVO_NAMES.map(() => CENTER));
  const [guidedDrafts, setGuidedDrafts] = useState<(number | undefined)[]>(() =>
    GUIDED_STEPS.map(() => undefined),
  );
  // Session-only guided centers — never saved to NVS / settings.
  const [centers, setCenters] = useState<(number | undefined)[]>(() =>
    SERVO_NAMES.map(() => undefined),
  );
  const [ranges, setRanges] = useResettableState(settings, savedRanges);

  const guidedStep = GUIDED_STEPS[guidedIndex] ?? GUIDED_STEPS[0];

  function clearGuidedSession() {
    setCenters(SERVO_NAMES.map(() => undefined));
    setGuidedDrafts(GUIDED_STEPS.map(() => undefined));
  }

  function goHub() {
    setMode("hub");
    setGuidedIndex(0);
    clearGuidedSession();
  }

  function startAssembly() {
    setMode("assembly");
  }

  function startGuided() {
    clearGuidedSession();
    setGuidedIndex(0);
    setAngles(ranges.map((range) => rangeMid(range)));
    setMode("guided");
  }

  function setJointCenter(joint: number, angle: number) {
    setCenters((current) => current.map((each, index) => (index === joint ? angle : each)));
  }

  function setGuidedDraft(index: number, angle: number) {
    setGuidedDrafts((current) =>
      current.map((draft, draftIndex) => (draftIndex === index ? angle : draft)),
    );
  }

  function guidedBack() {
    if (guidedIndex <= 0) {
      goHub();

      return;
    }

    setGuidedIndex(guidedIndex - 1);
  }

  function guidedNext() {
    if (guidedIndex >= GUIDED_STEPS.length - 1) {
      return false;
    }

    setGuidedIndex(guidedIndex + 1);

    return true;
  }

  function setJointRange(joint: number, range: Range) {
    setRanges((current) => current.map((each, index) => (index === joint ? range : each)));
  }

  function setJointAngle(joint: number, angle: number) {
    setAngles((current) => current.map((each, index) => (index === joint ? angle : each)));
  }

  return {
    mode,
    setMode,
    goHub,
    startAssembly,
    startGuided,
    guidedIndex,
    guidedStep,
    guidedBack,
    guidedNext,
    guidedDrafts,
    setGuidedDraft,
    isLastGuidedStep: guidedIndex >= GUIDED_STEPS.length - 1,
    angles,
    setAngles,
    setJointAngle,
    centers,
    setJointCenter,
    ranges,
    setRanges,
    setJointRange,
    valid: ranges.every(([min, max]) => min < max),
  };
}

export type Calibration = ReturnType<typeof useCalibration>;
