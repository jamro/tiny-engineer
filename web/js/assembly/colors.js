// Part colours: defaults as designed, a filament palette, and a compact form
// for share links (#c=Head.b87333~Hat.1d1d1b). No DOM here, so it is testable
// in Node (scripts/assembly/test_viewer.mjs).

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

/**
 * Finishes the M2 screws and nuts come in (scripts/assembly/fasteners.py):
 * black oxide and silver (zinc-plated or stainless steel), each as a
 * metal. Screws and nuts take one each (#f=black.silver: screws, then nuts).
 */
export const FASTENER_FINISHES = {
  silver: { name: 'Silver', about: 'zinc-plated or stainless steel', hex: '#c9ccd1', metalness: 0.75, roughness: 0.32 },
  black: { name: 'Black', about: 'black oxide', hex: '#2b2c30', metalness: 0.55, roughness: 0.42 },
};
export const FASTENER_KINDS = ['screw', 'nut'];
export const DEFAULT_FINISHES = Object.freeze({ screw: 'silver', nut: 'silver' });

/** Screw and nut finishes for a URL hash, or '' for the defaults. */
export function encodeFinishes(finishes) {
  const value = FASTENER_KINDS.map((kind) => (FASTENER_FINISHES[finishes?.[kind]] ? finishes[kind] : DEFAULT_FINISHES[kind]));
  return value.every((finish, i) => finish === DEFAULT_FINISHES[FASTENER_KINDS[i]]) ? '' : value.join('.');
}

/** Screw and nut finishes from encodeFinishes' text; unknown ones fall back to the defaults. */
export function decodeFinishes(text) {
  const values = String(text ?? '').split('.');
  return Object.fromEntries(FASTENER_KINDS.map((kind, i) => [kind, FASTENER_FINISHES[values[i]] ? values[i] : DEFAULT_FINISHES[kind]]));
}

/** Swatches offered in the picker: common PLA / PETG colours. */
export const PALETTE = [
  { name: 'Silk copper', hex: '#c08a3e' },
  { name: 'Silk gold', hex: '#d4af37' },
  { name: 'Black', hex: '#262624' },
  { name: 'Grey', hex: '#8a8c8e' },
  { name: 'White', hex: '#f2efe6' },
  { name: 'Red', hex: '#c8312c' },
  { name: 'Orange', hex: '#ef7d1a' },
  { name: 'Yellow', hex: '#f3c623' },
  { name: 'Green', hex: '#3d9a4a' },
  { name: 'Terminal green', hex: '#7cff6b' },
  { name: 'Blue', hex: '#2a62c9' },
  { name: 'Purple', hex: '#7a4fb3' },
  { name: 'Pink', hex: '#e778a8' },
  { name: 'Brown', hex: '#6e4a2f' },
];

/** Translucent printed parts (light passes through). */
export const TRANSLUCENT = new Set(['LampDiffuser']);

const HEX = /^#?([0-9a-f]{6})$/i;

/** `#abcdef`, lower case, or null. */
export function normalizeHex(value) {
  const match = HEX.exec(String(value ?? '').trim());
  return match ? `#${match[1].toLowerCase()}` : null;
}

/** The colour a part has before anyone changes it. */
export function defaultColor(part) {
  if (normalizeHex(part.color)) return normalizeHex(part.color);
  if (part.kind === 'hardware') return HARDWARE_COLORS[part.component] ?? '#555555';
  return PART_COLORS[part.id] ?? GROUP_COLORS[part.printGroup] ?? GROUP_COLORS.PartsSetB;
}

/** Only the colours that differ from the defaults, encoded for a URL hash. */
export function encodeColors(parts, colors) {
  return parts
    .filter((part) => colors[part.id] && colors[part.id] !== defaultColor(part))
    .map((part) => `${encodeURIComponent(part.id)}.${colors[part.id].slice(1)}`)
    .join('~');
}

/** Colours from an encoded string; unknown parts and bad values are dropped. */
export function decodeColors(text, parts) {
  const known = new Set(parts.map((part) => part.id));
  const colors = {};
  for (const item of String(text ?? '').split('~')) {
    const dot = item.lastIndexOf('.');
    if (dot < 1) continue;
    let id;
    try { id = decodeURIComponent(item.slice(0, dot)); } catch { continue; }
    const hex = normalizeHex(item.slice(dot + 1));
    if (known.has(id) && hex) colors[id] = hex;
  }
  return colors;
}

/** Every part's colour: defaults overridden by `changes`. */
export function resolveColors(parts, changes) {
  return Object.fromEntries(parts.map((part) => [part.id, changes[part.id] ?? defaultColor(part)]));
}
