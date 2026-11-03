// Joint poses in the firmware's pose space (−1..1 of each joint's saved range,
// docs/robot-movement.md), and short animation loops built from the
// firmware's pose constants (src/animation/constants.h). No DOM or three.js:
// tested in Node (scripts/assembly/test_viewer.mjs).

export const JOINT_IDS = ['head', 'neck', 'hand-left', 'hand-right', 'body'];

/**
 * The five servo joints, as a kinematic tree over the model's parts: rigged
 * by joints.js, and checked against the meshes by
 * scripts/assembly/check_poses.mjs. `moves`: parts the joint turns (besides its child joints). `index` is the
 * firmware's servo index. `sense` says what +1 (the joint's saved max) looks
 * like, as a rotation about a CAD axis (+x the robot's front, +y its left,
 * +z up): body and neck turn left (+z), the head tilts up (−y), the left hand
 * goes down (+y) and the right hand up (−y) (docs/robot-movement.md).
 */
export const JOINTS = [
  {
    id: 'body', index: 4, label: 'Body', servo: 'servo-body', sense: ['z', 1],
    moves: ['BellyJoint', 'Belly', 'Chest', 'servo-neck', 'servo-arm-left', 'servo-arm-right'],
    hint: 'Torso left / right',
  },
  { id: 'neck', index: 1, label: 'Neck', servo: 'servo-neck', parent: 'body', sense: ['z', 1], moves: ['Neck'], hint: 'Head left / right' },
  { id: 'head', index: 0, label: 'Head', servo: 'servo-head', parent: 'neck', sense: ['y', -1], moves: ['Head', 'Hat', 'OLED', 'servo-head'], hint: 'Face up / down' },
  { id: 'hand-left', index: 2, label: 'Left hand', servo: 'servo-arm-left', parent: 'body', sense: ['y', 1], moves: ['UpperArm', 'ForearmLeft'], hint: 'Down at +1' },
  { id: 'hand-right', index: 3, label: 'Right hand', servo: 'servo-arm-right', parent: 'body', sense: ['y', -1], moves: ['UpperArmMirror', 'ForearmRight'], hint: 'Up at +1' },
];

/** Degrees either side of 90° that a −1..1 pose reaches in the viewer. */
export const DEFAULT_RANGE = 25;

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
const RING_BODY_START = -1;
const RING_NECK_START = 0;
const RING_LEFT_START = 7 / 9;
const RING_RIGHT_START = 1;
const RING_RIGHT_BOUNCE = -5 / 9;
const RING_HEAD_STRIKE = -1;
const RING_HEAD_START = 2 / 7;
/**
 * Where the right hand meets the bell in the model at DEFAULT_RANGE, with the
 * body at RING_BODY_START. The firmware strikes to −8/9 on a calibrated
 * range; at the viewer's range that stops 2 mm above the bell.
 * scripts/assembly/check_poses.mjs checks the hand touches it here.
 */
export const RING_RIGHT_HIT = -0.97;
export const RING_HIT_POSE = Object.freeze({
  head: RING_HEAD_STRIKE, neck: RING_NECK_START, 'hand-left': RING_LEFT_START, 'hand-right': RING_RIGHT_HIT, body: RING_BODY_START,
});
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
    // As the firmware does it (src/animation/ring.cpp): turn right towards
    // the bell with the right hand up, strike down onto it fast while the
    // head looks down, bounce off, lift the hand again. Every 2.4 s.
    pose: (t) => {
      const s = t % 2.4;
      // Right hand: up, strike (accelerating), on the bell, bounce, rest, lift.
      let hand;
      let look; // 0 head up (start), 1 looking down at the bell
      if (s < 0.5) [hand, look] = [RING_RIGHT_START, 0];
      else if (s < 0.8) {
        const f = (s - 0.5) / 0.3;
        [hand, look] = [RING_RIGHT_START + (RING_RIGHT_HIT - RING_RIGHT_START) * f * f, Math.min(1, f * 1.4)];
      } else if (s < 0.88) [hand, look] = [RING_RIGHT_HIT, 1];
      else if (s < 1.05) {
        const f = 1 - (1 - (s - 0.88) / 0.17) ** 2;
        [hand, look] = [RING_RIGHT_HIT + (RING_RIGHT_BOUNCE - RING_RIGHT_HIT) * f, 1];
      } else if (s < 1.5) [hand, look] = [RING_RIGHT_BOUNCE, 1];
      else {
        const f = (1 - Math.cos(((s - 1.5) / 0.9) * Math.PI)) / 2;
        [hand, look] = [RING_RIGHT_BOUNCE + (RING_RIGHT_START - RING_RIGHT_BOUNCE) * f, 1 - f];
      }
      return {
        head: RING_HEAD_START * (1 - look) + RING_HEAD_STRIKE * look,
        neck: RING_NECK_START,
        'hand-left': RING_LEFT_START,
        'hand-right': hand,
        body: RING_BODY_START,
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
