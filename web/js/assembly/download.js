// Print files for the robot the viewer shows, as one ZIP: which files, where
// they are on the site (web/print/, staged by scripts/stage_print_files.mjs on
// deploy), where they go in the ZIP, and a README with quantities and the
// colours picked in the viewer. No DOM here, so it is testable in Node
// (scripts/assembly/test_viewer.mjs); main.js fetches and zips.

/** The site folder with the staged print files, relative to web/assembly/. */
export const PRINT_ROOT = '../print/';
/** Small testers to print before the robot (3d_models/README.md#print-first). */
export const PRINT_FIRST = ['ServoSizingTester', 'ScrewSizingTest'];
/** What a print service gets: the two colour sets and the parts printed on their own (docs/3d/order-parts.md). */
export const SERVICE_SETS = ['PartsSetA', 'PartsSetB'];
export const TARGETS = {
  home: { label: 'Home printer', format: '3mf', about: 'one 3MF per part, testers first' },
  service: { label: 'Print service', format: 'stl', about: 'the colour sets as STL (docs/3d/order-parts.md)' },
};

/** The print file a part comes from: its component, the mirrored upper arm being a second UpperArm. */
export const printName = (part) => String(part.component ?? part.id).replace(/\(Mirror\)$/, '');

/**
 * The files for the model's printed parts (`parts`: id, component, kind,
 * printGroup, mod): { files: [{ url, path, name, count, parts }], problems }.
 * `url` is relative to PRINT_ROOT, `path` the file's place in the ZIP.
 * `home` takes every part's own file (a mod part from the mod's folder) and
 * the testers; `service` takes the colour sets and the parts outside them,
 * for the stock robot only (a mod's parts are not in the sets).
 */
export function printList({ parts, preset, mod = null, target = 'home' }) {
  const format = TARGETS[target]?.format;
  if (!format) return { files: [], problems: [`unknown target ${target}`] };
  const printed = parts.filter((part) => part.kind === 'printed');
  const folder = (part) => (part?.mod ? `mods/${part.mod}/${preset}` : preset);
  const files = new Map();
  const add = (name, part, path) => {
    const url = `${folder(part)}/${format}/${name}.${format}`;
    const entry = files.get(url) ?? { url, path, name, count: 0, parts: [] };
    if (part) { entry.count += 1; entry.parts.push(part.id); }
    files.set(url, entry);
  };
  if (target === 'service') {
    if (mod || printed.some((part) => part.mod)) return { files: [], problems: ['a mod is printed from its own parts: use the home printer files'] };
    for (const set of SERVICE_SETS) {
      const members = printed.filter((part) => part.printGroup === set);
      if (!members.length) continue;
      add(set, null, `${set}.${format}`);
      const entry = files.get(`${preset}/${format}/${set}.${format}`);
      entry.count = 1;
      entry.parts = members.map((part) => part.id);
    }
    for (const part of printed.filter((p) => !SERVICE_SETS.includes(p.printGroup))) add(printName(part), part, `${printName(part)}.${format}`);
  } else {
    for (const name of PRINT_FIRST) { add(name, null, `print-first/${name}.${format}`); files.get(`${preset}/${format}/${name}.${format}`).count = 1; }
    for (const part of printed) add(printName(part), part, `parts/${printName(part)}.${format}`);
  }
  return { files: [...files.values()], problems: [] };
}

/** The ZIP's name: tiny-engineer-sg90-home.zip, tiny-engineer-sg90-halloween-home.zip. */
export const zipName = ({ preset, mod = null, target = 'home' }) => `tiny-engineer-${preset}${mod ? `-${mod}` : ''}-${target}.zip`;

/**
 * README.txt for the ZIP: what is in it, how many of each, in which colour
 * (`colors`: part id → hex, `colorName(hex)` a palette name or null).
 */
export function readme({ files, preset, presetLabel = preset, mod = null, target = 'home', colors = {}, colorName = () => null, link = '' }) {
  const colour = (ids) => {
    const hexes = [...new Set(ids.map((id) => colors[id]).filter(Boolean))];
    return hexes.map((hex) => (colorName(hex) ? `${colorName(hex)} (${hex})` : hex)).join(', ');
  };
  const lines = [
    `Tiny Engineer: print files, ${presetLabel} servos${mod ? `, ${mod} mod` : ''}`,
    `${TARGETS[target].label}: ${TARGETS[target].about}.`,
    '',
  ];
  if (link) lines.push(`Colours as chosen in the 3D viewer: ${link}`, '');
  const width = Math.max(...files.map((file) => file.path.length));
  for (const file of files) {
    const what = file.parts.length ? colour(file.parts) : '';
    lines.push(`${file.path.padEnd(width)}  x${file.count}${what ? `  ${what}` : ''}`);
  }
  lines.push(
    '',
    target === 'home'
      ? 'Print the two testers in print-first/ before the rest: https://github.com/jamro/tiny-engineer/blob/main/3d_models/README.md#print-first'
      : 'Upload these files to the print service: https://github.com/jamro/tiny-engineer/blob/main/docs/3d/order-parts.md',
    'LampDiffuser has intentionally thin walls: https://github.com/jamro/tiny-engineer/blob/main/3d_models/README.md#lampdiffuser-thin-walls',
    'Screws and assembly: https://github.com/jamro/tiny-engineer/blob/main/docs/3d/assembly.md',
    '',
    'The parts are licensed under CERN-OHL-S-2.0 (LICENSE, NOTICE). The Tiny Engineer name and logo are not licensed.',
    '',
  );
  return lines.join('\n');
}
