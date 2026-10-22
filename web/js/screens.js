// Laptop screen insert generator. Sizes and stock designs come from
// presets.json / designs.json, written by scripts/laptop_screens.py from the
// exported CAD meshes. The PDF is assembled here: one JPEG per insert, vector
// cut lines and scale check, no library.

const PX_PER_MM = 24; // ~610 dpi in the PDF
const TEXT_PAD = 0.6; // mm inside the window, plus half the pocket slack
const LINE_H = 1.2;
const PAPER_DIM = '#737373';
// Luminance ceilings for tinted text, same as scripts/laptop_screens.py.
const TINT_TEXT_LUM = 0.08;
const TINT_DIM_LUM = 0.2;
const MIN_CONTRAST = 3;
const A4 = [210, 297];
const MARGIN = 12;
const GAP_X = 4;
const LABEL_H = 4;
const MARKUP = /\{([a-z][a-z0-9_-]*|\/)\}/g;

const $ = (id) => document.getElementById(id);
const state = {
  presets: [], designs: [], preset: null, source: 'text', design: null,
  image: null, zoom: 1, panX: 0.5, panY: 0.5, print: 'colour', paperColour: '#fff1a8', mask: true,
};

// ------------------------------------------------------------ .screen format

function parseScreen(source) {
  const meta = { title: 'Custom', background: '#000000', foreground: '#ffffff', align: 'top-left', outline: 'none', 'paper-dim': '' };
  const palette = {};
  const lines = source.replace(/\r/g, '').split('\n');
  while (lines.length && lines[0].startsWith('#')) {
    const raw = lines.shift().slice(1);
    const i = raw.indexOf(':');
    const key = raw.slice(0, i).trim();
    const value = raw.slice(i + 1).trim();
    if (key.startsWith('color ')) palette[key.slice(6).trim()] = value;
    else if (key in meta) meta[key] = value;
  }
  while (lines.length && !lines[0].trim()) lines.shift();
  while (lines.length && !lines[lines.length - 1].trim()) lines.pop();
  return {
    title: meta.title, background: meta.background, foreground: meta.foreground,
    align: meta.align, outline: meta.outline, palette,
    paperDim: meta['paper-dim'].split(/\s+/).filter(Boolean), body: lines.join('\n'),
  };
}

// Colour tags used in the text, in order of first use.
function usedTags(body) {
  return [...new Set([...body.matchAll(MARKUP)].map((m) => m[1]).filter((n) => n !== '/'))];
}

// Starting colours for a tag the user just typed: the first one not already in
// the design that still reads on its background.
const TAG_DEFAULTS = ['#ffb000', '#5fd7ff', '#ff5fd2', '#9dff5f', '#ff6b6b', '#c792ea',
  '#b7410e', '#2b6cb0', '#2f855a', '#805ad5', '#9c4221', '#1a202c'];

function ensureTags(d) {
  for (const name of usedTags(d.body)) {
    if (name in d.palette) continue;
    const taken = new Set([d.foreground, ...Object.values(d.palette)]);
    d.palette[name] = TAG_DEFAULTS.find((c) => !taken.has(c) && contrast(c, d.background) >= MIN_CONTRAST)
      ?? d.foreground;
  }
}

function toScreenSource(d) {
  const head = [`# title: ${d.title}`, `# background: ${d.background}`, `# foreground: ${d.foreground}`];
  const used = usedTags(d.body);
  // A paper-dim name without its colour line fails parse_screen, so drop unused ones.
  const dim = d.paperDim.filter((name) => used.includes(name));
  if (dim.length) head.push(`# paper-dim: ${dim.join(' ')}`);
  for (const name of used) head.push(`# color ${name}: ${d.palette[name]}`);
  if (d.align !== 'top-left') head.push(`# align: ${d.align}`);
  if (d.outline !== 'none') head.push(`# outline: ${d.outline}`);
  return `${head.join('\n')}\n${d.body}\n`;
}

function runs(line, d) {
  const out = [];
  let colour = d.foreground;
  let pos = 0;
  for (const m of line.matchAll(MARKUP)) {
    if (m.index > pos) out.push([line.slice(pos, m.index), colour]);
    colour = m[1] === '/' ? d.foreground : (d.palette[m[1]] ?? d.foreground);
    pos = m.index + m[0].length;
  }
  if (pos < line.length) out.push([line.slice(pos), colour]);
  return out;
}

// ------------------------------------------------------------ colour

function luminance(hex) {
  const c = [1, 3, 5].map((i) => parseInt(hex.slice(i, i + 2), 16) / 255)
    .map((v) => (v <= 0.04045 ? v / 12.92 : ((v + 0.055) / 1.055) ** 2.4));
  return 0.2126 * c[0] + 0.7152 * c[1] + 0.0722 * c[2];
}

function contrast(a, b) {
  const [hi, lo] = [luminance(a), luminance(b)].sort((x, y) => y - x);
  return (hi + 0.05) / (lo + 0.05);
}

// Keep the hue, darken until it reads on pale coloured paper. Scaling linear
// RGB keeps the chromaticity, so green stays green.
function tintedInk(hex, dim) {
  const target = dim ? TINT_DIM_LUM : TINT_TEXT_LUM;
  const lum = luminance(hex);
  if (lum <= target) return hex;
  const k = target / lum;
  return '#' + [1, 3, 5].map((i) => {
    const v = parseInt(hex.slice(i, i + 2), 16) / 255;
    const lin = (v <= 0.04045 ? v / 12.92 : ((v + 0.055) / 1.055) ** 2.4) * k;
    const s = lin <= 0.0031308 ? 12.92 * lin : 1.055 * lin ** (1 / 2.4) - 0.055;
    return Math.round(s * 255).toString(16).padStart(2, '0');
  }).join('');
}

function inkFor(colour, dim) {
  if (state.print === 'paper') return dim ? PAPER_DIM : '#000000';
  if (state.print === 'tinted') return tintedInk(colour, dim);
  return colour;
}

function lowContrastColours(d) {
  const used = new Set([d.foreground, ...Object.values(d.palette)]);
  return [...used].filter((hex) => contrast(hex, d.background) < MIN_CONTRAST);
}

// ------------------------------------------------------------ drawing

// What a black-only printer does to the page. Done on pixels rather than with
// ctx.filter, which Safari ignores.
function greyscale(ctx, w, h) {
  const img = ctx.getImageData(0, 0, w, h);
  const px = img.data;
  for (let i = 0; i < px.length; i += 4) {
    const y = 0.2126 * px[i] + 0.7152 * px[i + 1] + 0.0722 * px[i + 2];
    px[i] = px[i + 1] = px[i + 2] = y;
  }
  ctx.putImageData(img, 0, 0);
}

function drawInsert(ctx, preset, scale, { mask = false } = {}) {
  const [w, h] = preset.insert;
  const [wx, wy, ww, wh] = preset.window;
  const d = state.design;
  const mm = (v) => v * scale;
  const paper = onPaper();
  ctx.save();
  ctx.fillStyle = paper ? '#ffffff' : d.background;
  ctx.fillRect(0, 0, mm(w), mm(h));

  if (state.source === 'image' && state.image) {
    const bleed = preset.slack / 2;
    const rx = Math.max(0, wx - bleed);
    const ry = Math.max(0, wy - bleed);
    const rw = Math.min(w, wx + ww + bleed) - rx;
    const rh = Math.min(h, wy + wh + bleed) - ry;
    const img = state.image;
    const s = Math.max(mm(rw) / img.naturalWidth, mm(rh) / img.naturalHeight) * state.zoom;
    const dw = img.naturalWidth * s;
    const dh = img.naturalHeight * s;
    ctx.beginPath();
    ctx.rect(mm(rx), mm(ry), mm(rw), mm(rh));
    ctx.clip();
    ctx.drawImage(img, mm(rx) + (mm(rw) - dw) * state.panX, mm(ry) + (mm(rh) - dh) * state.panY, dw, dh);
  } else if (state.source === 'text') {
    const lines = d.body.split('\n').map((l) => runs(l, d));
    const cols = Math.max(1, ...lines.map((r) => r.reduce((n, [t]) => n + t.length, 0)));
    const pad = TEXT_PAD + preset.slack / 2;
    let ax = wx + pad;
    let ay = wy + pad;
    const aw = ww - 2 * pad;
    const ah = wh - 2 * pad;
    ctx.font = '100px "IBM Plex Mono", monospace';
    const charW = ctx.measureText('M').width / 100;
    const size = Math.min(aw / (cols * charW), ah / (Math.max(1, lines.length) * LINE_H));
    if (d.align === 'center') {
      ax += (aw - cols * charW * size) / 2;
      ay += (ah - lines.length * LINE_H * size) / 2;
    }
    // Draw at 100px and scale down: at small sizes the browser rounds glyph
    // advances, which made long lines overrun the window.
    ctx.font = '100px "IBM Plex Mono", monospace';
    ctx.textBaseline = 'alphabetic';
    const k = mm(size) / 100;
    const dim = new Set(d.paperDim.map((n) => d.palette[n]));
    lines.forEach((r, i) => {
      let cx = ax;
      const base = ay + (i + 0.8) * LINE_H * size;
      for (const [text, colour] of r) {
        ctx.fillStyle = inkFor(colour, dim.has(colour));
        ctx.save();
        ctx.translate(mm(cx), mm(base));
        ctx.scale(k, k);
        ctx.fillText(text, 0, 0);
        ctx.restore();
        cx += text.length * charW * size;
      }
    });
  }

  if (d.outline === 'window' && state.source === 'text') {
    ctx.setLineDash([mm(0.8), mm(0.8)]);
    ctx.strokeStyle = '#737373';
    ctx.lineWidth = mm(0.15);
    ctx.strokeRect(mm(wx), mm(wy), mm(ww), mm(wh));
    ctx.setLineDash([]);
  }
  ctx.restore();
  if (state.print === 'mono' || state.print === 'paper') greyscale(ctx, Math.round(mm(w)), Math.round(mm(h)));
  ctx.save();
  if (mask && paper) {
    // Preview only: ink is translucent, so the paper colour multiplies through.
    ctx.globalCompositeOperation = 'multiply';
    ctx.fillStyle = state.paperColour;
    ctx.fillRect(0, 0, mm(w), mm(h));
    ctx.globalCompositeOperation = 'source-over';
  }
  if (mask) {
    ctx.fillStyle = 'rgba(17, 17, 17, 0.6)';
    ctx.beginPath();
    ctx.rect(0, 0, mm(w), mm(h));
    ctx.rect(mm(wx), mm(wy), mm(ww), mm(wh));
    ctx.fill('evenodd');
  }
  ctx.restore();
}

function renderPreview() {
  const preset = state.preset;
  if (!preset || !state.design) return;
  const canvas = $('scr-preview');
  const cssW = canvas.parentElement.clientWidth;
  const scale = (cssW / preset.insert[0]) * (window.devicePixelRatio || 1);
  canvas.width = Math.round(preset.insert[0] * scale);
  canvas.height = Math.round(preset.insert[1] * scale);
  canvas.style.aspectRatio = `${preset.insert[0]} / ${preset.insert[1]}`;
  drawInsert(canvas.getContext('2d', { willReadFrequently: true }), preset, scale, { mask: state.mask });

  const status = $('scr-status');
  const weak = state.source === 'text' && (state.print === 'colour' || state.print === 'mono')
    ? lowContrastColours(state.design) : [];
  if (state.source === 'image' && !state.image) {
    status.textContent = 'No image yet. Pick a file or drop one on the preview.';
    status.className = 'status-message';
  } else if (weak.length) {
    status.textContent = `Low contrast: ${weak.join(', ')} may vanish on a black-only printer.`;
    status.className = 'status-message is-err';
  } else {
    status.textContent = 'Ready. Preview matches the printed insert.';
    status.className = 'status-message is-ok';
  }
  $('scr-download').disabled = state.source === 'image' && !state.image;
  $('scr-copy').disabled = state.source !== 'text';
}

// ------------------------------------------------------------ PDF

const enc = new TextEncoder();
const num = (v) => (Math.round(v * 1000) / 1000).toString();
const pdfStr = (s) => `(${s.replace(/[^\x20-\x7e]/g, '?').replace(/([\\()])/g, '\\$1')})`;

class Ops {
  constructor() { this.ops = [`${num(72 / 25.4)} 0 0 ${num(-72 / 25.4)} 0 ${num(A4[1] * 72 / 25.4)} cm`]; }
  push(s) { this.ops.push(s); }
  rgb(c, stroke) { this.push(`${c.map(num).join(' ')} ${stroke ? 'RG' : 'rg'}`); }
  line(x0, y0, x1, y1, w) { this.rgb([0, 0, 0], true); this.push(`${num(w)} w ${num(x0)} ${num(y0)} m ${num(x1)} ${num(y1)} l S`); }
  rect(x, y, w, h, colour, width) { this.rgb(colour, true); this.push(`${num(width)} w ${num(x)} ${num(y)} ${num(w)} ${num(h)} re S`); }
  text(x, y, s, size, font = 'F1', colour = [0, 0, 0]) {
    this.rgb(colour);
    this.push(`BT /${font} ${num(size)} Tf 1 0 0 -1 ${num(x)} ${num(y)} Tm ${pdfStr(s)} Tj ET`);
  }
  image(x, y, w, h) { this.push(`q ${num(w)} 0 0 ${num(-h)} ${num(x)} ${num(y + h)} cm /Im0 Do Q`); }
}

function sheetOps(preset, title) {
  const [w, h] = preset.insert;
  const [wx, wy, ww, wh] = preset.window;
  const grey = [0.45, 0.45, 0.45];
  const o = new Ops();
  let y = MARGIN + 6;
  o.text(MARGIN, y, 'Tiny Engineer - laptop screen inserts', 6, 'F2');
  y += 7.5;
  o.text(MARGIN, y, `Servo preset: ${preset.name}  (${preset.id})`, 4.6, 'F2');
  y += 6;
  o.text(MARGIN, y, `Insert ${w.toFixed(1)} x ${h.toFixed(1)} mm (cut on the grey line). Visible window ${ww.toFixed(1)} x ${wh.toFixed(1)} mm, ${wx.toFixed(1)} mm from the left and ${wy.toFixed(1)} mm from the top edge.`, 2.8);
  y += 4.2;
  o.text(MARGIN, y, 'Slide the paper insert, then a same-size piece of clear PET, into the lid pocket in front of LaptopScreen.', 2.8);
  y += 4;
  o.rect(MARGIN, y, 10, 10, [0, 0, 0], 0.2);
  o.text(MARGIN + 1.2, y + 5.8, '10 mm', 2.4, 'F1', grey);
  const rx = MARGIN + 16;
  o.line(rx, y + 10, rx + 50, y + 10, 0.2);
  for (let i = 0; i <= 50; i++) {
    const tick = i % 10 === 0 ? 3 : i % 5 === 0 ? 2 : 1.2;
    o.line(rx + i, y + 10, rx + i, y + 10 - tick, 0.12);
    if (i % 10 === 0) o.text(rx + i - 0.8, y + 5.6, String(i), 2.2, 'F1', grey);
  }
  o.text(rx + 53, y + 4.4, 'Square must measure 10 x 10 mm and the ruler 50 mm.', 2.6);
  o.text(rx + 53, y + 8.6, 'If not, reprint at 100% / Actual size (scaling off).', 2.6);
  y += 16;

  const cols = Math.max(1, Math.floor((A4[0] - 2 * MARGIN + GAP_X) / (w + GAP_X)));
  const rows = Math.max(1, Math.floor((A4[1] - MARGIN - y) / (h + LABEL_H + 2)));
  const x0 = (A4[0] - (cols * w + (cols - 1) * GAP_X)) / 2;
  for (let i = 0; i < cols * rows; i++) {
    const ix = x0 + (i % cols) * (w + GAP_X);
    const iy = y + Math.floor(i / cols) * (h + LABEL_H + 2);
    o.image(ix, iy, w, h);
    o.rect(ix, iy, w, h, [0.6, 0.6, 0.6], 0.1);
    o.text(ix, iy + h + 2.8, `${preset.id} - ${title}`, 2.4, 'F1', grey);
  }
  return o.ops.join('\n');
}

async function buildPdf() {
  const preset = state.preset;
  const canvas = document.createElement('canvas');
  canvas.width = Math.round(preset.insert[0] * PX_PER_MM);
  canvas.height = Math.round(preset.insert[1] * PX_PER_MM);
  drawInsert(canvas.getContext('2d'), preset, PX_PER_MM);
  const blob = await new Promise((resolve) => canvas.toBlob(resolve, 'image/jpeg', 0.95));
  const jpeg = new Uint8Array(await blob.arrayBuffer());
  const content = enc.encode(sheetOps(preset, $('scr-title').value.trim() || 'Custom'));
  const page = `${num(A4[0] * 72 / 25.4)} ${num(A4[1] * 72 / 25.4)}`;

  const objs = [
    enc.encode('<< /Type /Catalog /Pages 2 0 R /ViewerPreferences << /PrintScaling /None >> >>'),
    enc.encode('<< /Type /Pages /Kids [3 0 R] /Count 1 >>'),
    enc.encode(`<< /Type /Page /Parent 2 0 R /MediaBox [0 0 ${page}] /Resources << /Font << /F1 5 0 R /F2 6 0 R >> /XObject << /Im0 7 0 R >> >> /Contents 4 0 R >>`),
    [enc.encode(`<< /Length ${content.length} >>\nstream\n`), content, enc.encode('\nendstream')],
    enc.encode('<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>'),
    enc.encode('<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold /Encoding /WinAnsiEncoding >>'),
    [enc.encode(`<< /Type /XObject /Subtype /Image /Width ${canvas.width} /Height ${canvas.height} /ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter /DCTDecode /Length ${jpeg.length} >>\nstream\n`), jpeg, enc.encode('\nendstream')],
  ];
  const parts = [enc.encode('%PDF-1.4\n%\xe2\xe3\xcf\xd3\n')];
  let size = parts[0].length;
  const offsets = [];
  objs.forEach((body, i) => {
    offsets.push(size);
    for (const chunk of [enc.encode(`${i + 1} 0 obj\n`), ...[body].flat(), enc.encode('\nendobj\n')]) {
      parts.push(chunk);
      size += chunk.length;
    }
  });
  const xref = [`xref\n0 ${objs.length + 1}\n0000000000 65535 f \n`, ...offsets.map((o) => `${String(o).padStart(10, '0')} 00000 n \n`)].join('');
  parts.push(enc.encode(`${xref}trailer\n<< /Size ${objs.length + 1} /Root 1 0 R >>\nstartxref\n${size}\n%%EOF\n`));
  return new Blob(parts, { type: 'application/pdf' });
}

// ------------------------------------------------------------ UI

function setDesign(d) {
  state.design = structuredClone(d);
  ensureTags(state.design);
  $('scr-body').value = d.body;
  $('scr-title').value = d.title;
  syncColours();
  renderPreview();
}

const onPaper = () => state.print === 'tinted' || state.print === 'paper';
const mono = () => state.print === 'mono';

// A black-only printer only has greys, so in that mode every swatch shows and
// stores the grey that will print.
function toGrey(hex) {
  const lin = luminance(hex);
  const s = lin <= 0.0031308 ? 12.92 * lin : 1.055 * lin ** (1 / 2.4) - 0.055;
  return `#${Math.round(s * 255).toString(16).padStart(2, '0').repeat(3)}`;
}

// What the swatch shows is what prints: greys on a black-only printer,
// darkened ink on coloured paper.
const shown = (hex, dim = false) => (mono() ? toGrey(hex) : state.print === 'tinted' ? tintedInk(hex, dim) : hex);
const stored = (hex) => (mono() ? toGrey(hex) : hex);

// Swatches follow the print mode: real colours, greys for a black-only
// printer, none at all when everything prints black.
function syncColours() {
  const d = state.design;
  const box = $('scr-colours');
  box.replaceChildren();
  const blackOnly = state.print === 'paper';
  box.hidden = blackOnly;
  $('scr-colours-note').hidden = !blackOnly;
  if (!blackOnly) {
    const swatch = (label, value, dim, onInput) => {
      const id = `scr-colour-${label.replace(/\W/g, '-')}`;
      const wrap = document.createElement('label');
      wrap.className = 'swatch';
      wrap.htmlFor = id;
      const input = Object.assign(document.createElement('input'), { type: 'color', id, value: shown(value, dim) });
      input.addEventListener('input', () => {
        onInput(stored(input.value));
        input.value = shown(input.value, dim);
        renderPreview();
      });
      const name = Object.assign(document.createElement('span'), { className: 'mono', textContent: label });
      wrap.append(input, name);
      box.append(wrap);
    };
    swatch('text', d.foreground, false, (v) => { d.foreground = v; });
    for (const name of usedTags(d.body)) {
      swatch(`{${name}}`, d.palette[name], d.paperDim.includes(name), (v) => { d.palette[name] = v; });
    }
  }

  // One background swatch: the design's background when it is printed, the
  // paper colour (preview only) when the print mode leaves it unprinted.
  const paper = onPaper();
  $('scr-bg-label').textContent = paper ? 'Paper colour · preview only'
    : mono() ? 'Background colour · prints as grey' : 'Background colour';
  $('scr-bg-hint').textContent = paper
    ? 'No background is printed. Set this to your paper to preview the ink on it; ink varies by printer, so treat it as a guide.'
    : mono()
      ? 'Black-only printers print shades of grey, so pick a grey. Light greys save toner.'
      : 'Printed behind your text or image.';
  $('scr-bg').value = paper ? state.paperColour : shown(d.background);
}

function loadImage(file) {
  if (!file || !file.type.startsWith('image/')) return;
  const img = new Image();
  img.onload = () => { state.image = img; renderPreview(); };
  img.src = URL.createObjectURL(file);
}

function setSource(source) {
  state.source = source;
  $('scr-text-fields').hidden = source !== 'text';
  $('scr-image-fields').hidden = source !== 'image';
  $('scr-template-field').hidden = source !== 'text';
  if (source === 'image' && $('scr-title').value === state.design.title) $('scr-title').value = 'Custom';
  renderPreview();
}

function bind() {
  $('scr-preset').addEventListener('change', (e) => {
    state.preset = state.presets.find((p) => p.id === e.target.value);
    const [w, h] = state.preset.insert;
    const [, , ww, wh] = state.preset.window;
    $('scr-insert-size').textContent = `${w.toFixed(1)} × ${h.toFixed(1)} mm`;
    $('scr-window-size').textContent = `${ww.toFixed(1)} × ${wh.toFixed(1)} mm`;
    $('scr-stock-pdf').href = `https://github.com/jamro/tiny-engineer/raw/main/3d_models/laptop_screens/laptop-screens-${state.preset.id}.pdf`;
    renderPreview();
  });
  $('scr-template').addEventListener('change', (e) => setDesign(state.designs[Number(e.target.value)]));
  let tagSet = '';
  $('scr-body').addEventListener('input', (e) => {
    state.design.body = e.target.value;
    const tags = usedTags(state.design.body).join(' ');
    if (tags !== tagSet) {
      tagSet = tags;
      ensureTags(state.design);
      syncColours();
    }
    renderPreview();
  });
  document.querySelectorAll('input[name="scr-source"]').forEach((r) => r.addEventListener('change', () => setSource(r.value)));
  $('scr-image').addEventListener('change', (e) => loadImage(e.target.files[0]));
  const drop = $('scr-drop');
  drop.addEventListener('dragover', (e) => { e.preventDefault(); drop.classList.add('is-over'); });
  drop.addEventListener('dragleave', () => drop.classList.remove('is-over'));
  drop.addEventListener('drop', (e) => {
    e.preventDefault();
    drop.classList.remove('is-over');
    $('scr-source-image').checked = true;
    setSource('image');
    loadImage(e.dataTransfer.files[0]);
  });
  $('scr-zoom').addEventListener('input', (e) => {
    state.zoom = Number(e.target.value) / 100;
    $('scr-zoom-val').textContent = `${e.target.value}%`;
    renderPreview();
  });
  $('scr-pan-x').addEventListener('input', (e) => { state.panX = Number(e.target.value) / 100; renderPreview(); });
  $('scr-pan-y').addEventListener('input', (e) => { state.panY = Number(e.target.value) / 100; renderPreview(); });
  // Printer x background -> print mode.
  const MODES = { 'colour/printed': 'colour', 'mono/printed': 'mono', 'colour/none': 'tinted', 'mono/none': 'paper' };
  document.querySelectorAll('input[name="scr-printer"], input[name="scr-bgmode"]').forEach((r) => r.addEventListener('change', () => {
    const printer = document.querySelector('input[name="scr-printer"]:checked').value;
    const bg = document.querySelector('input[name="scr-bgmode"]:checked').value;
    state.print = MODES[`${printer}/${bg}`];
    syncColours();
    renderPreview();
  }));
  $('scr-bg').addEventListener('input', (e) => {
    if (onPaper()) {
      state.paperColour = e.target.value;
    } else {
      state.design.background = stored(e.target.value);
      e.target.value = shown(e.target.value);
    }
    renderPreview();
  });
  $('scr-mask').addEventListener('change', (e) => { state.mask = e.target.checked; renderPreview(); });
  $('scr-title').addEventListener('input', () => { state.design.title = $('scr-title').value; });
  $('scr-download').addEventListener('click', async () => {
    const button = $('scr-download');
    button.disabled = true;
    try {
      const url = URL.createObjectURL(await buildPdf());
      const a = Object.assign(document.createElement('a'), { href: url, download: `tiny-engineer-screen-${state.preset.id}.pdf` });
      a.click();
      setTimeout(() => URL.revokeObjectURL(url), 10000);
      $('scr-status').textContent = 'PDF downloaded. Print at 100% and check the 10 mm square.';
      $('scr-status').className = 'status-message is-ok';
    } catch (err) {
      $('scr-status').textContent = `Could not build the PDF: ${err.message}`;
      $('scr-status').className = 'status-message is-err';
    } finally {
      button.disabled = false;
    }
  });
  $('scr-copy').addEventListener('click', async () => {
    const text = toScreenSource({ ...state.design, title: $('scr-title').value.trim() || 'Custom' });
    const out = $('scr-source-out');
    try {
      await navigator.clipboard.writeText(text);
      out.hidden = true;
      $('scr-status').textContent = 'Copied. Save it as 3d_models/laptop_screens/screens/<name>.screen to add it to the stock sheet.';
      $('scr-status').className = 'status-message is-ok';
    } catch {
      out.value = text;
      out.hidden = false;
      out.focus();
      out.select();
      $('scr-status').textContent = 'Clipboard blocked. Copy the selected source under the buttons by hand.';
      $('scr-status').className = 'status-message is-err';
    }
  });
  new ResizeObserver(renderPreview).observe($('scr-drop'));
}

async function init() {
  bind();
  try {
    const [presets, designs] = await Promise.all(['presets.json', 'designs.json'].map(async (f) => {
      const res = await fetch(f);
      if (!res.ok) throw new Error(`${f}: HTTP ${res.status}`);
      return res.json();
    }));
    state.presets = presets;
    state.designs = designs.map((d) => parseScreen(d.source));
  } catch (err) {
    $('scr-status').textContent = `Could not load sizes (${err.message}). Reload to try again.`;
    $('scr-status').className = 'status-message is-err';
    return;
  }
  const preset = $('scr-preset');
  preset.replaceChildren(...state.presets.map((p) => new Option(`${p.name} (${p.id})`, p.id)));
  preset.disabled = false;
  $('scr-template').replaceChildren(...state.designs.map((d, i) => new Option(d.title, String(i))));
  await document.fonts.load('10px "IBM Plex Mono"').catch(() => {});
  setDesign(state.designs[0]);
  preset.dispatchEvent(new Event('change'));
}

init();
