#!/usr/bin/env node
// Check the assembly viewer's build steps for parts clipping through each other.
//
//   node scripts/assembly/check_clipping.mjs artifacts/assembly/sg90/assembly.json web/assembly/steps.json
//
// For each step it walks the moving parts along their paths (from where they
// wait to where the step leaves them, as the viewer draws them: offsetsAt in
// web/js/assembly/steps.js) and measures how deep they go into every part
// shown. It also checks the assembled robot, where mating faces touch but no
// part should sit inside another. Penetration is measured on the meshes
// (scripts/assembly/clipping.mjs), not on bounding boxes.
//
// Options: --tolerance <mm> (default 0.3), --step <mm> path sampling (default 1).
// Exit status 1 when anything clips, so CI fails.
import { readFileSync } from 'node:fs';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { meshPath, readStl } from './build_glb.mjs';
import { makeSolid, penetration, boxesOverlap } from './clipping.mjs';
import { flatten, offsetsAt, phases, sceneAt, withFasteners } from '../../web/js/assembly/steps.js';

/** Stock steps.json plus screws and nuts from the manifest. */
export function loadSteps(stepsPath, manifest) {
  return withFasteners(JSON.parse(readFileSync(stepsPath, 'utf8')), manifest.hardware);
}

/** The solids of a manifest, at their assembled poses (CAD frame, mm). */
export function loadSolids(manifestPath) {
  const manifest = JSON.parse(readFileSync(manifestPath, 'utf8'));
  const here = dirname(resolve(manifestPath));
  const solids = new Map();
  const cache = new Map();
  for (const entry of [...manifest.parts, ...manifest.hardware]) {
    if (!cache.has(entry.mesh)) cache.set(entry.mesh, readStl(readFileSync(meshPath(here, entry.mesh))));
    const corners = cache.get(entry.mesh);
    const m = entry.matrix;
    const out = new Float32Array(corners.length);
    for (let i = 0; i < corners.length; i += 3) {
      const x = corners[i], y = corners[i + 1], z = corners[i + 2];
      out[i] = m[0] * x + m[4] * y + m[8] * z + m[12];
      out[i + 1] = m[1] * x + m[5] * y + m[9] * z + m[13];
      out[i + 2] = m[2] * x + m[6] * y + m[10] * z + m[14];
    }
    solids.set(entry.id, makeSolid(entry.id, out));
  }
  return solids;
}

const ZERO = [0, 0, 0];
const sub = (a, b) => a.map((v, k) => v - b[k]);
const length = (v) => Math.hypot(...v);
const fmt = (mm) => `${mm.toFixed(2)} mm`;

/**
 * Problems with the steps for these solids, and the overlaps that `fits`
 * (press fits, everywhere) or a step's `allow` (that step only) let through:
 * { problems, allowed }, each entry { kind, step, title, a, b, depth, at }.
 * `kind` is 'assembled' (parts inside each other in the finished robot) or
 * 'path' (a part clips while it moves).
 */
export function checkSteps(solids, data, { tolerance = 0.3, sampleMm = 1 } = {}) {
  const steps = flatten(data);
  const ids = [...solids.keys()];
  const problems = [];
  const allowed = [];
  const pairKey = (a, b) => (a < b ? `${a}|${b}` : `${b}|${a}`);
  // Declared press fits (everywhere) and a step's own allowances (in that step).
  const fits = new Map((data.fits ?? []).map((fit) => [pairKey(...fit.parts), fit]));
  const limit = (a, b, step) => {
    const own = step?.allow?.find((entry) => pairKey(...entry.parts) === pairKey(a, b));
    return own ?? fits.get(pairKey(a, b)) ?? null;
  };
  const judge = (problem, step) => {
    const allowance = limit(problem.a, problem.b, step);
    if (allowance && problem.depth <= allowance.mm) allowed.push({ ...problem, allowance });
    else problems.push(problem);
  };
  const where = (index) => `${index + 1} (${steps[index].section.id} ${steps[index].index + 1})`;

  // The finished robot: touching is fine, inside is not.
  for (let i = 0; i < ids.length; i++) {
    for (let j = i + 1; j < ids.length; j++) {
      const a = solids.get(ids[i]);
      const b = solids.get(ids[j]);
      if (!boxesOverlap(a.box, b.box)) continue;
      const depth = penetration(a, b, ZERO, Infinity, tolerance);
      if (depth > tolerance) judge({ kind: 'assembled', a: a.id, b: b.id, depth });
    }
  }

  const offsetOf = (offsets, id) => offsets.get(id) ?? ZERO;
  for (let s = 0; s < steps.length; s++) {
    const scene = sceneAt(steps, s);
    if (!scene.moving.size) continue;
    // Every part moves in a straight line within each phase of the step (the
    // parts, then its screws).
    const breaks = phases(steps[s]);
    const at = breaks.map((t) => offsetsAt(steps, s, t));
    // The pairs that move relative to each other in this step.
    const pairs = [];
    for (const m of scene.moving) {
      for (const other of scene.shown) {
        if (other === m || (scene.moving.has(other) && other < m)) continue;
        const path = at.map((offsets) => sub(offsetOf(offsets, other), offsetOf(offsets, m)));
        if (path.every((p) => length(sub(p, path[0])) < 1e-9) && !steps[s].add.includes(m) && !steps[s].add.includes(other)) continue;
        pairs.push([m, other, path]);
      }
    }
    for (const [m, other, path] of pairs) {
      const a = solids.get(m);
      const b = solids.get(other);
      if (!a || !b) continue;
      let worst = { depth: 0, at: 0 };
      for (let leg = 0; leg + 1 < path.length; leg++) {
        const [start, end] = [path[leg], path[leg + 1]];
        // Samples about `sampleMm` apart along the pair's relative path.
        const samples = Math.max(1, Math.ceil(length(sub(start, end)) / sampleMm));
        for (let k = leg ? 1 : 0; k <= samples; k++) {
          const t = k / samples;
          const shift = start.map((v, axis) => v + (end[axis] - v) * t);
          // At their assembled relative pose the parts are checked as assembled.
          if (length(shift) < 1e-9) continue;
          const depth = penetration(a, b, shift, 50, tolerance);
          if (depth > worst.depth) worst = { depth, at: breaks[leg] + (breaks[leg + 1] - breaks[leg]) * t };
        }
      }
      if (worst.depth > tolerance) judge({ kind: 'path', step: where(s), title: steps[s].title, a: m, b: other, ...worst }, steps[s]);
    }
  }
  return { problems, allowed };
}

export function describe(problem) {
  if (problem.kind === 'assembled') return `assembled: ${problem.a} × ${problem.b} overlap ${fmt(problem.depth)}`;
  return `step ${problem.step} "${problem.title}": ${problem.a} × ${problem.b} clips ${fmt(problem.depth)} at ${Math.round(problem.at * 100)} % of the move`;
}

function main(argv) {
  const options = { tolerance: 0.3, sampleMm: 1 };
  const args = [];
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === '--tolerance') options.tolerance = Number(argv[++i]);
    else if (argv[i] === '--step') options.sampleMm = Number(argv[++i]);
    else args.push(argv[i]);
  }
  const [manifestPath, stepsPath] = args;
  if (!manifestPath || !stepsPath || !(options.tolerance >= 0) || !(options.sampleMm > 0)) {
    console.error('usage: check_clipping.mjs <assembly.json> <steps.json> [--tolerance mm] [--step mm]');
    return 2;
  }
  const data = loadSteps(stepsPath, JSON.parse(readFileSync(manifestPath, 'utf8')));
  const solids = loadSolids(manifestPath);
  const { problems, allowed } = checkSteps(solids, data, options);
  const count = flatten(data).length;
  for (const entry of allowed) console.log(`  allowed, ≤ ${fmt(entry.allowance.mm)}: ${describe(entry)} (${entry.allowance.why})`);
  if (problems.length) {
    console.error(`${stepsPath}: ${problems.length} clipping problem${problems.length === 1 ? '' : 's'} (tolerance ${fmt(options.tolerance)}):`);
    for (const problem of problems) console.error(`  ${describe(problem)}`);
    return 1;
  }
  console.log(`${stepsPath}: ${count} steps, ${solids.size} parts, nothing clips (tolerance ${fmt(options.tolerance)})`);
  return 0;
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) process.exitCode = main(process.argv.slice(2));
