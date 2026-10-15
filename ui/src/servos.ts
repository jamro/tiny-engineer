import type { Settings } from "./api/types";

export type Range = readonly [min: number, max: number];

export const SERVO_NAMES = ["Head", "Neck", "Left hand", "Right hand", "Body"];

export const DEFAULT_SERVO_RANGES: Range[] = [
  [60, 130],
  [40, 130],
  [45, 135],
  [35, 125],
  [40, 130],
];

export const savedRanges = ({ servo_mins: mins, servo_maxs: maxs }: Settings): Range[] =>
  mins.map((min, index) => [min, maxs[index]] as const);

export const inRange = (angle: number, [min, max]: Range) => angle >= min && angle <= max;

export const isCenter = (angle: number, [min, max]: Range) =>
  Number.isFinite(angle) && angle > min && angle < max;
