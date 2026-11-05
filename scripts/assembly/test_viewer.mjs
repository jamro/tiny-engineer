#!/usr/bin/env node
// Tests for the assembly viewer's browser-independent modules (web/js/assembly/).
//   node scripts/assembly/test_viewer.mjs
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { FASTEN_AT, flatten, offsetsAt, phases, sceneAt, startOffset, validate, withFasteners } from '../../web/js/assembly/steps.js';
import { CENTRE, JOINTS, JOINT_IDS, REST, blendPoses, fastenerFollowers, normalizePose, withFollowers } from '../../web/js/assembly/poses.js';
import { defaultColor, normalizeHex, resolveColors, GROUP_COLORS, HARDWARE_COLORS } from '../../web/js/assembly/colors.js';

const parts = [
  { id: 'Head', kind: 'printed', printGroup: 'PartsSetA' },
  { id: 'Desk', kind: 'printed', printGroup: 'PartsSetB' },
  { id: 'LampDiffuser', kind: 'printed', printGroup: null },
  { id: 'UpperArmMirror', kind: 'printed', printGroup: 'PartsSetB' },
  { id: 'servo-head', kind: 'hardware', component: 'Servo' },
];

assert.equal(normalizeHex('#ABCDEF'), '#abcdef');
assert.equal(normalizeHex('abcdef'), '#abcdef');
assert.equal(normalizeHex('#abc'), null);
assert.equal(normalizeHex('javascript:alert(1)'), null);

assert.equal(defaultColor(parts[0]), GROUP_COLORS.PartsSetA);
assert.equal(defaultColor(parts[1]), GROUP_COLORS.PartsSetB);
assert.notEqual(defaultColor(parts[2]), GROUP_COLORS.PartsSetB, 'diffuser has its own colour');
assert.equal(defaultColor(parts[4]), HARDWARE_COLORS.Servo);

const resolved = resolveColors(parts);
assert.equal(resolved.Head, GROUP_COLORS.PartsSetA);
assert.equal(resolved.Desk, GROUP_COLORS.PartsSetB);
assert.equal(Object.keys(resolved).length, parts.length);

// Steps.
const data = {
  format: 1,
  sections: [
    { id: 'a', title: 'A', steps: [
      { title: 'one', text: 't', add: ['Head'], from: [0, 0, 10] },
      { title: 'two', text: 't', add: ['Hat', 'Desk'], fromEach: { Hat: [0, 0, 5] } },
    ] },
    { id: 'b', title: 'B', steps: [{ title: 'look', text: 't', add: [], focus: ['Desk'], ghost: ['Head'] }] },
  ],
};
const steps = flatten(data);
assert.equal(steps.length, 3);
assert.equal(steps[1].section.id, 'a');
assert.equal(steps[2].index, 0);
assert.deepEqual(startOffset(steps[1], 'Hat'), [0, 0, 5]);
assert.deepEqual(startOffset(steps[1], 'Desk'), [0, 0, 0]);
assert.deepEqual([...sceneAt(steps, 0).shown], ['Head']);
const at1 = sceneAt(steps, 1);
assert.deepEqual([...at1.shown].sort(), ['Desk', 'Hat', 'Head']);
assert.deepEqual([...at1.moving].sort(), ['Desk', 'Hat']);
const at2 = sceneAt(steps, 2);
assert.equal(at2.moving.size, 0);
assert.deepEqual(at2.focus, ['Desk']);
assert.ok(at2.ghost.has('Head'));
assert.deepEqual(validate(data, ['Head', 'Hat', 'Desk']), []);
const problems = validate(data, ['Head', 'Hat', 'Desk', 'Bell']);
assert.deepEqual(problems, ['no step adds Bell']);
const twice = structuredClone(data);
twice.sections[1].steps[0].add = ['Head'];
assert.ok(validate(twice, ['Head', 'Hat', 'Desk']).some((problem) => problem.includes('adds Head again')));

// Sub-assemblies: parts wait at the sum of the moves still ahead of them.
const staged = {
  format: 1,
  sections: [{ id: 'a', title: 'A', steps: [
    { title: 'head', text: 't', add: ['Head'], from: [0, 0, 10] },
    { title: 'hat', text: 't', add: ['Hat'], from: [0, 0, 5] },
    { title: 'desk', text: 't', add: ['Desk'] },
    { title: 'mount', text: 't', add: [], move: ['Head', 'Hat'], from: [0, 0, 40] },
    { title: 'slide', text: 't', add: [], move: ['Head', 'Hat', 'Desk'], fromEach: { Head: [20, 0, 0], Hat: [20, 0, 0], Desk: [0, 9, 0] } },
  ] }],
};
const stagedSteps = flatten(staged);
assert.deepEqual(Object.fromEntries(offsetsAt(stagedSteps, 0, 0)), { Head: [20, 0, 50] }, 'arrives at its waiting place + from');
assert.deepEqual(Object.fromEntries(offsetsAt(stagedSteps, 0, 1)), { Head: [20, 0, 40] });
assert.deepEqual(Object.fromEntries(offsetsAt(stagedSteps, 1, 0.5)), { Head: [20, 0, 40], Hat: [20, 0, 42.5] });
assert.deepEqual(Object.fromEntries(offsetsAt(stagedSteps, 3, 0.25)), { Head: [20, 0, 30], Hat: [20, 0, 30], Desk: [0, 9, 0] });
assert.deepEqual(Object.fromEntries(offsetsAt(stagedSteps, 4, 1)), {}, 'all home');
assert.deepEqual([...sceneAt(stagedSteps, 3).moving].sort(), ['Hat', 'Head']);
assert.deepEqual(sceneAt(stagedSteps, 3).focus.sort(), ['Hat', 'Head'], 'a move frames what it moves');
assert.deepEqual(validate(staged, ['Head', 'Hat', 'Desk']), []);
const badMove = structuredClone(staged);
badMove.sections[0].steps[0].move = ['Desk'];
badMove.sections[0].steps[3].from = undefined;
badMove.sections[0].steps[4].allow = [{ parts: ['Head', 'Nope'], mm: 0 }];
const badProblems = validate(badMove, ['Head', 'Hat', 'Desk']);
assert.ok(badProblems.some((problem) => problem.includes('moves Desk, which no earlier step adds')));
assert.ok(badProblems.some((problem) => problem.includes('moves parts without a from')));
assert.ok(badProblems.some((problem) => problem.includes('unknown part Nope')));
assert.ok(badProblems.some((problem) => problem.includes('mm must be a positive number')));
assert.ok(badProblems.some((problem) => problem.includes('say why')));

// The shipped steps are well formed (part ids are checked against the model by check_steps.mjs).
const shipped = JSON.parse(readFileSync(new URL('../../web/assembly/steps.json', import.meta.url), 'utf8'));
const shippedIds = [...new Set(flatten(shipped).flatMap((step) => step.add))];
assert.deepEqual(validate(shipped, shippedIds), []);

// Poses.
assert.deepEqual(normalizePose({ head: 3, neck: -2, body: 'x' }), { head: 1, neck: -1, 'hand-left': 0, 'hand-right': 0, body: 0 });
assert.deepEqual(blendPoses(CENTRE, REST, 1), { ...REST });
assert.equal(blendPoses(CENTRE, REST, 0.5)['hand-left'], 0.5);
assert.equal(REST['hand-left'], 1);
assert.equal(REST['hand-right'], -1);
assert.deepEqual(JOINTS.map((joint) => joint.id).sort(), [...JOINT_IDS].sort(), 'one joint per pose id');

// Fasteners in the steps.
{
  const data = {
    format: 1,
    sections: [
      { id: 'a', title: 'A', steps: [{ title: 'Seat the arm', text: 't', add: ['Arm'], from: [0, 0, 30] }] },
      { id: 'b', title: 'B', steps: [
        { title: 'Fasten', text: 't', add: [] },
        { title: 'Mount', text: 't', add: ['Body'], move: ['Arm'], fromEach: { Arm: [10, 0, 0] } },
      ] },
    ],
  };
  const hardware = [
    { id: 'screw-arm-1', fastener: { kind: 'screw', parent: 'Arm', parts: ['Arm', 'Body'], step: 'Seat the arm', from: [0, 0, 5] } },
    { id: 'screw-fasten-1', fastener: { kind: 'screw', parent: 'Arm', parts: ['Arm'], step: 'Fasten', from: [0, -4, 0] } },
    { id: 'servo' },
  ];
  const withScrews = withFasteners(data, hardware);
  const steps = flatten(withScrews);
  assert.deepEqual(steps[0].add, ['Arm', 'screw-arm-1']);
  assert.deepEqual(steps[0].fasten, ['screw-arm-1']);
  assert.deepEqual(steps[0].fromEach['screw-arm-1'], [0, 0, 30], 'rides with the arm');
  assert.deepEqual(steps[1].fromEach['screw-fasten-1'], [0, 0, 0], 'nothing moves: it only slides');
  assert.deepEqual(steps[2].move, ['Arm', 'screw-arm-1', 'screw-fasten-1'], 'moves with the arm later');
  assert.deepEqual(steps[2].fromEach['screw-arm-1'], [10, 0, 0]);
  assert.deepEqual(validate(withScrews, ['Arm', 'Body', 'screw-arm-1', 'screw-fasten-1']), []);
  assert.deepEqual(withFasteners(data, [{ id: 'servo' }]), data, 'no fasteners: the steps as they are');
  assert.deepEqual(validate(withFasteners(data, [{ id: 'lost', fastener: { ...hardware[0].fastener, step: 'Nope' } }]), ['Arm', 'Body', 'lost']), ['no step adds lost']);
  assert.deepEqual(phases(steps[0]), [0, FASTEN_AT, 1]);
  assert.deepEqual(phases(data.sections[0].steps[0]), [0, 1]);
  assert.deepEqual(offsetsAt(steps, 0, 0).get('screw-arm-1'), [10, 0, 35]);
  assert.deepEqual(offsetsAt(steps, 0, FASTEN_AT).get('screw-arm-1'), [10, 0, 5], 'the arm is in, the screw still out');
  assert.deepEqual(offsetsAt(steps, 0, FASTEN_AT).get('Arm'), [10, 0, 0]);
  assert.deepEqual(offsetsAt(steps, 0, 1).get('screw-arm-1'), [10, 0, 0], 'driven in');
  assert.equal(offsetsAt(steps, 2, 1).get('screw-arm-1'), undefined, 'home with the arm');
  const joints = withFollowers(JOINTS, fastenerFollowers([{ id: 'screw-elbow-left-1', fastener: { parent: 'ForearmLeft' } }, { id: 'nut', fastener: { parent: 'Nope' } }]));
  assert.ok(joints.find((joint) => joint.id === 'hand-left').moves.includes('screw-elbow-left-1'));
  assert.ok(!joints.some((joint) => joint.moves.includes('nut')), 'a part no joint moves: it stays');
  assert.ok(!JOINTS.find((joint) => joint.id === 'hand-left').moves.includes('screw-elbow-left-1'), 'JOINTS itself is left alone');
}

console.log('viewer tests passed');
