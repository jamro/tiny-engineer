// Joint poses in the firmware's pose space (−1..1 of each joint's saved range,
// docs/robot-movement.md), and short animation loops built from the
// firmware's pose constants (src/animation/constants.h). No DOM or three.js:
// tested in Node (scripts/assembly/test_viewer.mjs).

export const JOINT_IDS = ['head', 'neck', 'hand-left', 'hand-right', 'body'];

/** All joints at 90° (the pose the robot is assembled and modelled in). */
export const CENTRE = Object.freeze({ head: 0, neck: 0, 'hand-left': 0, 'hand-right': 0, body: 0 });

/** Resting over the keyboard: left hand at max, right at min (both down). */
export const REST = Object.freeze({ head: -0.6, neck: 0, 'hand-left': 1, 'hand-right': -1, body: 0 });

const wave = (t, period, phase = 0) => Math.sin(((t / period) + phase) * 2 * Math.PI);
const pulse = (t, period, phase = 0) => (wave(t, period, phase) + 1) / 2; // 0..1

// From src/animation/constants.h.
const TYPING_HAND_BAND = 1 / 3;
const TYPING_HEAD_BAND = 2 / 7;
const TYPING_SWAY = 1 / 9;
const TALKING_HEAD_MID = 1 / 7;
const TALKING_HEAD_NOD = 2 / 7;
const TALKING_NECK_SWAY = 1 / 6;
const TALKING_BODY_SWAY = 1 / 10;
const TALKING_HAND_LIFT = TYPING_HAND_BAND * 1.4;
const RING_RIGHT_START = 1;
const RING_RIGHT_STRIKE = -8 / 9;
const RING_HEAD_STRIKE = -1;
const RING_HEAD_START = 2 / 7;
const WELCOME_HEAD_UP = 2 / 7;
const WELCOME_HAND_RAISED = 1;
const WELCOME_HAND_WIGGLE = 8 / 45;

/** Looping animations: `pose(t)` for t in seconds. */
export const ANIMATIONS = {
  typing: {
    label: 'Typing',
    pose: (t) => ({
      head: -1 + TYPING_HEAD_BAND * pulse(t, 3.1),
      neck: TYPING_SWAY * wave(t, 5.3),
      'hand-left': 1 - TYPING_HAND_BAND * pulse(t, 0.34),
      'hand-right': -1 + TYPING_HAND_BAND * pulse(t, 0.34, 0.5),
      body: TYPING_SWAY * wave(t, 7.7, 0.25),
    }),
  },
  talking: {
    label: 'Talking',
    pose: (t) => ({
      head: TALKING_HEAD_MID + (TALKING_HEAD_NOD / 2) * wave(t, 0.9),
      neck: TALKING_NECK_SWAY * wave(t, 3.7),
      'hand-left': 1 - TALKING_HAND_LIFT * pulse(t, 2.3),
      'hand-right': -1 + TALKING_HAND_LIFT * pulse(t, 2.9, 0.4),
      body: TALKING_BODY_SWAY * wave(t, 5.1),
    }),
  },
  ring: {
    label: 'Ring the bell',
    // Lift the right hand, strike down onto the bell, bounce, every 2.4 s.
    pose: (t) => {
      const phase = (t % 2.4) / 2.4;
      const strike = phase < 0.55 ? 0 : phase < 0.65 ? (phase - 0.55) / 0.1 : 1 - (phase - 0.65) / 0.35;
      return {
        head: RING_HEAD_START + (RING_HEAD_STRIKE - RING_HEAD_START) * strike * 0.5,
        neck: -0.15,
        'hand-left': 1,
        'hand-right': RING_RIGHT_START + (RING_RIGHT_STRIKE - RING_RIGHT_START) * strike,
        body: -0.35,
      };
    },
  },
  wave: {
    label: 'Wave',
    pose: (t) => ({
      head: WELCOME_HEAD_UP * (0.6 + 0.4 * pulse(t, 1.7)),
      neck: 0.2 * wave(t, 4.1),
      'hand-left': 1,
      'hand-right': WELCOME_HAND_RAISED - WELCOME_HAND_WIGGLE * 2 * pulse(t, 0.6),
      body: 0.1 * wave(t, 6.3),
    }),
  },
};

const clamp = (value) => Math.max(-1, Math.min(1, value));

/** A pose with every joint in −1..1; missing joints at 0. */
export function normalizePose(pose) {
  return Object.fromEntries(JOINT_IDS.map((id) => [id, clamp(Number(pose?.[id]) || 0)]));
}

/** Between two poses, `f` in 0..1. */
export function blendPoses(from, to, f) {
  return Object.fromEntries(JOINT_IDS.map((id) => [id, from[id] + (to[id] - from[id]) * f]));
}
