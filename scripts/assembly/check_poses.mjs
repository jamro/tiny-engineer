#!/usr/bin/env node
// Check the viewer's joint poses and animation loops against the meshes.
//
//   node scripts/assembly/check_poses.mjs artifacts/assembly/sg90/assembly.json web/assembly/steps.json
//
// Turns the parts about the servo shafts as the viewer does (JOINTS in
// web/js/assembly/poses.js, at DEFAULT_RANGE) and measures how deep parts
// that move relative to each other go into one another (clipping.mjs): at
// 90°, at rest, and through every animation loop. The press fits in the
// steps' `fits` are let through. The ring loop's strike must also land the
// right hand on the bell: touching it, not hovering above it.
//
// Options: --tolerance <mm> (default 0.3), --range <deg> (default DEFAULT_RANGE).
// Exit status 1 on any problem.
import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { loadSolids } from './check_clipping.mjs';
import { boxesOverlap, makeSolid, penetration } from './clipping.mjs';
import { ANIMATIONS, CENTRE, DEFAULT_RANGE, JOINTS, REST, RING_HIT_POSE } from '../../web/js/assembly/poses.js';

/** Largest gap (mm) between the striking hand and the bell that still counts as a hit. */
export const HIT_GAP_MM = 0.5;
/** Animation loops are sampled this often (s). */
const SAMPLE_S = 0.1;
const LOOP_S = 2.4;

const AXIS = { x: 0, y: 1, z: 2 };
const byId = Object.fromEntries(JOINTS.map((joint) => [joint.id, joint]));

/** The joints that turn a part, innermost first ([] for a part no joint moves). */
function chainOf(id) {
  let joint = JOINTS.find((entry) => entry.moves.includes(id));
  const chain = [];
  while (joint) {
    chain.push(joint);
    joint = byId[joint.parent];
  }
  return chain;
}

/** `p` turned by `degrees` about the line through `origin` along unit `axis`. */
function turn(p, origin, axis, degrees) {
  const t = (degrees * Math.PI) / 180;
  const c = Math.cos(t);
  const s = Math.sin(t);
  const v = [p[0] - origin[0], p[1] - origin[1], p[2] - origin[2]];
  const d = axis[0] * v[0] + axis[1] * v[1] + axis[2] * v[2];
  const cross = [axis[1] * v[2] - axis[2] * v[1], axis[2] * v[0] - axis[0] * v[2], axis[0] * v[1] - axis[1] * v[0]];
  return [0, 1, 2].map((k) => origin[k] + v[k] * c + cross[k] * s + axis[k] * d * (1 - c));
}

/**
 * Poses the solids: `posed(pose)` returns a Map id → solid with every joint
 * turned to `pose` (−1..1, `range` degrees either side of 90°), as joints.js
 * rigs them: each pivot on its servo's shaft (the servo frame's +Y).
 */
export function poser(solids, servoFrames, range = DEFAULT_RANGE) {
  const chains = new Map([...solids.keys()].map((id) => [id, chainOf(id)]));
  const shafts = Object.fromEntries(
    JOINTS.filter((joint) => servoFrames.has(joint.servo)).map((joint) => {
      const { origin, axis } = servoFrames.get(joint.servo);
      const [name, direction] = joint.sense;
      return [joint.id, { origin, axis, sign: Math.sign(axis[AXIS[name]]) * direction || direction }];
    }),
  );
  const posed = (pose) => {
    const out = new Map();
    for (const [id, solid] of solids) {
      const chain = chains.get(id).filter((joint) => shafts[joint.id] && pose[joint.id]);
      if (!chain.length) {
        out.set(id, solid);
        continue;
      }
      const tri = new Float64Array(solid.tri.length);
      for (let i = 0; i < tri.length; i += 3) {
        let p = [solid.tri[i], solid.tri[i + 1], solid.tri[i + 2]];
        for (const joint of chain) {
          const { origin, axis, sign } = shafts[joint.id];
          p = turn(p, origin, axis, pose[joint.id] * range * sign);
        }
        tri[i] = p[0];
        tri[i + 1] = p[1];
        tri[i + 2] = p[2];
      }
      out.set(id, makeSolid(id, tri));
    }
    return out;
  };
  return { posed, chains };
}

/** Servo shaft frames (origin, unit +Y) from a manifest. */
export function servoFrames(manifest) {
  const frames = new Map();
  for (const entry of manifest.hardware) {
    const m = entry.matrix;
    const axis = [m[4], m[5], m[6]];
    const length = Math.hypot(...axis);
    frames.set(entry.id, { origin: [m[12], m[13], m[14]], axis: axis.map((v) => v / length) });
  }
  return frames;
}

/** The poses to check: name → pose. */
export function posesToCheck() {
  const poses = { '90°': CENTRE, rest: REST };
  for (const [name, animation] of Object.entries(ANIMATIONS)) {
    for (let t = 0; t < LOOP_S - 1e-9; t += SAMPLE_S) poses[`${name} at ${t.toFixed(1)} s`] = animation.pose(t);
  }
  poses['ring strike'] = RING_HIT_POSE;
  return poses;
}

/**
 * Problems with the poses: parts that clip (pairs that a joint moves
 * relative to each other, deeper than `tolerance` and than any declared
 * fit), and a ring strike that misses the bell.
 */
export function checkPoses(solids, frames, data, { tolerance = 0.3, range = DEFAULT_RANGE, poses = posesToCheck() } = {}) {
  const { posed, chains } = poser(solids, frames, range);
  const pairKey = (a, b) => (a < b ? `${a}|${b}` : `${b}|${a}`);
  const fits = new Map((data?.fits ?? []).map((fit) => [pairKey(...fit.parts), fit.mm]));
  const chainKey = (id) => chains.get(id).map((joint) => joint.id).join('>');
  const ids = [...solids.keys()];
  const pairs = [];
  for (let i = 0; i < ids.length; i++) {
    for (let j = i + 1; j < ids.length; j++) if (chainKey(ids[i]) !== chainKey(ids[j])) pairs.push([ids[i], ids[j]]);
  }
  const problems = [];
  for (const [name, pose] of Object.entries(poses)) {
    const shaped = posed(pose);
    for (const [a, b] of pairs) {
      const sa = shaped.get(a);
      const sb = shaped.get(b);
      if (!boxesOverlap(sa.box, sb.box)) continue;
      const limit = Math.max(tolerance, fits.get(pairKey(a, b)) ?? 0);
      const depth = penetration(sa, sb, [0, 0, 0], 50, limit);
      if (depth > limit) problems.push(`${name}: ${a} × ${b} clip ${depth.toFixed(2)} mm`);
    }
  }
  // The strike lands: the hand touches the bell (raise the bell by the gap and they meet).
  if (solids.has('Bell') && solids.has('ForearmRight')) {
    const shaped = posed(RING_HIT_POSE);
    const hand = shaped.get('ForearmRight');
    const bell = shaped.get('Bell');
    if (penetration(hand, bell, [0, 0, HIT_GAP_MM]) <= 0.02) {
      problems.push(`ring strike: the right hand stops more than ${HIT_GAP_MM} mm above the bell`);
    }
  }
  return problems;
}

function main(argv) {
  const options = { tolerance: 0.3, range: DEFAULT_RANGE };
  const args = [];
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === '--tolerance') options.tolerance = Number(argv[++i]);
    else if (argv[i] === '--range') options.range = Number(argv[++i]);
    else args.push(argv[i]);
  }
  const [manifestPath, stepsPath] = args;
  if (!manifestPath || !(options.tolerance >= 0) || !(options.range > 0)) {
    console.error('usage: check_poses.mjs <assembly.json> [<steps.json>] [--tolerance mm] [--range deg]');
    return 2;
  }
  const manifest = JSON.parse(readFileSync(manifestPath, 'utf8'));
  const data = stepsPath ? JSON.parse(readFileSync(stepsPath, 'utf8')) : null;
  const solids = loadSolids(manifestPath);
  const poses = posesToCheck();
  const problems = checkPoses(solids, servoFrames(manifest), data, { ...options, poses });
  if (problems.length) {
    console.error(`${manifestPath}: ${problems.length} pose problem${problems.length === 1 ? '' : 's'} (±${options.range}°, tolerance ${options.tolerance} mm):`);
    for (const problem of problems) console.error(`  ${problem}`);
    return 1;
  }
  console.log(`${manifestPath}: ${Object.keys(poses).length} poses at ±${options.range}°, nothing clips, the ring strike hits the bell`);
  return 0;
}

const here = fileURLToPath(import.meta.url);
if (process.argv[1] && resolve(process.argv[1]) === here) process.exitCode = main(process.argv.slice(2));
