#!/usr/bin/env node
// Tests for the assembly viewer's browser-independent modules (web/js/assembly/).
//   node scripts/assembly/test_viewer.mjs
import assert from 'node:assert/strict';
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

console.log('viewer tests passed');
