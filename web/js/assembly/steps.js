// Assembly steps (web/assembly/steps.json): flattening, the scene at a step,
// and validation against the model's parts. No DOM, so it is testable in Node
// (scripts/assembly/test_viewer.mjs, scripts/assembly/check_steps.mjs).

/** Every step in order, with its section and its index within the section. */
export function flatten(data) {
  const steps = [];
  for (const section of data.sections ?? []) {
    (section.steps ?? []).forEach((step, index) => {
      steps.push({ ...step, section, index, count: section.steps.length, add: step.add ?? [], move: step.move ?? [] });
    });
  }
  return steps;
}

/** Where a part added or moved in `step` starts its move, relative to where it ends (mm, CAD frame). */
export function startOffset(step, id) {
  return step.fromEach?.[id] ?? step.from ?? [0, 0, 0];
}

/** In a step that drives screws in, the parts are home this far through it; the screws then slide in. */
export const FASTEN_AT = 0.6;

/** Where a step's motion changes (0..1): the parts' move, then its screws' (see offsetsAt). */
export function phases(step) {
  return step?.fasten?.length ? [0, FASTEN_AT, 1] : [0, 1];
}

/**
 * Where every part is drawn at step `index`, `progress` (0..1, eased) of the
 * way through its move: part id → offset from its assembled pose (mm, CAD
 * frame). A sub-assembly built off the robot waits at the sum of the moves
 * still ahead of it (`move` in later steps), so it travels with the parts it
 * is joined to and goes home in the step that mounts it. Parts at their
 * assembled pose are left out. In a step with screws and nuts (`fasten`), the
 * parts move up to FASTEN_AT, and then the fasteners slide in along their axes
 * (`slide`).
 */
export function offsetsAt(steps, index, progress = 1) {
  const offsets = new Map();
  const shift = (id, vector, scale) => {
    if (!scale || vector.every((v) => v === 0)) return;
    const current = offsets.get(id) ?? [0, 0, 0];
    offsets.set(id, current.map((v, k) => v + vector[k] * scale));
  };
  const shown = new Set();
  for (let i = 0; i <= index && i < steps.length; i++) for (const id of steps[i].add) shown.add(id);
  for (let i = index; i < steps.length; i++) {
    const step = steps[i];
    // Moves after this step are still ahead; this step's own move is (1 - progress) to go.
    const fastening = i === index && step.fasten?.length;
    const scale = i === index ? 1 - (fastening ? Math.min(1, progress / FASTEN_AT) : progress) : 1;
    for (const id of step.move) if (shown.has(id)) shift(id, startOffset(step, id), scale);
    if (i === index) for (const id of step.add) shift(id, startOffset(step, id), scale);
    if (fastening) {
      const slid = 1 - Math.max(0, (progress - FASTEN_AT) / (1 - FASTEN_AT));
      for (const id of step.fasten) shift(id, step.slide?.[id] ?? [0, 0, 0], slid);
    }
  }
  return offsets;
}

/**
 * The scene at step `index`: the parts shown (added at or before it), the
 * parts moving in during it, the parts drawn as ghosts, and the parts to
 * frame (the step's `focus`, else what it adds, else its section's parts).
 */
export function sceneAt(steps, index) {
  const shown = new Set();
  for (let i = 0; i <= index && i < steps.length; i++) for (const id of steps[i].add) shown.add(id);
  const step = steps[index];
  const moving = new Set([...(step?.add ?? []), ...(step?.move ?? [])]);
  let focus = step?.focus ?? (moving.size ? [...moving] : null);
  if (!focus) focus = steps.filter((other) => other.section === step?.section).flatMap((other) => other.add);
  return { shown, moving, ghost: new Set(step?.ghost ?? []), focus: focus.filter((id) => shown.has(id)) };
}

/**
 * The steps with the model's screws and nuts in them (hardware entries with
 * `fastener`, scripts/assembly/fasteners.py). Each one is added in the step
 * its `step` names (by title): it rides in with the part its head sits on
 * (`fastener.parent`) when that part moves in the step, then slides in along
 * its own axis (`slide`, from `fastener.from`) once the parts are home. Later
 * it moves with its parts: a step that moves one of them moves it too.
 * `fasten` lists a step's fasteners, so the step can name its parts without
 * them.
 */
export function withFasteners(data, hardware) {
  const fasteners = (hardware ?? []).filter((entry) => entry.fastener);
  if (!fasteners.length) return data;
  const sections = data.sections.map((section) => ({ ...section, steps: section.steps.map((step) => ({ ...step })) }));
  const steps = sections.flatMap((section) => section.steps);
  for (const { id, fastener } of fasteners) {
    const at = steps.findIndex((step) => step.title === fastener.step);
    if (at < 0) continue; // validate() reports it: no step adds it
    const step = steps[at];
    const rides = (step.add ?? []).includes(fastener.parent) || (step.move ?? []).includes(fastener.parent);
    step.add = [...(step.add ?? []), id];
    step.fasten = [...(step.fasten ?? []), id];
    step.fromEach = { ...step.fromEach, [id]: rides ? startOffset(step, fastener.parent) : [0, 0, 0] };
    step.slide = { ...step.slide, [id]: fastener.from };
    for (const later of steps.slice(at + 1)) {
      const part = fastener.parts.find((candidate) => (later.move ?? []).includes(candidate));
      if (!part) continue;
      later.fromEach = { ...later.fromEach, [id]: startOffset(later, part) };
      later.move = [...later.move, id];
    }
  }
  return { ...data, sections };
}

/** Problems with the step data for a model with these part ids (empty when fine). */
export function validate(data, ids) {
  const known = new Set(ids);
  const problems = [];
  const added = new Map();
  const vector = (value) => Array.isArray(value) && value.length === 3 && value.every(Number.isFinite);
  if (data.format !== 1) problems.push(`format ${data.format} is not 1`);
  const sectionIds = new Set();
  for (const section of data.sections ?? []) {
    if (!section.id || sectionIds.has(section.id)) problems.push(`section id ${section.id} missing or repeated`);
    sectionIds.add(section.id);
    if (!section.title) problems.push(`section ${section.id}: no title`);
    if (!section.steps?.length) problems.push(`section ${section.id}: no steps`);
  }
  for (const step of flatten(data)) {
    const where = `${step.section.id} step ${step.index + 1}`;
    if (!step.title || !step.text) problems.push(`${where}: needs a title and a text`);
    if (step.from !== undefined && !vector(step.from)) problems.push(`${where}: from is not [x, y, z]`);
    for (const [id, value] of Object.entries(step.fromEach ?? {})) {
      if (!step.add.includes(id) && !step.move.includes(id)) problems.push(`${where}: fromEach names ${id}, which it does not add or move`);
      if (!vector(value)) problems.push(`${where}: fromEach.${id} is not [x, y, z]`);
    }
    for (const id of [...step.add, ...step.move, ...(step.focus ?? []), ...(step.ghost ?? [])]) {
      if (!known.has(id)) problems.push(`${where}: unknown part ${id}`);
    }
    for (const id of step.add) {
      if (added.has(id)) problems.push(`${where}: adds ${id} again (first in ${added.get(id)})`);
      else added.set(id, where);
    }
    for (const id of step.focus ?? []) {
      if (!added.has(id)) problems.push(`${where}: focuses on ${id} before it is added`);
    }
    for (const id of step.move) {
      if (!added.has(id) || step.add.includes(id)) problems.push(`${where}: moves ${id}, which no earlier step adds`);
    }
    if (step.move.length && step.from === undefined && !step.move.every((id) => step.fromEach?.[id])) {
      problems.push(`${where}: moves parts without a from`);
    }
  }
  for (const id of ids) if (!added.has(id)) problems.push(`no step adds ${id}`);
  // Overlaps the clipping check lets through: press fits, and a step's own allowances.
  const allowances = [
    ...(data.fits ?? []).map((entry, index) => [`fits[${index}]`, entry]),
    ...flatten(data).flatMap((step) => (step.allow ?? []).map((entry) => [`${step.section.id} step ${step.index + 1} allow`, entry])),
  ];
  for (const [label, entry] of allowances) {
    if (!Array.isArray(entry.parts) || entry.parts.length !== 2) problems.push(`${label}: parts is not a pair`);
    for (const id of entry.parts ?? []) if (!known.has(id)) problems.push(`${label}: unknown part ${id}`);
    if (!(entry.mm > 0)) problems.push(`${label}: mm must be a positive number`);
    if (!entry.why) problems.push(`${label}: say why`);
  }
  return problems;
}
