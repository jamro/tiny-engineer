// Export a development board from canfactory's parts library as a coloured
// binary STL for the assembly viewer (scripts/assembly/hardware/).
//
// canfactory (https://github.com/nntin/canfactory) draws each board from its
// manufacturer's drawing: outline, castellated pins, USB-C receptacle,
// buttons, LED, chip, antenna (packages/contracts/src/parts/dev-boards.ts,
// built by packages/part-geometry). Run it with canfactory's own tsx and
// dependencies (npm ci in a canfactory checkout):
//
//   <canfactory>/node_modules/.bin/tsx scripts/assembly/canfactory_board.ts <canfactory> waveshare-esp32-c3-zero scripts/assembly/hardware/ESP32-C3-Zero.stl
//
// The board lies component side up, as canfactory lays it out: PCB corner at
// the origin, pins in two columns along +Y, USB-C toward +Y, PCB from z = 0 to
// its thickness, millimetres. Each triangle's colour is in its attribute
// bytes, VisCAM style: bit 15 set, then 5 bits each of red, green, blue (sRGB).
import { writeFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { pathToFileURL } from 'node:url';

const [checkout, id, out] = process.argv.slice(2);
if (!checkout || !id || !out) {
  console.error('usage: tsx canfactory_board.ts <canfactory checkout> <part id> <out.stl>');
  process.exit(2);
}
const from = (path: string) => import(pathToFileURL(resolve(checkout, path)).href);
const { parts } = await from('packages/contracts/src/parts/index.ts');
const { partGeometry } = await from('packages/part-geometry/src/index.ts');
const part = parts.find((candidate: { id: string }) => candidate.id === id);
if (!part) throw new Error(`no part ${id} in ${checkout}`);
const geometry = partGeometry(part);
if (!geometry) throw new Error(`${id} has no procedural geometry`);
const flat = geometry.index ? geometry.toNonIndexed() : geometry;
const position = flat.getAttribute('position');
const color = flat.getAttribute('color');
const count = position.count / 3;
const bytes = Buffer.alloc(84 + count * 50);
bytes.write(`canfactory ${id}`.slice(0, 80), 0, 'ascii');
bytes.writeUInt32LE(count, 80);
// three.js keeps vertex colours linear; the STL stores sRGB.
const srgb = (linear: number) => (linear <= 0.0031308 ? 12.92 * linear : 1.055 * linear ** (1 / 2.4) - 0.055);
const five = (linear: number) => Math.max(0, Math.min(31, Math.round(srgb(linear) * 31)));
for (let t = 0; t < count; t++) {
  const base = 84 + t * 50;
  const corner = (c: number, k: number) => position.getComponent(t * 3 + c, k);
  const u = [0, 1, 2].map((k) => corner(1, k) - corner(0, k));
  const v = [0, 1, 2].map((k) => corner(2, k) - corner(0, k));
  const n = [u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]];
  const length = Math.hypot(...n) || 1;
  for (let k = 0; k < 3; k++) bytes.writeFloatLE(n[k] / length, base + k * 4);
  for (let c = 0; c < 3; c++) for (let k = 0; k < 3; k++) bytes.writeFloatLE(corner(c, k), base + 12 + (c * 3 + k) * 4);
  const rgb = color ? [color.getX(t * 3), color.getY(t * 3), color.getZ(t * 3)] : [0.5, 0.5, 0.5];
  bytes.writeUInt16LE(0x8000 | (five(rgb[0]) << 10) | (five(rgb[1]) << 5) | five(rgb[2]), base + 48);
}
writeFileSync(out, bytes);
const box = flat.boundingBox ?? (flat.computeBoundingBox(), flat.boundingBox);
console.log(`${id}: ${count} triangles, ${box.min.toArray().map((x: number) => x.toFixed(2)).join(' ')} .. ${box.max.toArray().map((x: number) => x.toFixed(2)).join(' ')} -> ${out}`);
