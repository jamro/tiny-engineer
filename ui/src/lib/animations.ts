/** Shared animation names for Animations page and Actions picker. */
export const ANIMATIONS = [
  ["none", "Idle", "Rest pose"],
  ["typing", "Typing", "Continuous"],
  ["reading", "Reading", "Continuous"],
  ["thinking", "Thinking", "Continuous"],
  ["talking", "Talking", "Continuous"],
  ["ring", "Bell", "One-shot"],
  ["welcome", "Welcome", "One-shot"],
  ["wakeup", "Wakeup", "Sleep inertia"],
  ["sleep", "Sleep", "Close + OLED off"],
  ["attention", "Attention", "+ audio, 1 min hold"],
  ["error", "Error", "+ audio, 1 min hold"],
  ["abort", "Abort", "One-shot + audio"],
  ["dead", "Dead", "dead.wav, then X X"],
] as const;

export type AnimationName = (typeof ANIMATIONS)[number][0];
