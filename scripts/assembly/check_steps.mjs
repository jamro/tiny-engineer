#!/usr/bin/env node
// Check the viewer's assembly steps against a model's parts: every part is
// added exactly once, and every id the steps name exists.
//   node scripts/assembly/check_steps.mjs artifacts/assembly/sg90/assembly.json web/assembly/steps.json [web/assembly/mods/<id>.json]
// With a mod's config, the steps are checked as the mod changes them.
import { readFileSync } from 'node:fs';
import { applyMod, validate } from '../../web/js/assembly/steps.js';

const [manifestPath, stepsPath, modPath] = process.argv.slice(2);
if (!manifestPath || !stepsPath) {
  console.error('usage: check_steps.mjs <assembly.json> <steps.json>');
  process.exit(2);
}
const manifest = JSON.parse(readFileSync(manifestPath, 'utf8'));
const ids = [...manifest.parts, ...manifest.hardware].map((entry) => entry.id);
let steps = JSON.parse(readFileSync(stepsPath, 'utf8'));
if (modPath) steps = applyMod(steps, JSON.parse(readFileSync(modPath, 'utf8')));
const problems = validate(steps, ids);
if (problems.length) {
  console.error(`${stepsPath}:\n  ${problems.join('\n  ')}`);
  process.exit(1);
}
console.log(`${stepsPath}${modPath ? ` + ${modPath}` : ''}: every one of ${ids.length} parts is added once`);
