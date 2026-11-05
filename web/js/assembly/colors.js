// Part colours as designed. No picker or share encoding — defaults only.
// Tested in Node (scripts/assembly/test_viewer.mjs).

/** Colour as designed, by the aggregate the part is printed in. */
export const GROUP_COLORS = {
  PartsSetA: '#c08a3e', // silk copper
  PartsSetB: '#262624', // black
};

/** Parts printed on their own, outside the two aggregates. */
export const PART_COLORS = {
  LampDiffuser: '#f2efe6', // white or translucent
};

/** Hardware, as bought. */
export const HARDWARE_COLORS = {
  Servo: '#2f55b4',
  OLED: '#1b1d22',
  // Drawn in their own colours (vertex colours); white leaves them as they are.
  MainControlBoard: '#ffffff',
  'ESP32-C3-Zero': '#ffffff',
  PinHeader: '#ffffff',
};

/** Default finish for M2 screws and nuts (silver metal look). */
export const FASTENER_FINISH = {
  name: 'Silver',
  hex: '#c9ccd1',
  metalness: 0.75,
  roughness: 0.32,
};

/** Translucent printed parts (light passes through). */
export const TRANSLUCENT = new Set(['LampDiffuser']);

const HEX = /^#?([0-9a-f]{6})$/i;

/** `#abcdef`, lower case, or null. */
export function normalizeHex(value) {
  const match = HEX.exec(String(value ?? '').trim());
  return match ? `#${match[1].toLowerCase()}` : null;
}

/** The colour a part has as designed. */
export function defaultColor(part) {
  if (normalizeHex(part.color)) return normalizeHex(part.color);
  if (part.kind === 'hardware') return HARDWARE_COLORS[part.component] ?? '#555555';
  return PART_COLORS[part.id] ?? GROUP_COLORS[part.printGroup] ?? GROUP_COLORS.PartsSetB;
}

/** Every part's default colour. */
export function resolveColors(parts) {
  return Object.fromEntries(parts.map((part) => [part.id, defaultColor(part)]));
}
