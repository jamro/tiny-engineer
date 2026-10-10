export const DEFAULT_SERVO_RANGES = [
  [60, 130],
  [40, 130],
  [45, 135],
  [35, 125],
  [40, 130],
];

export const cloneRanges = (ranges) => ranges.map(([min, max]) => [min, max]);

export const servoRanges = cloneRanges(DEFAULT_SERVO_RANGES);

const listeners = [];

export function onServoRangesChange(listener) {
  listeners.push(listener);
}

export const rangesToParams = (ranges) => ({
  servo_mins: ranges.map(([min]) => min).join(","),
  servo_maxs: ranges.map(([, max]) => max).join(","),
});

export function applyServoRanges(settings) {
  const { servo_mins: mins, servo_maxs: maxs } = settings;

  if (mins?.length !== servoRanges.length || maxs?.length !== servoRanges.length) {
    return;
  }

  mins.forEach((min, index) => {
    servoRanges[index] = [min, maxs[index]];
  });

  for (const listener of listeners) {
    listener();
  }
}
