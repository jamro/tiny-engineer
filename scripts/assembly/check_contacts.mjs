#!/usr/bin/env node
// Check that a servo preset's robot still holds together: every two parts that
// touch in the robot as designed (the saved preset, from extract.py) still touch
// at the preset that presets.py solved. The clipping check catches parts inside
// each other; this one catches parts that came apart (a mug floating over the desk).
//
//   node scripts/assembly/check_contacts.mjs artifacts/assembly/sg90/assembly.json artifacts/assembly/fs0307/assembly.json
//
// Options: --touch <mm> (default 0.05: what counts as touching in the design),
// --tolerance <mm> (default 0.1: the largest gap allowed at the preset).
// Exit status 1 when a contact opened.
import { resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { loadSolids } from './check_clipping.mjs';
import { gap, penetration } from './clipping.mjs';

/** Pairs that touch in `base` and are more than `tolerance` apart in `preset`: [{a, b, gap}]. */
export function openedContacts(base, preset, { touch = 0.05, tolerance = 0.1 } = {}) {
  const ids = [...base.keys()].filter((id) => preset.has(id));
  const opened = [];
  for (let i = 0; i < ids.length; i++) {
    for (let j = i + 1; j < ids.length; j++) {
      const a = ids[i], b = ids[j];
      if (gap(base.get(a), base.get(b), touch * 2) > touch) continue;
      const now = gap(preset.get(a), preset.get(b), 5);
      // Parts that cross each other at an angle (a press fit) touch, whatever the samples say.
      if (now > tolerance && !(penetration(preset.get(a), preset.get(b), [0, 0, 0], 0.01) > 0)) opened.push({ a, b, gap: now });
    }
  }
  return opened;
}

function main(argv) {
  const options = { touch: 0.05, tolerance: 0.1 };
  const args = [];
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === '--touch') options.touch = Number(argv[++i]);
    else if (argv[i] === '--tolerance') options.tolerance = Number(argv[++i]);
    else args.push(argv[i]);
  }
  const [basePath, presetPath] = args;
  if (!basePath || !presetPath) {
    console.error('usage: check_contacts.mjs <design assembly.json> <preset assembly.json> [--touch mm] [--tolerance mm]');
    return 2;
  }
  const opened = openedContacts(loadSolids(basePath), loadSolids(presetPath), options);
  if (opened.length) {
    console.error(`${presetPath}: ${opened.length} contact${opened.length === 1 ? '' : 's'} of ${basePath} opened (more than ${options.tolerance} mm apart):`);
    for (const { a, b, gap: g } of opened) console.error(`  ${a} × ${b}: ${g >= 5 ? '≥ 5' : g.toFixed(2)} mm apart`);
    return 1;
  }
  console.log(`${presetPath}: every contact of ${basePath} still touches (within ${options.tolerance} mm)`);
  return 0;
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) process.exitCode = main(process.argv.slice(2));
