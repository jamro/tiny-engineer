// Joint poses in the firmware's pose space (−1..1 of each joint's saved range,
// docs/robot-movement.md). No DOM or three.js: tested in Node
// (scripts/assembly/test_viewer.mjs).

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

/**
 * JOINTS with more parts that hang on them: `followers` maps a part id to
 * the part it is fastened to (a screw or nut to its `fastener.parent`), and
 * it turns with that part's joint.
 */
export function withFollowers(joints, followers) {
  const extra = new Map();
  for (const [id, parent] of followers) {
    const joint = joints.find((entry) => entry.moves.includes(parent));
    if (joint) extra.set(joint.id, [...(extra.get(joint.id) ?? []), id]);
  }
  return joints.map((joint) => (extra.has(joint.id) ? { ...joint, moves: [...joint.moves, ...extra.get(joint.id)] } : joint));
}

/** The fastener followers of a manifest's hardware (id → parent). */
export function fastenerFollowers(hardware) {
  return new Map((hardware ?? []).filter((entry) => entry.fastener).map((entry) => [entry.id, entry.fastener.parent]));
}

/** Degrees either side of 90° that a −1..1 pose reaches in the viewer. */
export const DEFAULT_RANGE = 25;

/** All joints at 90° (the pose the robot is assembled and modelled in). */
export const CENTRE = Object.freeze({ head: 0, neck: 0, 'hand-left': 0, 'hand-right': 0, body: 0 });

/** Resting over the keyboard: left hand at max, right at min (both down). */
export const REST = Object.freeze({ head: -0.6, neck: 0, 'hand-left': 1, 'hand-right': -1, body: 0 });

const clamp = (value) => Math.max(-1, Math.min(1, value));

/** A pose with every joint in −1..1; missing joints at 0. */
export function normalizePose(pose) {
  return Object.fromEntries(JOINT_IDS.map((id) => [id, clamp(Number(pose?.[id]) || 0)]));
}
