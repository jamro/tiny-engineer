#!/usr/bin/env node
// Tests for the assembly viewer's browser-independent modules (web/js/assembly/).
//   node scripts/assembly/test_viewer.mjs
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { flatten, sceneAt, startOffset, validate } from '../../web/js/assembly/steps.js';
import { decodeColors, defaultColor, encodeColors, normalizeHex, resolveColors, GROUP_COLORS, HARDWARE_COLORS } from '../../web/js/assembly/colors.js';

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

// Only changed colours are encoded, and they round-trip.
const changes = { Head: '#ff0000', Desk: GROUP_COLORS.PartsSetB, UpperArmMirror: '#00ff00' };
const encoded = encodeColors(parts, changes);
assert.equal(encoded, 'Head.ff0000~UpperArmMirror.00ff00');
assert.deepEqual(decodeColors(encoded, parts), { Head: '#ff0000', UpperArmMirror: '#00ff00' });

// Unknown parts and bad values are ignored.
assert.deepEqual(decodeColors('Nope.ff0000~Head.zzzzzz~Desk.123456~garbage~.~%E0.ffffff', parts), { Desk: '#123456' });

const resolved = resolveColors(parts, { Head: '#ff0000' });
assert.equal(resolved.Head, '#ff0000');
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

// The shipped steps are well formed (part ids are checked against the model by check_steps.mjs).
const shipped = JSON.parse(readFileSync(new URL('../../web/assembly/steps.json', import.meta.url), 'utf8'));
const shippedIds = [...new Set(flatten(shipped).flatMap((step) => step.add))];
assert.deepEqual(validate(shipped, shippedIds), []);

console.log('viewer tests passed');
