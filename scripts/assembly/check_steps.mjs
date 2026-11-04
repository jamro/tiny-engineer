#!/usr/bin/env node
// Check the viewer's assembly steps against a model's parts: every part is
// added exactly once, and every id the steps name exists.
//   node scripts/assembly/check_steps.mjs artifacts/assembly/sg90/assembly.json web/assembly/steps.json [web/assembly/mods/<id>.json]
// With a mod's config, the steps are checked as the mod changes them.
import { readFileSync } from 'node:fs';
import { validate } from '../../web/js/assembly/steps.js';
import { loadSteps } from './check_clipping.mjs';

const [manifestPath, stepsPath, modPath] = process.argv.slice(2);
if (!manifestPath || !stepsPath) {
  console.error('usage: check_steps.mjs <assembly.json> <steps.json>');
  process.exit(2);
}
const manifest = JSON.parse(readFileSync(manifestPath, 'utf8'));
const ids = [...manifest.parts, ...manifest.hardware].map((entry) => entry.id);
const problems = validate(loadSteps(stepsPath, modPath, manifest), ids);
if (problems.length) {
  console.error(`${stepsPath}:\n  ${problems.join('\n  ')}`);
  process.exit(1);
}
const fasteners = manifest.hardware.filter((entry) => entry.fastener).length;
console.log(`${stepsPath}${modPath ? ` + ${modPath}` : ''}: every one of ${ids.length} parts is added once${fasteners ? ` (${fasteners} screws and nuts)` : ''}`);
