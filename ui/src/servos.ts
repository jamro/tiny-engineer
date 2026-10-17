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
