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
  copy: string;
}

export const GUIDED_STEPS: GuidedStep[] = [
  {
    id: "body-center",
    kind: "center",
    joint: JOINT.body,
    title: "Body · center",
    copy: "Nudge until the body sits centered in the chair. Confirm when it looks straight.",
  },
  {
    id: "body-min",
    kind: "min",
    joint: JOINT.body,
    title: "Body · minimum",
    copy: "Rotate the body toward the robot’s right until the right hand sits over the bell. Stop before cables pull taut. Max will mirror this about the center you set.",
  },
  {
    id: "neck-center",
    kind: "center",
    joint: JOINT.neck,
    title: "Neck · center",
    copy: "Nudge until the neck yaw faces straight forward.",
  },
  {
    id: "neck-min",
    kind: "min",
    joint: JOINT.neck,
    title: "Neck · minimum",
    copy: "Pan toward the electrical minimum (robot’s right). Choose a stop before cables bind. Max will mirror this about the center you set.",
  },
  {
    id: "head-center",
    kind: "center",
    joint: JOINT.head,
    title: "Head · center",
    copy: "Nudge until the head looks straight forward.",
  },
  {
    id: "head-min",
    kind: "min",
    joint: JOINT.head,
    title: "Head · minimum",
    copy: "Tilt down until the head looks at the computer screen. Stop before parts collide. Max will mirror this about the center you set.",
  },
  {
    id: "hand-right-min",
    kind: "hand",
    joint: JOINT.rightHand,
    bound: "min",
    title: "Right hand · keyboard",
    copy: "Set the right hand over the laptop keyboard with a little slack (not touching). This is the electrical minimum.",
  },
  {
    id: "hand-left-max",
    kind: "hand",
    joint: JOINT.leftHand,
    bound: "max",
    title: "Left hand · keyboard",
    copy: "Match the same keyboard height on the left hand. This is the electrical maximum.",
  },
  {
    id: "hand-left-min",
    kind: "hand",
    joint: JOINT.leftHand,
    bound: "min",
    title: "Left hand · upper",
    copy: "Raise the left hand to the upper stop (upper arm roughly horizontal). This is the electrical minimum.",
  },
  {
    id: "hand-right-max",
    kind: "hand",
    joint: JOINT.rightHand,
    bound: "max",
    title: "Right hand · upper",
    copy: "Raise the right hand to the same upper height. This is the electrical maximum.",
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
  // Session-only guided centers — never saved to NVS / settings.
  const [centers, setCenters] = useState<(number | undefined)[]>(() =>
    SERVO_NAMES.map(() => undefined),
  );
  const [ranges, setRanges] = useResettableState(settings, savedRanges);

  const guidedStep = GUIDED_STEPS[guidedIndex] ?? GUIDED_STEPS[0];

  function clearCenters() {
    setCenters(SERVO_NAMES.map(() => undefined));
  }

  function goHub() {
    setMode("hub");
    setGuidedIndex(0);
    clearCenters();
  }

  function startAssembly() {
    setMode("assembly");
  }

  function startGuided() {
    clearCenters();
    setGuidedIndex(0);
    setAngles(ranges.map((range) => rangeMid(range)));
    setMode("guided");
  }

  function setJointCenter(joint: number, angle: number) {
    setCenters((current) => current.map((each, index) => (index === joint ? angle : each)));
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
