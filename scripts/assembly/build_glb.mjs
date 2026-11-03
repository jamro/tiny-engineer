#!/usr/bin/env node
// Pack the assembled robot into one binary glTF (GLB) for the 3D viewer.
//
// Input: the assembly.json that scripts/assembly/extract.py wrote, the printed
// parts' STLs it names (3d_models/parts/<preset>/stl/) and the hardware STLs
// beside it. Output: one GLB with a node per part, at its assembled pose.
//
//   node scripts/assembly/build_glb.mjs artifacts/assembly/sg90/assembly.json web/assembly/models/sg90.glb
//
// The GLB keeps the CAD frame (millimetres, Z up) under a root node that turns
// it to glTF's Y up. Meshes carry positions and indices only; the viewer
// computes normals with a crease angle, so CAD edges stay sharp. A mesh used
// more than once (the five servos, the mirrored upper arm) is stored once.
// An STL with colours in its triangles' attribute bytes (VisCAM: bit 15 set,
// 5 bits each of red, green, blue, sRGB; the ESP32-C3-Zero from
// canfactory_board.ts) also gets vertex colours (COLOR_0, linear).
// Node 18+, no dependencies.

import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { dirname, isAbsolute, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
/** Vertices closer than this (mm) are welded into one. */
const WELD_MM = 1e-4;
/** Z up (CAD) to Y up (glTF), column-major. */
const Z_UP_TO_Y_UP = [1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1];

/** Triangle corners of a binary STL. */
export function readStl(bytes) {
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const count = view.getUint32(80, true);
  if (84 + count * 50 > bytes.byteLength) throw new Error('not a binary STL, or truncated');
  const corners = new Float32Array(count * 9);
  for (let t = 0; t < count; t++) {
    const base = 84 + t * 50 + 12;
    for (let k = 0; k < 9; k++) corners[t * 9 + k] = view.getFloat32(base + k * 4, true);
  }
  return corners;
}

/**
 * Per-triangle colours of a binary STL (linear RGB, three per triangle), or
 * null when not every triangle has one (bit 15 of its attribute bytes).
 */
export function readStlColors(bytes) {
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const count = view.getUint32(80, true);
  if (!count) return null;
  const linear = (five) => { const c = five / 31; return c <= 0.04045 ? c / 12.92 : ((c + 0.055) / 1.055) ** 2.4; };
  const colors = new Float32Array(count * 3);
  for (let t = 0; t < count; t++) {
    const word = view.getUint16(84 + t * 50 + 48, true);
    if (!(word & 0x8000)) return null;
    colors[t * 3] = linear((word >> 10) & 31);
    colors[t * 3 + 1] = linear((word >> 5) & 31);
    colors[t * 3 + 2] = linear(word & 31);
  }
  return colors;
}

/**
 * Shared vertices and indices from triangle corners; drops degenerate
 * triangles. With per-triangle `colors`, a vertex is shared only between
 * triangles of one colour, and `colors` comes back per vertex.
 */
export function weld(corners, triangleColors = null) {
  const lookup = new Map();
  const positions = [];
  const colors = [];
  const indices = [];
  const key = (x, y, z) => `${Math.round(x / WELD_MM)},${Math.round(y / WELD_MM)},${Math.round(z / WELD_MM)}`;
  for (let t = 0; t < corners.length / 9; t++) {
    const triangle = [];
    const tint = triangleColors ? `|${triangleColors[t * 3]},${triangleColors[t * 3 + 1]},${triangleColors[t * 3 + 2]}` : '';
    for (let c = 0; c < 3; c++) {
      const o = t * 9 + c * 3;
      const k = key(corners[o], corners[o + 1], corners[o + 2]) + tint;
      let index = lookup.get(k);
      if (index === undefined) {
        index = positions.length / 3;
        lookup.set(k, index);
        positions.push(corners[o], corners[o + 1], corners[o + 2]);
        if (triangleColors) colors.push(triangleColors[t * 3], triangleColors[t * 3 + 1], triangleColors[t * 3 + 2]);
      }
      triangle.push(index);
    }
    if (triangle[0] !== triangle[1] && triangle[1] !== triangle[2] && triangle[0] !== triangle[2]) indices.push(...triangle);
  }
  return {
    positions: Float32Array.from(positions),
    colors: triangleColors ? Float32Array.from(colors) : null,
    indices: positions.length / 3 > 65535 ? Uint32Array.from(indices) : Uint16Array.from(indices),
  };
}

function bounds(positions) {
  const min = [Infinity, Infinity, Infinity];
  const max = [-Infinity, -Infinity, -Infinity];
  for (let i = 0; i < positions.length; i += 3) {
    for (let a = 0; a < 3; a++) {
      min[a] = Math.min(min[a], positions[i + a]);
      max[a] = Math.max(max[a], positions[i + a]);
    }
  }
  return { min, max };
}

/** A GLB from the manifest; `load(path)` returns a mesh file's bytes. */
export function buildGlb(manifest, load) {
  const json = {
    asset: { version: '2.0', generator: 'tiny-engineer scripts/assembly/build_glb.mjs' },
    scene: 0,
    scenes: [{ name: `TinyEngineer-${manifest.preset}`, nodes: [0] }],
    nodes: [{ name: 'TinyEngineer', matrix: Z_UP_TO_Y_UP, children: [], extras: { preset: manifest.preset, units: 'mm', source: manifest.source } }],
    meshes: [],
    accessors: [],
    bufferViews: [],
    buffers: [{ byteLength: 0 }],
  };
  const chunks = [];
  let length = 0;
  const push = (array, target) => {
    const bytes = new Uint8Array(array.buffer, array.byteOffset, array.byteLength);
    json.bufferViews.push({ buffer: 0, byteOffset: length, byteLength: bytes.byteLength, target });
    chunks.push(bytes);
    length += bytes.byteLength;
    const pad = (4 - (length % 4)) % 4;
    if (pad) { chunks.push(new Uint8Array(pad)); length += pad; }
    return json.bufferViews.length - 1;
  };
  const meshIndex = new Map();
  const mesh = (path) => {
    if (meshIndex.has(path)) return meshIndex.get(path);
    const bytes = load(path);
    const { positions, colors, indices } = weld(readStl(bytes), readStlColors(bytes));
    if (!indices.length) throw new Error(`${path}: no triangles`);
    const { min, max } = bounds(positions);
    json.accessors.push({ bufferView: push(positions, 34962), componentType: 5126, count: positions.length / 3, type: 'VEC3', min, max });
    const attributes = { POSITION: json.accessors.length - 1 };
    if (colors) {
      json.accessors.push({ bufferView: push(colors, 34962), componentType: 5126, count: colors.length / 3, type: 'VEC3' });
      attributes.COLOR_0 = json.accessors.length - 1;
    }
    json.accessors.push({ bufferView: push(indices, 34963), componentType: indices instanceof Uint32Array ? 5125 : 5123, count: indices.length, type: 'SCALAR' });
    json.meshes.push({ name: path.split('/').pop().replace(/\.stl$/i, ''), primitives: [{ attributes, indices: json.accessors.length - 1 }] });
    meshIndex.set(path, json.meshes.length - 1);
    return json.meshes.length - 1;
  };
  const add = (entry, kind) => {
    const extras = { kind, component: entry.component };
    if (entry.print_group) extras.print_group = entry.print_group;
    if (entry.color) extras.color = entry.color;
    if (entry.mod) extras.mod = entry.mod;
    json.nodes.push({ name: entry.id, mesh: mesh(entry.mesh), matrix: entry.matrix, extras });
    json.nodes[0].children.push(json.nodes.length - 1);
  };
  for (const part of manifest.parts) add(part, 'printed');
  for (const item of manifest.hardware) add(item, 'hardware');
  json.buffers[0].byteLength = length;

  const text = new TextEncoder().encode(JSON.stringify(json));
  const jsonLength = Math.ceil(text.length / 4) * 4;
  const glb = new Uint8Array(12 + 8 + jsonLength + 8 + length);
  const view = new DataView(glb.buffer);
  view.setUint32(0, 0x46546c67, true); // glTF
  view.setUint32(4, 2, true);
  view.setUint32(8, glb.byteLength, true);
  view.setUint32(12, jsonLength, true);
  view.setUint32(16, 0x4e4f534a, true); // JSON
  glb.fill(0x20, 20, 20 + jsonLength);
  glb.set(text, 20);
  view.setUint32(20 + jsonLength, length, true);
  view.setUint32(24 + jsonLength, 0x004e4942, true); // BIN
  let offset = 28 + jsonLength;
  for (const chunk of chunks) { glb.set(chunk, offset); offset += chunk.byteLength; }
  return glb;
}

function main([manifestPath, outPath]) {
  if (!manifestPath || !outPath) {
    console.error('usage: build_glb.mjs <assembly.json> <out.glb>');
    return 2;
  }
  const manifest = JSON.parse(readFileSync(manifestPath, 'utf8'));
  if (manifest.format !== 1) throw new Error(`${manifestPath}: unsupported format ${manifest.format}`);
  const here = dirname(resolve(manifestPath));
  // Printed parts name repository paths; hardware meshes sit beside the manifest.
  const load = (path) => readFileSync(isAbsolute(path) ? path : path.startsWith('3d_models/') || path.startsWith('mods/') ? resolve(ROOT, path) : resolve(here, path));
  const glb = buildGlb(manifest, load);
  mkdirSync(dirname(resolve(outPath)), { recursive: true });
  writeFileSync(outPath, glb);
  console.log(`${manifest.parts.length} parts, ${manifest.hardware.length} hardware -> ${outPath} (${(glb.byteLength / 1048576).toFixed(2)} MB)`);
  return 0;
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) process.exitCode = main(process.argv.slice(2));
