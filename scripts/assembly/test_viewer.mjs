#!/usr/bin/env node
// Tests for the assembly viewer's browser-independent modules (web/js/assembly/).
//   node scripts/assembly/test_viewer.mjs
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import {
  CENTER_BENCH_PARTS, CENTER_COVER_PARTS, FASTEN_AT, flatten, offsetsAt, phases, sceneAt,
  showsBenchElectronics, showsCenterCallout, startOffset, validate, withFasteners,
} from '../../web/js/assembly/steps.js';
import { defaultColor, resolveColors, GROUP_COLORS, HARDWARE_COLORS, PART_COLORS } from '../../web/js/assembly/colors.js';

const parts = [
  { id: 'Head', kind: 'printed', printGroup: 'PartsSetA' },
  { id: 'Desk', kind: 'printed', printGroup: 'PartsSetB' },
  { id: 'LampDiffuser', kind: 'printed', printGroup: null },
  { id: 'UpperArmMirror', kind: 'printed', printGroup: 'PartsSetB' },
  { id: 'servo-head', kind: 'hardware', component: 'Servo' },
];

assert.equal(defaultColor(parts[0]), GROUP_COLORS.PartsSetA);
assert.equal(defaultColor(parts[1]), GROUP_COLORS.PartsSetB);
assert.equal(defaultColor(parts[2]), PART_COLORS.LampDiffuser);
assert.equal(defaultColor({ ...parts[0], color: '#ff0000' }), GROUP_COLORS.PartsSetA, 'part.color overrides ignored');
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

// Optional center flag: boolean when present.
{
  const withCenter = structuredClone(data);
  withCenter.sections[0].steps[0].center = true;
  assert.deepEqual(validate(withCenter, ['Head', 'Hat', 'Desk']), []);
  withCenter.sections[0].steps[0].center = 'yes';
  assert.ok(validate(withCenter, ['Head', 'Hat', 'Desk']).some((problem) => problem.includes('center must be a boolean')));
}

// The shipped steps are well formed (part ids are checked against the model by check_steps.mjs).
const shipped = JSON.parse(readFileSync(new URL('../../web/assembly/steps.json', import.meta.url), 'utf8'));
const shippedIds = [...new Set(flatten(shipped).flatMap((step) => step.add))];
assert.deepEqual(validate(shipped, shippedIds), []);
const shippedSteps = flatten(shipped);
const centeredTitles = shippedSteps.filter((step) => step.center === true).map((step) => step.title);
assert.deepEqual(centeredTitles, [
  'Center the neck servo',
  'Fit Neck onto the head servo',
  'Center the last servo',
  'Mount the arms on the chest servos',
]);
assert.ok(shipped.sections.some((section, index, list) => {
  const prev = list[index - 1];
  return section.id === 'bench-window' && prev?.id === 'hat' && list[index + 1]?.id === 'neck';
}), 'bench-window sits after hat and before neck');
assert.equal(shippedSteps.find((step) => step.title === "Seat the robot's left servo")?.center, undefined);
assert.equal(shippedSteps.find((step) => step.title === "Seat the robot's right servo")?.center, undefined);
assert.equal(showsCenterCallout({ center: true }), true);
assert.equal(showsCenterCallout({ center: false }), false);
assert.equal(showsCenterCallout({}), false);
assert.equal(showsBenchElectronics({ bench: true }), true);
assert.equal(showsBenchElectronics({ center: true }), true);
assert.equal(showsBenchElectronics({}), false);
for (const step of shippedSteps) {
  assert.equal(showsCenterCallout(step), step.center === true);
  assert.equal(showsBenchElectronics(step), step.center === true || step.bench === true);
}

// Bench-window and centering steps preview the board / ESP32; desk covers ghost when present.
{
  const bench = shippedSteps.findIndex((step) => step.title === 'Keep electronics on the desk');
  const intro = sceneAt(shippedSteps, bench);
  assert.equal(showsCenterCallout(shippedSteps[bench]), false, 'bench intro has no centering callout');
  assert.ok(intro.preview.has('MainControlBoard'));
  assert.ok(intro.preview.has('ESP32-C3-Zero'));
  assert.ok(intro.preview.has('ESP32-pin-headers'));
  for (const id of CENTER_BENCH_PARTS) assert.ok(intro.focus.includes(id));

  const neckCenter = shippedSteps.findIndex((step) => step.title === 'Center the neck servo');
  const early = sceneAt(shippedSteps, neckCenter);
  assert.ok(early.preview.has('MainControlBoard'));
  assert.ok(early.preview.has('ESP32-C3-Zero'));
  assert.ok(early.preview.has('ESP32-pin-headers'));
  assert.ok(!early.shown.has('MainControlBoard'), 'preview only until electronics are added');
  for (const id of CENTER_BENCH_PARTS) assert.ok(early.focus.includes(id));

  const arms = shippedSteps.findIndex((step) => step.title === 'Mount the arms on the chest servos');
  const late = sceneAt(shippedSteps, arms);
  assert.equal(late.preview.size, 0, 'board already added: not a preview');
  assert.ok(late.shown.has('MainControlBoard'));
  assert.ok(late.shown.has('ESP32-C3-Zero'));
  for (const id of ['Desk', 'DeskPad', 'DeskTop', 'LampBase']) {
    assert.ok(late.ghost.has(id), `${id} ghosted so the board stays readable`);
  }
  assert.ok(CENTER_COVER_PARTS.includes('Desk'));
}

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
}

console.log('viewer tests passed');
