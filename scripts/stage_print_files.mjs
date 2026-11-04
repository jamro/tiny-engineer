#!/usr/bin/env node
// Copy the printable parts into the GitHub Pages site, for the assembly
// viewer's download button (web/js/assembly/download.js):
//
//   3d_models/parts/<preset>/{3mf,stl}/*                -> web/print/<preset>/{3mf,stl}/
//   mods/<mod>/3d_models/parts/<preset>/{3mf,stl}/*     -> web/print/mods/<mod>/<preset>/{3mf,stl}/
//   3d_models/LICENSE, 3d_models/NOTICE                 -> web/print/
//
//   node scripts/stage_print_files.mjs [out-dir]   # default web/print
// Node 18+, no dependencies.
import { copyFileSync, existsSync, mkdirSync, readdirSync, rmSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const FORMATS = ['3mf', 'stl'];

/** [source, destination] pairs, relative to the repository and to the output folder. */
export function printFiles(root = ROOT) {
  const pairs = [];
  const dirs = (path) => (existsSync(path) ? readdirSync(path, { withFileTypes: true }).filter((d) => d.isDirectory()).map((d) => d.name).sort() : []);
  const parts = (from, to) => {
    for (const preset of dirs(join(root, from))) {
      for (const format of FORMATS) {
        const dir = join(from, preset, format);
        if (!existsSync(join(root, dir))) continue;
        for (const file of readdirSync(join(root, dir)).sort()) {
          if (file.toLowerCase().endsWith(`.${format}`)) pairs.push([join(dir, file), join(to, preset, format, file)]);
        }
      }
    }
  };
  parts('3d_models/parts', '');
  for (const mod of dirs(join(root, 'mods'))) parts(join('mods', mod, '3d_models/parts'), join('mods', mod));
  for (const file of ['LICENSE', 'NOTICE']) pairs.push([join('3d_models', file), file]);
  return pairs;
}

function main([out = 'web/print']) {
  const target = resolve(ROOT, out);
  rmSync(target, { recursive: true, force: true });
  const pairs = printFiles();
  for (const [from, to] of pairs) {
    mkdirSync(dirname(join(target, to)), { recursive: true });
    copyFileSync(join(ROOT, from), join(target, to));
  }
  console.log(`${pairs.length} print files -> ${out}`);
  return 0;
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) process.exitCode = main(process.argv.slice(2));
