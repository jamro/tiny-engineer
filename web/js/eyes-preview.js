/**
 * OLED eye-style gallery previews for /eyes/.
 * Lightweight canvas loops approximating firmware looks (not a C++ port).
 */
import { rasterFace } from "./kaomoji-designs.js";

const W = 128;
const H = 32;
const CORNER = 3;
const DOT_SIZE = 12;
const DOT_MASK = [
  0b000011110000,
  0b001111111100,
  0b011111111110,
  0b011111111110,
  0b111111111111,
  0b111111111111,
  0b111111111111,
  0b111111111111,
  0b011111111110,
  0b011111111110,
  0b001111111100,
  0b000011110000,
];

const DEFAULT_LEFT = { x: 20, y: 9, w: 24, h: 14 };
const DEFAULT_RIGHT = { x: 84, y: 9, w: 24, h: 14 };
const KAOMOJI_FACES = ["idle", "happy", "thinking"];
const KAOMOJI_HOLD_MS = 2800;
const FRAME_MS = 80;

function createBuffer() {
  return new Uint8Array(W * H);
}

function clear(buf) {
  buf.fill(0);
}

function put(buf, x, y, value = 1) {
  x |= 0;
  y |= 0;
  if (x < 0 || y < 0 || x >= W || y >= H) return;
  buf[y * W + x] = value ? 1 : 0;
}

function fillRect(buf, x, y, w, h, value = 1) {
  const x0 = Math.max(0, Math.round(x));
  const y0 = Math.max(0, Math.round(y));
  const x1 = Math.min(W, Math.round(x + w));
  const y1 = Math.min(H, Math.round(y + h));
  for (let row = y0; row < y1; row++) {
    for (let col = x0; col < x1; col++) put(buf, col, row, value);
  }
}

function fillCircle(buf, cx, cy, r, value = 1) {
  const rr = r * r;
  const x0 = Math.floor(cx - r);
  const y0 = Math.floor(cy - r);
  const x1 = Math.ceil(cx + r);
  const y1 = Math.ceil(cy + r);
  for (let y = y0; y <= y1; y++) {
    for (let x = x0; x <= x1; x++) {
      const dx = x - cx;
      const dy = y - cy;
      if (dx * dx + dy * dy <= rr) put(buf, x, y, value);
    }
  }
}

function fillRoundedRect(buf, x, y, w, h, radius) {
  if (w <= 0 || h <= 0) return;
  const r = Math.max(0, Math.min(radius, (w / 2) | 0, (h / 2) | 0));
  if (r <= 0) {
    fillRect(buf, x, y, w, h);
    return;
  }
  fillRect(buf, x + r, y, w - 2 * r, h);
  fillRect(buf, x, y + r, w, h - 2 * r);
  fillCircle(buf, x + r, y + r, r);
  fillCircle(buf, x + w - r - 1, y + r, r);
  fillCircle(buf, x + r, y + h - r - 1, r);
  fillCircle(buf, x + w - r - 1, y + h - r - 1, r);
}

/** Blink open amount + horizontal glance from a looping timeline (ms). */
function poseAt(nowMs, reduceMotion, phaseMs = 0) {
  if (reduceMotion) {
    return {
      left: { ...DEFAULT_LEFT },
      right: { ...DEFAULT_RIGHT },
      open: 1,
    };
  }

  const cycle = (nowMs + phaseMs) % 5200;
  let open = 1;
  if (cycle > 2100 && cycle < 2260) {
    open = 0.12;
  } else if (cycle > 2260 && cycle < 2380) {
    open = 0.12 + ((cycle - 2260) / 120) * 0.88;
  } else if (cycle > 3400 && cycle < 3520) {
    open = 1 - ((cycle - 3400) / 120) * 0.88;
  } else if (cycle > 3520 && cycle < 3640) {
    open = 0.12;
  } else if (cycle > 3640 && cycle < 3760) {
    open = 0.12 + ((cycle - 3640) / 120) * 0.88;
  }

  let glance = 0;
  if (cycle > 600 && cycle < 1400) glance = -5;
  else if (cycle > 4000 && cycle < 4800) glance = 5;

  function eye(base) {
    const h = Math.max(1, Math.round(base.h * open));
    const y = base.y + ((base.h - h) / 2) | 0;
    return { x: base.x + glance, y, w: base.w, h };
  }

  return { left: eye(DEFAULT_LEFT), right: eye(DEFAULT_RIGHT), open };
}

function drawClassic(buf, pose) {
  clear(buf);
  fillRoundedRect(buf, pose.left.x, pose.left.y, pose.left.w, pose.left.h, CORNER);
  fillRoundedRect(buf, pose.right.x, pose.right.y, pose.right.w, pose.right.h, CORNER);
}

function dotMaskAt(x, y) {
  if (x < 0 || y < 0 || x >= DOT_SIZE || y >= DOT_SIZE) return false;
  return (DOT_MASK[y] & (1 << (DOT_SIZE - 1 - x))) !== 0;
}

function drawDot(buf, eye) {
  if (eye.h <= 0 || eye.w <= 0) return;
  const d = Math.min(H, Math.round((DOT_SIZE * eye.h) / DEFAULT_LEFT.h));
  if (d <= 0) return;
  const cx = eye.x + (eye.w / 2) | 0;
  const cy = eye.y + (eye.h / 2) | 0;
  const x0 = cx - (d / 2) | 0;
  const y0 = cy - (d / 2) | 0;
  for (let y = 0; y < d; y++) {
    const sy = ((y * DOT_SIZE + (DOT_SIZE / 2) | 0) / d) | 0;
    for (let x = 0; x < d; x++) {
      const sx = ((x * DOT_SIZE + (DOT_SIZE / 2) | 0) / d) | 0;
      if (dotMaskAt(sx, sy)) put(buf, x0 + x, y0 + y);
    }
  }
}

function drawDots(buf, pose) {
  clear(buf);
  drawDot(buf, pose.left);
  drawDot(buf, pose.right);
}

function drawKaomoji(buf, nowMs, reduceMotion) {
  clear(buf);
  let id = "idle";
  let t = 0;
  if (!reduceMotion) {
    const slot = Math.floor(nowMs / KAOMOJI_HOLD_MS) % KAOMOJI_FACES.length;
    id = KAOMOJI_FACES[slot];
    const local = nowMs % KAOMOJI_HOLD_MS;
    t = local < 2600 ? local / 1000 : 0;
  }
  const pixels = rasterFace(id, t);
  for (let i = 0; i < pixels.length; i++) {
    if (pixels[i]) buf[i] = 1;
  }
}

function drawKawaii(buf, pose, nowMs, reduceMotion) {
  clear(buf);
  const smilePhase = reduceMotion ? 0 : (nowMs % 5200);
  const smile = !reduceMotion && smilePhase > 2800 && smilePhase < 4200 ? 0.85 : 0.15;
  const gazeX = pose.left.x - DEFAULT_LEFT.x;

  function kawaiiEye(eye, outer) {
    const cx = eye.x + eye.w / 2;
    const cy = eye.y + eye.h / 2;
    const rx = Math.max(2, eye.w * 0.48);
    const ry = Math.max(1, eye.h * 0.55);

    if (pose.open < 0.25) {
      // Closed lid: ‿ or ∩
      const sag = smile > 0.5 ? -2 : 2;
      for (let i = -Math.round(rx * 0.9); i <= Math.round(rx * 0.9); i++) {
        const t = i / (rx * 0.9);
        const y = cy + sag * (1 - t * t);
        put(buf, Math.round(cx + i), Math.round(y));
        put(buf, Math.round(cx + i), Math.round(y) + (sag > 0 ? 1 : -1));
      }
      return;
    }

    // White eye ellipse
    for (let y = Math.floor(cy - ry); y <= Math.ceil(cy + ry); y++) {
      for (let x = Math.floor(cx - rx); x <= Math.ceil(cx + rx); x++) {
        const dx = (x - cx) / rx;
        const dy = (y - cy) / ry;
        if (dx * dx + dy * dy <= 1) put(buf, x, y);
      }
    }

    // Happy carve (lower crescent)
    if (smile > 0.2) {
      const carveRy = ry * (0.4 + smile * 0.55);
      const carveCy = cy + ry * (0.35 + smile * 0.5);
      for (let y = Math.floor(carveCy - carveRy); y <= Math.ceil(carveCy + carveRy); y++) {
        for (let x = Math.floor(cx - rx * 1.1); x <= Math.ceil(cx + rx * 1.1); x++) {
          const dx = (x - cx) / (rx * (1.1 - smile * 0.25));
          const dy = (y - carveCy) / carveRy;
          if (dx * dx + dy * dy <= 1) put(buf, x, y, 0);
        }
      }
    }

    // Pupil (black) + catch-lights
    const pupilR = Math.max(2, Math.min(rx, ry) * 0.55);
    const px = cx + gazeX * 0.35;
    const py = cy - 0.5;
    fillCircle(buf, px, py, pupilR, 0);
    fillCircle(buf, px - pupilR * 0.3, py - pupilR * 0.3, Math.max(1, pupilR * 0.32), 1);
    fillCircle(buf, px + pupilR * 0.45, py - pupilR * 0.35, Math.max(1, pupilR * 0.14), 1);

    // Blush under outer corner
    const blushCx = outer === "left" ? cx - rx - 1 : cx + rx + 1;
    const blushCy = cy + ry * 0.55;
    for (let y = -3; y <= 3; y++) {
      for (let x = -6; x <= 6; x++) {
        const nx = x / 5.5;
        const ny = y / 2.4;
        if (nx * nx + ny * ny > 1) continue;
        if (((x + y) & 1) === 0) put(buf, Math.round(blushCx + x), Math.round(blushCy + y));
      }
    }
  }

  kawaiiEye(pose.left, "left");
  kawaiiEye(pose.right, "right");

  // Mouth between eyes
  const mx = 64;
  const my = 26;
  const mouthOpen = smile > 0.5 ? 3 : 1;
  const mouthW = 10 + smile * 4;
  if (mouthOpen <= 1) {
    for (let i = -mouthW / 2; i <= mouthW / 2; i++) {
      const t = i / (mouthW / 2);
      put(buf, Math.round(mx + i), Math.round(my + (1 - t * t) * (smile > 0.3 ? 1.5 : 0.5)));
    }
  } else {
    fillRect(buf, mx - mouthW / 2, my - 1, mouthW, mouthOpen);
  }
}

const blitScratch = {
  canvas: null,
  ctx: null,
  image: null,
};

function blit(buf, canvas) {
  const ctx = canvas.getContext("2d");
  if (!ctx) return;
  // Prefer CSS pixel size so the panel stays sharp when layout scales.
  const cssW = canvas.clientWidth || canvas.width || W * 3;
  const scale = Math.max(2, Math.round(cssW / W));
  const dw = W * scale;
  const dh = H * scale;
  if (canvas.width !== dw || canvas.height !== dh) {
    canvas.width = dw;
    canvas.height = dh;
  }
  if (!blitScratch.canvas) {
    blitScratch.canvas = document.createElement("canvas");
    blitScratch.canvas.width = W;
    blitScratch.canvas.height = H;
    blitScratch.ctx = blitScratch.canvas.getContext("2d");
    blitScratch.image = blitScratch.ctx.createImageData(W, H);
  }
  const data = blitScratch.image.data;
  for (let i = 0; i < buf.length; i++) {
    const on = buf[i] ? 242 : 8;
    const o = i * 4;
    data[o] = on;
    data[o + 1] = on;
    data[o + 2] = on;
    data[o + 3] = 255;
  }
  blitScratch.ctx.putImageData(blitScratch.image, 0, 0);
  ctx.imageSmoothingEnabled = false;
  ctx.fillStyle = "#080808";
  ctx.fillRect(0, 0, dw, dh);
  ctx.drawImage(blitScratch.canvas, 0, 0, dw, dh);
}

const STYLE_PHASE = {
  classic: 0,
  dots: 1700,
  kawaii: 3100,
  kaomoji: 900,
};

function createPreview(canvas, style) {
  const buf = createBuffer();
  const reduceMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  const phaseMs = STYLE_PHASE[style] || 0;
  let visible = true;
  let raf = 0;
  let last = 0;

  function frame(now) {
    raf = 0;
    if (!visible && !reduceMotion) return;
    if (now - last < FRAME_MS && !reduceMotion) {
      raf = requestAnimationFrame(frame);
      return;
    }
    last = now;
    const pose = poseAt(now, reduceMotion, phaseMs);
    if (style === "classic") drawClassic(buf, pose);
    else if (style === "dots") drawDots(buf, pose);
    else if (style === "kaomoji") drawKaomoji(buf, now + phaseMs, reduceMotion);
    else if (style === "kawaii") drawKawaii(buf, pose, now + phaseMs, reduceMotion);
    blit(buf, canvas);
    if (!reduceMotion && visible) raf = requestAnimationFrame(frame);
  }

  function start() {
    if (raf || reduceMotion) {
      if (reduceMotion) frame(0);
      return;
    }
    raf = requestAnimationFrame(frame);
  }

  function stop() {
    if (raf) cancelAnimationFrame(raf);
    raf = 0;
  }

  const io = new IntersectionObserver(
    (entries) => {
      visible = entries.some((e) => e.isIntersecting);
      if (visible) start();
      else stop();
    },
    { threshold: 0.15 }
  );
  io.observe(canvas);
  start();

  return () => {
    stop();
    io.disconnect();
  };
}

function init() {
  const nodes = document.querySelectorAll("canvas[data-eye-style]");
  for (const canvas of nodes) {
    const style = canvas.getAttribute("data-eye-style");
    if (!style) continue;
    createPreview(canvas, style);
  }
}

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", init);
} else {
  init();
}
