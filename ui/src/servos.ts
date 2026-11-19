import type { Settings } from "./api/types";

export type Range = readonly [min: number, max: number];

export const SERVO_NAMES = ["Head", "Neck", "Left hand", "Right hand", "Body"];

export const DEFAULT_SERVO_RANGES: Range[] = [
  [80, 100],
  [80, 100],
  [80, 100],
  [80, 100],
  [80, 100],
];

export const savedRanges = ({ servo_mins: mins, servo_maxs: maxs }: Settings): Range[] =>
  mins.map((min, index) => [min, maxs[index]] as const);

export const inRange = (angle: number, [min, max]: Range) => angle >= min && angle <= max;

export const isStockServoRanges = (ranges: readonly Range[]) =>
  ranges.length === DEFAULT_SERVO_RANGES.length &&
  ranges.every(
    ([min, max], index) =>
      min === DEFAULT_SERVO_RANGES[index][0] && max === DEFAULT_SERVO_RANGES[index][1],
  );

// Same mapping as the firmware's servoNormToDeg: -1 = min, 0 = mid, 1 = max.
export const normToDeg = (n: number, [min, max]: Range) =>
  Math.round(((min + max) / 2 + (Math.max(-1, Math.min(1, n)) * (max - min)) / 2) * 10) / 10;

// Inverse of normToDeg; angles outside the range saturate to -1 / 1.
export const degToNorm = (angle: number, [min, max]: Range) =>
  max > min
    ? Math.round(Math.max(-1, Math.min(1, (2 * (angle - min)) / (max - min) - 1)) * 10000) / 10000
    : 0;

// The face pose drives head, neck and body, in this order.
export const FACE_JOINTS = [
  { index: 0, name: "Head", low: "down", high: "up" },
  { index: 1, name: "Neck", low: "right", high: "left" },
  { index: 4, name: "Body", low: "right", high: "left" },
] as const;

// The talking rest pose the firmware uses when no face pose is set.
export const DEFAULT_FACE_POSE: readonly number[] = [1 / 7, 0, 0];

export const savedFacePose = ({ face_pose: pose }: Settings): number[] =>
  pose.length === FACE_JOINTS.length ? [...pose] : [...DEFAULT_FACE_POSE];
