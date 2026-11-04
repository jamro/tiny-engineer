#!/usr/bin/env node
// Put a mod's parts on the stock robot, for the 3D viewer.
//
//   node scripts/assembly/merge_mod.mjs <stock assembly.json> <mod assembly.json> <web/assembly/mods/<id>.json> <out assembly.json>
//
// The mod's assembly.json comes from extract.py --mod: its parts in the mod
// design's own frame. The viewer config (web/assembly/mods/<id>.json) says how
// each group of them lines up with the stock robot:
//   component: the mod keeps a stock component, so that component's frame is shared;
//   centre / floor: centre a part's footprint on a stock part's, and stand it
//     on the stock part's floor (min z);
//   at: [x, y] on the floor beside the robot (a prop);
//   check: after aligning, a part's footprint must land on a stock part's.
// Then it removes `remove`, swaps `replace`, and replaces same-id stock parts,
// and drops the stock screws and nuts that go through any of those.
// Node 18+, no dependencies.

import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { readStl } from './build_glb.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');

// 4x4 column-major helpers.
const multiply = (a, b) => {
  const out = new Array(16).fill(0);
  for (let c = 0; c < 4; c++) for (let r = 0; r < 4; r++) for (let k = 0; k < 4; k++) out[c * 4 + r] += a[k * 4 + r] * b[c * 4 + k];
  return out;
};
const invertRigid = (m) => {
  const r = [m[0], m[1], m[2], m[4], m[5], m[6], m[8], m[9], m[10]]; // columns
  const t = [m[12], m[13], m[14]];
  // R^T: rows become columns.
  const out = [r[0], r[3], r[6], 0, r[1], r[4], r[7], 0, r[2], r[5], r[8], 0, 0, 0, 0, 1];
  for (let i = 0; i < 3; i++) out[12 + i] = -(out[i] * t[0] + out[4 + i] * t[1] + out[8 + i] * t[2]);
  return out;
};
const translation = ([x, y, z]) => [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1];
const round = (m) => m.map((value) => Math.round(value * 1e6) / 1e6 + 0);

/** Bounding box of a part's mesh at its pose (mm). */
function bounds(part, load) {
  const corners = readStl(load(part.mesh));
  const m = part.matrix;
  const min = [Infinity, Infinity, Infinity];
  const max = [-Infinity, -Infinity, -Infinity];
  for (let i = 0; i < corners.length; i += 3) {
    const [x, y, z] = [corners[i], corners[i + 1], corners[i + 2]];
    for (let a = 0; a < 3; a++) {
      const value = m[a] * x + m[4 + a] * y + m[8 + a] * z + m[12 + a];
      min[a] = Math.min(min[a], value);
      max[a] = Math.max(max[a], value);
    }
  }
  return { min, max, centre: min.map((value, a) => (value + max[a]) / 2) };
}

export function mergeMod(stock, mod, config, load) {
  const stockParts = new Map(stock.parts.map((part) => [part.id, part]));
  const modParts = new Map(mod.parts.map((part) => [part.id, part]));
  const problems = [];
  const aligned = [];
  const placed = new Set();
  const floor = Math.min(...stock.parts.map((part) => bounds(part, load).min[2]));

  for (const group of config.align ?? []) {
    const members = group.parts.map((id) => modParts.get(id)).filter(Boolean);
    for (const id of group.parts) if (!modParts.has(id)) problems.push(`${id}: not among the mod's extracted parts`);
    if (!members.length) continue;
    let shift;
    if (group.component) {
      const stockPart = stockParts.get(group.component);
      const modPart = modParts.get(group.component);
      if (!stockPart?.occurrence || !modPart?.occurrence) { problems.push(`${group.component}: no occurrence to share a frame`); continue; }
      shift = multiply(stockPart.occurrence, invertRigid(modPart.occurrence));
    } else if (group.centre && group.floor) {
      const from = bounds(modParts.get(group.centre.part), load);
      const onto = bounds(stockParts.get(group.centre.onto), load);
      const fromFloor = bounds(modParts.get(group.floor.part), load).min[2];
      const ontoFloor = bounds(stockParts.get(group.floor.onto), load).min[2];
      shift = translation([onto.centre[0] - from.centre[0], onto.centre[1] - from.centre[1], ontoFloor - fromFloor]);
    } else if (group.at) {
      const box = bounds(members[0], load);
      shift = translation([group.at[0] - box.centre[0], group.at[1] - box.centre[1], floor - box.min[2]]);
    } else {
      problems.push(`group ${group.parts.join(', ')}: no way to align it`);
      continue;
    }
    for (const part of members) {
      const matrix = round(multiply(shift, part.matrix));
      aligned.push({ ...part, matrix, occurrence: undefined, mod: config.id, color: config.colors?.[part.id] });
      placed.add(part.id);
    }
    if (group.check) {
      const moved = aligned.find((part) => part.id === group.check.part);
      const a = bounds(moved, load);
      const b = bounds(stockParts.get(group.check.onto), load);
      const error = Math.max(Math.abs(a.centre[0] - b.centre[0]), Math.abs(a.centre[1] - b.centre[1]), Math.abs(a.min[2] - b.min[2]));
      if (error > group.check.max_mm) problems.push(`${group.check.part} lands ${error.toFixed(2)} mm off the stock ${group.check.onto}`);
    }
  }
  for (const id of modParts.keys()) if (!placed.has(id)) problems.push(`${id}: in the mod but in no align group`);

  const removed = new Set([...(config.remove ?? []), ...Object.keys(config.replace ?? {}), ...placed]);
  const parts = [...stock.parts.filter((part) => !removed.has(part.id)), ...aligned].map(({ occurrence, ...part }) => part);
  if (problems.length) throw new Error(`mod ${config.id}:\n  ${problems.join('\n  ')}`);
  // A screw or nut through a part the mod removes or replaces goes too: the mod part's holes are its own.
  const hardware = stock.hardware.filter((item) => !item.fastener?.parts.some((id) => removed.has(id)));
  return { ...stock, mod: config.id, parts, hardware };
}

function main([stockPath, modPath, configPath, outPath]) {
  if (!outPath) {
    console.error('usage: merge_mod.mjs <stock assembly.json> <mod assembly.json> <mod config.json> <out assembly.json>');
    return 2;
  }
  const read = (path) => JSON.parse(readFileSync(path, 'utf8'));
  const stockDir = dirname(resolve(stockPath));
  const load = (path) => readFileSync(path.startsWith('3d_models/') || path.startsWith('mods/') ? resolve(ROOT, path) : resolve(stockDir, path));
  const merged = mergeMod(read(stockPath), read(modPath), read(configPath), load);
  // Hardware meshes stay beside the stock manifest.
  merged.hardware = merged.hardware.map((item) => ({ ...item, mesh: resolve(stockDir, item.mesh) }));
  mkdirSync(dirname(resolve(outPath)), { recursive: true });
  writeFileSync(outPath, `${JSON.stringify(merged, null, 1)}\n`);
  console.log(`${merged.parts.length} parts (${merged.parts.filter((part) => part.mod).length} from ${merged.mod}) -> ${outPath}`);
  return 0;
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) process.exitCode = main(process.argv.slice(2));
