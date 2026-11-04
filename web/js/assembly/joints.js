// The robot's five servo joints (JOINTS in poses.js) rigged over the model's parts.
//
// Each servo turns about its shaft: the servo mesh's frame has the shaft on
// its +Y axis through the origin, so a joint's pivot is the servo node's
// position and its axis the node's +Y. The table follows
// docs/robot-movement.md: body → (neck → head, left hand, right hand).
import * as THREE from 'three';
import { DEFAULT_RANGE, JOINTS } from './poses.js';

export { DEFAULT_RANGE, JOINTS };

const AXES = { x: new THREE.Vector3(1, 0, 0), y: new THREE.Vector3(0, 1, 0), z: new THREE.Vector3(0, 0, 1) };

/**
 * Builds pivots under `root` (the CAD-frame node) and moves the parts under
 * them. `nodes` maps part id → Object3D; `joints` is JOINTS, or JOINTS with
 * the screws and nuts added (withFollowers). Returns `set(id, value)` for a
 * pose in −1..1, `reset()`, and the joints that could be built.
 */
export function rig(root, nodes, joints = JOINTS) {
  root.updateMatrixWorld(true);
  const pivots = new Map();
  const built = [];
  for (const joint of joints) {
    const servo = nodes.get(joint.servo);
    if (!servo) continue;
    const parent = joint.parent ? pivots.get(joint.parent)?.pivot : root;
    if (!parent) continue;
    // Axis and origin of the shaft, in the root frame (the model is at rest).
    const servoInRoot = new THREE.Matrix4().copy(root.matrixWorld).invert().multiply(servo.matrixWorld);
    const origin = new THREE.Vector3().setFromMatrixPosition(servoInRoot);
    const axis = new THREE.Vector3(0, 1, 0).transformDirection(servoInRoot).normalize();
    const pivot = new THREE.Group();
    pivot.name = `joint-${joint.id}`;
    pivot.position.copy(origin);
    if (parent !== root) parent.worldToLocal(root.localToWorld(pivot.position));
    parent.add(pivot);
    pivot.updateMatrixWorld(true);
    for (const id of joint.moves) {
      const node = nodes.get(id);
      if (node) pivot.attach(node);
    }
    // Turn about the shaft in the direction `sense` asks for.
    const [axisName, direction] = joint.sense;
    const sign = Math.sign(axis.dot(AXES[axisName])) * direction || direction;
    pivots.set(joint.id, { pivot, axis, sign, joint });
    built.push(joint);
  }
  // Child pivots built later were parented to their parent's pivot already.
  const set = (id, value, range = DEFAULT_RANGE) => {
    const entry = pivots.get(id);
    if (!entry) return;
    const clamped = Math.max(-1, Math.min(1, value));
    entry.pivot.quaternion.setFromAxisAngle(entry.axis, THREE.MathUtils.degToRad(clamped * range * entry.sign));
  };
  const reset = () => { for (const { pivot } of pivots.values()) pivot.quaternion.identity(); };
  return { joints: built, set, reset, pivots };
}
