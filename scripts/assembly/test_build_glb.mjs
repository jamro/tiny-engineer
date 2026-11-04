#!/usr/bin/env node
// Tests for build_glb.mjs: node scripts/assembly/test_build_glb.mjs
import assert from 'node:assert/strict';
import { buildGlb, readStl, readStlColors, weld } from './build_glb.mjs';
import { mergeMod } from './merge_mod.mjs';

/** A binary STL of the given triangles (each 9 numbers). */
function stl(triangles) {
  const bytes = new Uint8Array(84 + triangles.length * 50);
  const view = new DataView(bytes.buffer);
  view.setUint32(80, triangles.length, true);
  triangles.forEach((triangle, t) => triangle.forEach((value, k) => view.setFloat32(84 + t * 50 + 12 + k * 4, value, true)));
  return bytes;
}

// A tetrahedron: four triangles over four shared corners.
const [a, b, c, d] = [[0, 0, 0], [10, 0, 0], [0, 10, 0], [0, 0, 10]];
const tetra = stl([[...a, ...c, ...b], [...a, ...b, ...d], [...a, ...d, ...c], [...b, ...c, ...d]]);

function parseGlb(glb) {
  const view = new DataView(glb.buffer, glb.byteOffset, glb.byteLength);
  assert.equal(view.getUint32(0, true), 0x46546c67, 'glTF magic');
  assert.equal(view.getUint32(4, true), 2, 'version 2');
  assert.equal(view.getUint32(8, true), glb.byteLength, 'total length');
  const jsonLength = view.getUint32(12, true);
  assert.equal(view.getUint32(16, true), 0x4e4f534a, 'JSON chunk');
  const json = JSON.parse(new TextDecoder().decode(glb.subarray(20, 20 + jsonLength)));
  const binLength = view.getUint32(20 + jsonLength, true);
  assert.equal(view.getUint32(24 + jsonLength, true), 0x004e4942, 'BIN chunk');
  assert.equal(binLength, json.buffers[0].byteLength);
  return { json, bin: glb.subarray(28 + jsonLength, 28 + jsonLength + binLength) };
}

{
  const corners = readStl(tetra);
  assert.equal(corners.length, 36);
  const { positions, indices } = weld(corners);
  assert.equal(positions.length, 12, 'four welded vertices');
  assert.equal(indices.length, 12);
}

{
  // A degenerate triangle is dropped.
  const { indices } = weld(readStl(stl([[...a, ...a, ...b], [...a, ...b, ...c]])));
  assert.equal(indices.length, 3);
}

{
  const identity = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1];
  const moved = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 5, 6, 7, 1];
  const manifest = {
    format: 1, preset: 'test', source: 'x.f3d',
    parts: [
      { id: 'Left', component: 'Arm', mesh: 'arm.stl', matrix: identity },
      { id: 'Right', component: 'Arm(Mirror)', mesh: 'arm.stl', matrix: moved },
    ],
    hardware: [
      { id: 'servo-head', component: 'Servo', mesh: 'servo.stl', matrix: identity },
      { id: 'screw-hat-1', component: 'M2x8', mesh: 'M2x8.stl', matrix: moved, source: 'fastener', fastener: { kind: 'screw', size: 'M2×8', parent: 'Left' } },
    ],
  };
  const loads = [];
  const glb = buildGlb(manifest, (path) => { loads.push(path); return tetra; });
  const { json, bin } = parseGlb(glb);
  assert.deepEqual(loads, ['arm.stl', 'servo.stl', 'M2x8.stl'], 'a mesh used twice is read once');
  assert.equal(json.meshes.length, 3);
  assert.deepEqual(json.nodes.map((node) => node.name), ['TinyEngineer', 'Left', 'Right', 'servo-head', 'screw-hat-1']);
  assert.deepEqual(json.nodes[0].children, [1, 2, 3, 4]);
  assert.equal(json.nodes[4].extras.kind, 'fastener');
  assert.deepEqual(json.nodes[4].extras.fastener, { kind: 'screw', size: 'M2×8', parent: 'Left' });
  assert.equal(json.nodes[1].mesh, json.nodes[2].mesh, 'mirrored copy shares the mesh');
  assert.deepEqual(json.nodes[2].matrix, moved);
  assert.equal(json.nodes[3].extras.kind, 'hardware');
  assert.equal(json.nodes[1].extras.kind, 'printed');
  for (const view of json.bufferViews) assert.equal(view.byteOffset % 4, 0, 'aligned buffer views');
  const position = json.accessors[json.meshes[0].primitives[0].attributes.POSITION];
  assert.deepEqual(position.min, [0, 0, 0]);
  assert.deepEqual(position.max, [10, 10, 10]);
  assert.ok(bin.byteLength >= 2 * (12 * 4 + 12 * 2));
}

{
  // merge_mod: a group sharing a component frame, a group centred on a stock part, a prop on the floor.
  const I = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1];
  const at = (x, y, z) => [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1];
  const stock = {
    format: 1, preset: 'test',
    parts: [
      { id: 'Head', mesh: 'tetra.stl', matrix: at(100, 0, 50), occurrence: at(100, 0, 50) },
      { id: 'Hat', mesh: 'tetra.stl', matrix: I },
      { id: 'Mug', mesh: 'tetra.stl', matrix: at(30, 40, 20) },
      { id: 'Desk', mesh: 'tetra.stl', matrix: at(0, 0, -5) },
    ],
    hardware: [
      { id: 'screw-hat-1', mesh: 'M2x8.stl', matrix: I, fastener: { parts: ['Hat', 'Head'] } },
      { id: 'screw-desk-1', mesh: 'M2x8.stl', matrix: I, fastener: { parts: ['Desk'] } },
    ],
  };
  const mod = {
    format: 1, preset: 'test',
    parts: [
      { id: 'Head', mesh: 'tetra.stl', matrix: I, occurrence: I },
      { id: 'Cover', mesh: 'tetra.stl', matrix: at(0, 0, 10), occurrence: at(0, 0, 10) },
      { id: 'Pot', mesh: 'tetra.stl', matrix: at(-7, -7, -7), occurrence: I },
      { id: 'Pumpkin', mesh: 'tetra.stl', matrix: I, occurrence: I },
    ],
  };
  const config = {
    id: 'test', replace: { Mug: 'Pot' }, remove: ['Hat'], colors: { Cover: '#ef7d1a' },
    align: [
      { parts: ['Head', 'Cover'], component: 'Head' },
      { parts: ['Pot'], centre: { part: 'Pot', onto: 'Mug' }, floor: { part: 'Pot', onto: 'Mug' } },
      { parts: ['Pumpkin'], at: [200, 0] },
    ],
  };
  const merged = mergeMod(stock, mod, config, () => tetra);
  const byId = Object.fromEntries(merged.parts.map((part) => [part.id, part]));
  assert.deepEqual(Object.keys(byId).sort(), ['Cover', 'Desk', 'Head', 'Pot', 'Pumpkin']);
  assert.deepEqual(byId.Cover.matrix.slice(12, 15), [100, 0, 60], 'cover follows the shared Head frame');
  assert.deepEqual(byId.Pot.matrix.slice(12, 15), [30, 40, 20], 'pot takes the mug spot');
  assert.deepEqual(byId.Pumpkin.matrix.slice(12, 15), [195, -5, -5], 'prop on the floor, footprint centred at [200, 0]');
  assert.equal(byId.Cover.color, '#ef7d1a');
  assert.equal(byId.Cover.mod, 'test');
  assert.deepEqual(merged.hardware.map((item) => item.id), ['screw-desk-1'], 'screws through removed or replaced parts go');
  assert.throws(() => mergeMod(stock, mod, { ...config, align: config.align.slice(0, 2) }, () => tetra), /Pumpkin: in the mod but in no align group/);
}

// Colours in the attribute bytes (VisCAM): vertices are shared only within one colour.
{
  const coloured = stl([[...a, ...c, ...b], [...a, ...b, ...d]]);
  const view = new DataView(coloured.buffer);
  view.setUint16(84 + 48, 0x8000 | (31 << 10), true); // red
  view.setUint16(84 + 50 + 48, 0x8000 | 31, true); // blue
  const colors = readStlColors(coloured);
  assert.deepEqual([...colors], [1, 0, 0, 0, 0, 1]);
  const welded = weld(readStl(coloured), colors);
  assert.equal(welded.positions.length / 3, 6, 'a and b are not shared across colours');
  assert.deepEqual([...welded.colors.subarray(0, 3)], [1, 0, 0]);
  assert.equal(readStlColors(tetra), null, 'no colours without bit 15');
  const json = parseGlb(buildGlb({ preset: 'sg90', parts: [], hardware: [{ id: 'ESP32-C3-Zero', component: 'ESP32-C3-Zero', mesh: 'board.stl', matrix: [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1] }] }, () => coloured)).json;
  assert.ok(json.meshes[0].primitives[0].attributes.COLOR_0 !== undefined, 'COLOR_0');
}

console.log('build_glb tests passed');
