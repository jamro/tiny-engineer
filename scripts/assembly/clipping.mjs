// Mesh interpenetration for the assembly viewer's build steps. No
// dependencies: a bounding volume hierarchy over each part's triangles,
// point-in-solid by ray parity and point-to-surface distance.
//
// How deep part A is inside part B: points spread over A's surface (every
// vertex plus samples about SAMPLE_MM apart) are tested against B. A point
// inside B lies that far from B's surface; the deepest such point, from
// either side, is the penetration depth. Mating faces that only touch give
// (close to) zero, so a small tolerance passes them.
//
// Used by scripts/assembly/check_clipping.mjs; tested by test_clipping.mjs.

/** Spacing of the surface samples, mm. */
export const SAMPLE_MM = 0.8;
/** Triangles per BVH leaf. */
const LEAF = 8;

/** A solid from triangle corners (Float32Array, 9 per triangle) in its final frame. */
export function makeSolid(id, corners) {
  const count = corners.length / 9;
  const tri = Float64Array.from(corners);
  const order = new Uint32Array(count);
  const centre = new Float64Array(count * 3);
  for (let t = 0; t < count; t++) {
    order[t] = t;
    for (let a = 0; a < 3; a++) centre[t * 3 + a] = (tri[t * 9 + a] + tri[t * 9 + 3 + a] + tri[t * 9 + 6 + a]) / 3;
  }
  const nodes = [];
  const build = (start, end) => {
    const box = [Infinity, Infinity, Infinity, -Infinity, -Infinity, -Infinity];
    for (let i = start; i < end; i++) {
      const t = order[i];
      for (let c = 0; c < 3; c++) {
        for (let a = 0; a < 3; a++) {
          const v = tri[t * 9 + c * 3 + a];
          if (v < box[a]) box[a] = v;
          if (v > box[a + 3]) box[a + 3] = v;
        }
      }
    }
    const node = { box, start, end, left: null, right: null };
    nodes.push(node);
    if (end - start > LEAF) {
      let axis = 0;
      for (let a = 1; a < 3; a++) if (box[a + 3] - box[a] > box[axis + 3] - box[axis]) axis = a;
      const slice = Array.from(order.subarray(start, end)).sort((p, q) => centre[p * 3 + axis] - centre[q * 3 + axis]);
      order.set(slice, start);
      const mid = (start + end) >> 1;
      node.left = build(start, mid);
      node.right = build(mid, end);
    }
    return node;
  };
  const root = build(0, count);
  return { id, tri, order, root, box: root.box, samples: surfaceSamples(tri) };
}

/**
 * Points over a surface: every corner, points along every edge, and points
 * inside each triangle, all about SAMPLE_MM apart (a low-discrepancy set, so
 * a long thin triangle gets as many points as its area needs and no more).
 */
function surfaceSamples(tri) {
  const points = [];
  const seen = new Set();
  const count = tri.length / 9;
  const g = 1.32471795724474602596; // plastic number: the R2 sequence
  const a1 = 1 / g, a2 = 1 / (g * g);
  for (let t = 0; t < count; t++) {
    const o = t * 9;
    const corner = (c, k) => tri[o + c * 3 + k];
    for (let c = 0; c < 3; c++) {
      const key = `${corner(c, 0).toFixed(3)},${corner(c, 1).toFixed(3)},${corner(c, 2).toFixed(3)}`;
      if (!seen.has(key)) { seen.add(key); points.push(corner(c, 0), corner(c, 1), corner(c, 2)); }
      const d = (c + 1) % 3;
      const n = Math.floor(Math.hypot(corner(d, 0) - corner(c, 0), corner(d, 1) - corner(c, 1), corner(d, 2) - corner(c, 2)) / SAMPLE_MM);
      for (let i = 1; i < n; i++) {
        const f = i / n;
        points.push(...[0, 1, 2].map((k) => corner(c, k) + (corner(d, k) - corner(c, k)) * f));
      }
    }
    const ux = corner(1, 0) - corner(0, 0), uy = corner(1, 1) - corner(0, 1), uz = corner(1, 2) - corner(0, 2);
    const vx = corner(2, 0) - corner(0, 0), vy = corner(2, 1) - corner(0, 1), vz = corner(2, 2) - corner(0, 2);
    const area = Math.hypot(uy * vz - uz * vy, uz * vx - ux * vz, ux * vy - uy * vx) / 2;
    const n = Math.floor(area / (SAMPLE_MM * SAMPLE_MM));
    for (let i = 0; i < n; i++) {
      let u = (0.5 + a1 * (i + 1)) % 1;
      let v = (0.5 + a2 * (i + 1)) % 1;
      if (u + v > 1) { u = 1 - u; v = 1 - v; }
      points.push(corner(0, 0) + ux * u + vx * v, corner(0, 1) + uy * u + vy * v, corner(0, 2) + uz * u + vz * v);
    }
  }
  return gridded(Float64Array.from(points));
}

/** Cell size of the sample grid, mm. */
const CELL = 4;

/** Samples sorted into a uniform grid, so a query visits only the cells it overlaps. */
function gridded(points) {
  const min = [Infinity, Infinity, Infinity];
  for (let i = 0; i < points.length; i += 3) for (let k = 0; k < 3; k++) min[k] = Math.min(min[k], points[i + k]);
  const cells = new Map();
  for (let i = 0; i < points.length; i += 3) {
    const key = cellKey(...[0, 1, 2].map((k) => Math.floor((points[i + k] - min[k]) / CELL)));
    let list = cells.get(key);
    if (!list) cells.set(key, (list = []));
    list.push(points[i], points[i + 1], points[i + 2]);
  }
  const span = [0, 0, 0];
  for (const key of cells.keys()) {
    const [x, y, z] = uncell(key);
    span[0] = Math.max(span[0], x); span[1] = Math.max(span[1], y); span[2] = Math.max(span[2], z);
  }
  return { min, span, count: points.length / 3, cells: new Map([...cells].map(([key, list]) => [key, Float64Array.from(list)])) };
}

const cellKey = (x, y, z) => (x * 1024 + y) * 1024 + z;
const uncell = (key) => [Math.floor(key / 1048576), Math.floor(key / 1024) % 1024, key % 1024];

/** Calls `visit(cell)` for every sample cell that may hold points in `box`. */
function eachCell(samples, box, visit) {
  const lo = [0, 1, 2].map((k) => Math.max(0, Math.floor((box[k] - samples.min[k]) / CELL)));
  const hi = [0, 1, 2].map((k) => Math.min(samples.span[k], Math.floor((box[k + 3] - samples.min[k]) / CELL)));
  for (let x = lo[0]; x <= hi[0]; x++) {
    for (let y = lo[1]; y <= hi[1]; y++) {
      for (let z = lo[2]; z <= hi[2]; z++) {
        const cell = samples.cells.get(cellKey(x, y, z));
        if (cell && visit(cell) === false) return;
      }
    }
  }
}

/** Do two boxes (min xyz, max xyz) overlap, B shifted by `shift`, grown by `pad`? */
export function boxesOverlap(a, b, shift = [0, 0, 0], pad = 0) {
  for (let k = 0; k < 3; k++) {
    if (a[k] - pad > b[k + 3] + shift[k] || b[k] + shift[k] - pad > a[k + 3]) return false;
  }
  return true;
}

/** Parity of the crossings of a ray from p along axis `axis` (+ direction) with the solid. */
function crossings(solid, p, axis) {
  // Rays along a coordinate axis, nudged off it so they miss shared edges.
  const d = [0, 0, 0];
  d[axis] = 1;
  d[(axis + 1) % 3] = 1.37e-4;
  d[(axis + 2) % 3] = 2.91e-4;
  const { tri, order } = solid;
  let hits = 0;
  const stack = [solid.root];
  while (stack.length) {
    const node = stack.pop();
    if (!rayHitsBox(p, d, node.box)) continue;
    if (node.left) { stack.push(node.left, node.right); continue; }
    for (let i = node.start; i < node.end; i++) if (rayHitsTriangle(p, d, tri, order[i] * 9)) hits++;
  }
  return hits & 1;
}

function rayHitsBox(p, d, box) {
  let near = 0;
  let far = Infinity;
  for (let a = 0; a < 3; a++) {
    if (Math.abs(d[a]) < 1e-12) {
      if (p[a] < box[a] || p[a] > box[a + 3]) return false;
      continue;
    }
    let t1 = (box[a] - p[a]) / d[a];
    let t2 = (box[a + 3] - p[a]) / d[a];
    if (t1 > t2) [t1, t2] = [t2, t1];
    if (t1 > near) near = t1;
    if (t2 < far) far = t2;
    if (near > far) return false;
  }
  return true;
}

/** Möller–Trumbore, hits at t > 0 only. */
function rayHitsTriangle(p, d, tri, o) {
  const e1x = tri[o + 3] - tri[o], e1y = tri[o + 4] - tri[o + 1], e1z = tri[o + 5] - tri[o + 2];
  const e2x = tri[o + 6] - tri[o], e2y = tri[o + 7] - tri[o + 1], e2z = tri[o + 8] - tri[o + 2];
  const hx = d[1] * e2z - d[2] * e2y, hy = d[2] * e2x - d[0] * e2z, hz = d[0] * e2y - d[1] * e2x;
  const det = e1x * hx + e1y * hy + e1z * hz;
  if (Math.abs(det) < 1e-14) return false;
  const inv = 1 / det;
  const sx = p[0] - tri[o], sy = p[1] - tri[o + 1], sz = p[2] - tri[o + 2];
  const u = (sx * hx + sy * hy + sz * hz) * inv;
  if (u < 0 || u > 1) return false;
  const qx = sy * e1z - sz * e1y, qy = sz * e1x - sx * e1z, qz = sx * e1y - sy * e1x;
  const v = (d[0] * qx + d[1] * qy + d[2] * qz) * inv;
  if (v < 0 || u + v > 1) return false;
  return (e2x * qx + e2y * qy + e2z * qz) * inv > 1e-9;
}

/** Is p inside the solid? Ray parity, the inside answer confirmed by a second ray, so a crack in the mesh rarely decides. */
export function inside(solid, p) {
  const b = solid.box;
  if (p[0] < b[0] || p[1] < b[1] || p[2] < b[2] || p[0] > b[3] || p[1] > b[4] || p[2] > b[5]) return false;
  // One ray settles most points (outside); an inside answer is confirmed.
  if (!crossings(solid, p, 2)) return false;
  return crossings(solid, p, 0) === 1 || crossings(solid, p, 1) === 1;
}

/** Distance from p to the solid's surface. */
export function surfaceDistance(solid, p, limit = Infinity) {
  const { tri, order } = solid;
  let best = limit * limit;
  const stack = [solid.root];
  while (stack.length) {
    const node = stack.pop();
    if (boxDistance2(p, node.box) >= best) continue;
    if (node.left) {
      const l = boxDistance2(p, node.left.box);
      const r = boxDistance2(p, node.right.box);
      if (l < r) stack.push(node.right, node.left); else stack.push(node.left, node.right);
      continue;
    }
    for (let i = node.start; i < node.end; i++) {
      const d2 = triangleDistance2(p, tri, order[i] * 9);
      if (d2 < best) best = d2;
    }
  }
  return Math.sqrt(best);
}

function boxDistance2(p, box) {
  let sum = 0;
  for (let a = 0; a < 3; a++) {
    const v = p[a] < box[a] ? box[a] - p[a] : p[a] > box[a + 3] ? p[a] - box[a + 3] : 0;
    sum += v * v;
  }
  return sum;
}

/** Squared distance from p to a triangle (Ericson, Real-Time Collision Detection 5.1.5). */
function triangleDistance2(p, tri, o) {
  const ax = tri[o], ay = tri[o + 1], az = tri[o + 2];
  const abx = tri[o + 3] - ax, aby = tri[o + 4] - ay, abz = tri[o + 5] - az;
  const acx = tri[o + 6] - ax, acy = tri[o + 7] - ay, acz = tri[o + 8] - az;
  const apx = p[0] - ax, apy = p[1] - ay, apz = p[2] - az;
  const d1 = abx * apx + aby * apy + abz * apz;
  const d2 = acx * apx + acy * apy + acz * apz;
  let qx, qy, qz;
  const at = (u, v) => { qx = ax + abx * u + acx * v; qy = ay + aby * u + acy * v; qz = az + abz * u + acz * v; };
  if (d1 <= 0 && d2 <= 0) at(0, 0);
  else {
    const bpx = p[0] - tri[o + 3], bpy = p[1] - tri[o + 4], bpz = p[2] - tri[o + 5];
    const d3 = abx * bpx + aby * bpy + abz * bpz;
    const d4 = acx * bpx + acy * bpy + acz * bpz;
    const cpx = p[0] - tri[o + 6], cpy = p[1] - tri[o + 7], cpz = p[2] - tri[o + 8];
    const d5 = abx * cpx + aby * cpy + abz * cpz;
    const d6 = acx * cpx + acy * cpy + acz * cpz;
    const vc = d1 * d4 - d3 * d2;
    const vb = d5 * d2 - d1 * d6;
    const va = d3 * d6 - d5 * d4;
    if (d3 >= 0 && d4 <= d3) at(1, 0);
    else if (d6 >= 0 && d5 <= d6) at(0, 1);
    else if (vc <= 0 && d1 >= 0 && d3 <= 0) at(d1 / (d1 - d3), 0);
    else if (vb <= 0 && d2 >= 0 && d6 <= 0) at(0, d2 / (d2 - d6));
    else if (va <= 0 && d4 - d3 >= 0 && d5 - d6 >= 0) {
      const w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
      at(1 - w, w);
    } else {
      const denom = 1 / (va + vb + vc);
      at(vb * denom, vc * denom);
    }
  }
  const dx = p[0] - qx, dy = p[1] - qy, dz = p[2] - qz;
  return dx * dx + dy * dy + dz * dz;
}

/** Deepest point of `a`'s surface inside `b`, `b` shifted by `shift` relative to `a`. */
function oneWay(a, b, shift, stopAt, floor) {
  let depth = 0;
  const p = [0, 0, 0];
  const box = b.box;
  // b's box in a's frame.
  const query = [0, 1, 2, 3, 4, 5].map((k) => box[k] + shift[k % 3]);
  eachCell(a.samples, query, (s) => {
    for (let i = 0; i < s.length; i += 3) {
      // Into b's own frame.
      p[0] = s[i] - shift[0]; p[1] = s[i + 1] - shift[1]; p[2] = s[i + 2] - shift[2];
      if (p[0] < box[0] || p[1] < box[1] || p[2] < box[2] || p[0] > box[3] || p[1] > box[4] || p[2] > box[5]) continue;
      // Not deeper than what we have: no need to test.
      if (depth > 0 && boxDepthBound(p, box) <= depth) continue;
      if (!inside(b, p)) continue;
      const d = surfaceDistance(b, p);
      if (d > Math.max(depth, floor)) {
        depth = d;
        if (depth > stopAt) return false;
      }
    }
    return true;
  });
  return depth;
}

/** Do any leaf boxes of two BVHs overlap, the second shifted by `shift`? */
function surfacesMeet(a, b, shift) {
  const stack = [a, b];
  while (stack.length) {
    const y = stack.pop();
    const x = stack.pop();
    if (!boxesOverlap(x.box, y.box, shift)) continue;
    if (!x.left && !y.left) return true;
    // Split the larger node.
    const size = (node) => node.box[3] - node.box[0] + node.box[4] - node.box[1] + node.box[5] - node.box[2];
    if (!y.left || (x.left && size(x) >= size(y))) stack.push(x.left, y, x.right, y);
    else stack.push(x, y.left, x, y.right);
  }
  return false;
}

/** Upper bound on how deep a point can be inside a solid with this box. */
function boxDepthBound(p, box) {
  return Math.min(p[0] - box[0], p[1] - box[1], p[2] - box[2], box[3] - p[0], box[4] - p[1], box[5] - p[2]);
}

/**
 * Penetration depth (mm) of solids `a` and `b`, with `b` moved by `shift`
 * relative to `a`. 0 when they are apart. Stops early once deeper than
 * `stopAt` (the answer is then at least that deep). Points less than
 * `floor` deep are not looked at, so the answer is 0 when nothing is deeper.
 */
export function penetration(a, b, shift = [0, 0, 0], stopAt = Infinity, floor = 0) {
  if (!boxesOverlap(a.box, b.box, shift)) return 0;
  // Surfaces apart (no two leaf boxes overlap): either part is wholly inside
  // the other or they do not meet. One corner each way tells which.
  if (!surfacesMeet(a.root, b.root, shift)) {
    const corner = (solid, shiftBy) => [0, 1, 2].map((k) => solid.tri[k] + shiftBy[k]);
    if (!inside(b, corner(a, shift.map((v) => -v))) && !inside(a, corner(b, shift))) return 0;
  }
  const there = oneWay(a, b, shift, stopAt, floor);
  if (there > stopAt) return there;
  return Math.max(there, oneWay(b, a, shift.map((v) => -v), stopAt, Math.max(floor, there)));
}
