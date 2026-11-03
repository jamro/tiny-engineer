#!/usr/bin/env node
// Tests for the build-step clipping check (clipping.mjs, check_clipping.mjs), on boxes.
//   node scripts/assembly/test_clipping.mjs
import assert from 'node:assert/strict';
import { makeSolid, penetration, inside, surfaceDistance } from './clipping.mjs';
import { checkSteps, describe } from './check_clipping.mjs';
import { checkPoses, poser } from './check_poses.mjs';

/** A closed box, min corner `at`, edge lengths `size`, as STL-style triangle corners. */
function box(id, at, size) {
  const [x0, y0, z0] = at;
  const [x1, y1, z1] = at.map((v, k) => v + size[k]);
  const v = [[x0, y0, z0], [x1, y0, z0], [x1, y1, z0], [x0, y1, z0], [x0, y0, z1], [x1, y0, z1], [x1, y1, z1], [x0, y1, z1]];
  const faces = [[0, 2, 1], [0, 3, 2], [4, 5, 6], [4, 6, 7], [0, 1, 5], [0, 5, 4], [1, 2, 6], [1, 6, 5], [2, 3, 7], [2, 7, 6], [3, 0, 4], [3, 4, 7]];
  return makeSolid(id, Float32Array.from(faces.flatMap((face) => face.flatMap((i) => v[i]))));
}

const near = (actual, expected, slack = 0.05) => assert.ok(Math.abs(actual - expected) <= slack, `${actual} is not ${expected} ± ${slack}`);

// Point queries.
const cube = box('cube', [0, 0, 0], [10, 10, 10]);
assert.ok(inside(cube, [5, 5, 5]));
assert.ok(!inside(cube, [15, 5, 5]));
assert.ok(!inside(cube, [5, 5, -1]));
near(surfaceDistance(cube, [5, 5, 5]), 5, 1e-9);
near(surfaceDistance(cube, [2, 5, 5]), 2, 1e-9);
near(surfaceDistance(cube, [13, 14, 5]), 5, 1e-9);

// Penetration: apart, touching (a mating face), overlapping, one inside the other.
const other = box('other', [0, 0, 0], [10, 10, 10]);
assert.equal(penetration(cube, other, [20, 0, 0]), 0, 'apart');
near(penetration(cube, other, [10, 0, 0]), 0, 1e-6); // faces touch
near(penetration(cube, other, [8, 0, 0]), 2);
near(penetration(cube, other, [0, 0, 6.5]), 3.5);
const pin = box('pin', [4, 4, 4], [2, 2, 2]);
near(penetration(cube, pin), 4, 0.05);
near(penetration(pin, cube), 4, 0.05);
// Thin plates crossing like a plus sign: no corner of either is inside the other.
const plateX = box('plateX', [-10, -1, 0], [20, 2, 10]);
const plateY = box('plateY', [-1, -10, 0], [2, 20, 10]);
near(penetration(plateX, plateY), 1, 0.15);
// A floor stops the search early.
assert.equal(penetration(cube, other, [9.9, 0, 0], Infinity, 0.3), 0, 'shallower than the floor');

// Steps: a peg that drops into a cup clips its rim; one that drops straight down the hole does not.
const solids = new Map([
  ['cup', box('cup', [0, 0, 0], [10, 10, 2])],
  ['peg', box('peg', [0, 0, 2], [10, 10, 4])],
  ['lid', box('lid', [0, 0, 6], [10, 10, 1])],
]);
const steps = (fromPeg, extra = {}) => ({
  format: 1,
  sections: [{ id: 's', title: 'S', steps: [
    { title: 'cup', text: 't', add: ['cup'] },
    { title: 'peg', text: 't', add: ['peg'], from: fromPeg },
    { title: 'lid', text: 't', add: ['lid'], from: [0, 0, 20], ...extra },
  ] }],
});
let result = checkSteps(solids, steps([0, 0, 20]));
assert.deepEqual(result.problems, [], 'straight down');
result = checkSteps(solids, steps([0, 12, -3]));
assert.equal(result.problems.length, 1, 'slides through the cup');
assert.equal(result.problems[0].kind, 'path');
assert.equal(result.problems[0].a, 'peg');
assert.equal(result.problems[0].b, 'cup');
assert.ok(result.problems[0].depth > 1 && result.problems[0].depth <= 2.05, `depth ${result.problems[0].depth}`);
assert.match(describe(result.problems[0]), /step 2 \(s 2\) "peg": peg × cup clips 1\.\d\d mm/);
// A step's allowance lets a declared overlap through, and says so.
const allowSteps = steps([0, 12, -3]);
allowSteps.sections[0].steps[1].allow = [{ parts: ['cup', 'peg'], mm: 2.1, why: 'it flexes' }];
result = checkSteps(solids, allowSteps);
assert.deepEqual(result.problems, []);
assert.equal(result.allowed.length, 1);

// The finished assembly: overlapping parts are a problem unless they are a declared fit.
const pressed = new Map([...solids, ['lid', box('lid', [0, 0, 5.5], [10, 10, 1])]]);
result = checkSteps(pressed, steps([0, 0, 20]));
assert.ok(result.problems.some((problem) => problem.kind === 'assembled' && problem.b === 'lid'));
result = checkSteps(pressed, { ...steps([0, 0, 20]), fits: [{ parts: ['lid', 'peg'], mm: 0.6, why: 'press fit' }] });
assert.deepEqual(result.problems, []);

// A sub-assembly waits off the robot and is mounted in one move. Leave the lid
// out of the move and it is put on in place, so the peg comes down through it.
const sub = (move) => ({
  format: 1,
  sections: [{ id: 's', title: 'S', steps: [
    { title: 'peg', text: 't', add: ['peg'] },
    { title: 'lid', text: 't', add: ['lid'], from: [0, 0, 10] },
    { title: 'cup', text: 't', add: ['cup'], from: [30, 0, 0] },
    { title: 'mount', text: 't', add: [], move, from: [0, 0, 20] },
  ] }],
});
assert.deepEqual(checkSteps(solids, sub(['peg', 'lid'])).problems, []);
result = checkSteps(solids, sub(['peg']));
assert.ok(result.problems.some((problem) => problem.step.startsWith('4 ') && problem.a === 'peg' && problem.b === 'lid'), 'peg through the lid');

// Poses: a forearm in front of the right arm servo's shaft (+Y of the servo
// frame along −y, as in the model) turns up at +1 and down onto a bell below.
const shaft = new Map([['servo-arm-right', { origin: [0, 0, 0], axis: [0, -1, 0] }]]);
const arm = new Map([
  ['ForearmRight', box('ForearmRight', [10, -2, -2], [30, 4, 4])],
  ['Bell', box('Bell', [25, -5, -22], [15, 10, 10])],
]);
const { posed } = poser(arm, shaft, 25);
assert.ok(posed({ 'hand-right': 1 }).get('ForearmRight').box[5] > 10, 'right hand up at +1');
assert.ok(posed({ 'hand-right': -1 }).get('ForearmRight').box[2] < -10, 'and down at −1');
assert.equal(posed({}).get('ForearmRight'), arm.get('ForearmRight'), 'unmoved parts are reused');
const poseProblems = checkPoses(arm, shaft, null, { poses: { low: { 'hand-right': -1 } } });
assert.ok(poseProblems.some((problem) => problem.startsWith('low: ForearmRight × Bell clip')), poseProblems.join('; '));

console.log('clipping tests passed');
