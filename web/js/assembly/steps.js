// Assembly steps (web/assembly/steps.json): flattening, the scene at a step,
// and validation against the model's parts. No DOM, so it is testable in Node
// (scripts/assembly/test_viewer.mjs, scripts/assembly/check_steps.mjs).

/** Every step in order, with its section and its index within the section. */
export function flatten(data) {
  const steps = [];
  for (const section of data.sections ?? []) {
    (section.steps ?? []).forEach((step, index) => {
      steps.push({ ...step, section, index, count: section.steps.length, add: step.add ?? [] });
    });
  }
  return steps;
}

/** Where a part added in `step` starts its move (mm, CAD frame). */
export function startOffset(step, id) {
  return step.fromEach?.[id] ?? step.from ?? [0, 0, 0];
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
  const moving = new Set(step?.add ?? []);
  let focus = step?.focus ?? (step?.add?.length ? step.add : null);
  if (!focus) focus = steps.filter((other) => other.section === step?.section).flatMap((other) => other.add);
  return { shown, moving, ghost: new Set(step?.ghost ?? []), focus: focus.filter((id) => shown.has(id)) };
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
      if (!step.add.includes(id)) problems.push(`${where}: fromEach names ${id}, which it does not add`);
      if (!vector(value)) problems.push(`${where}: fromEach.${id} is not [x, y, z]`);
    }
    for (const id of [...step.add, ...(step.focus ?? []), ...(step.ghost ?? [])]) {
      if (!known.has(id)) problems.push(`${where}: unknown part ${id}`);
    }
    for (const id of step.add) {
      if (added.has(id)) problems.push(`${where}: adds ${id} again (first in ${added.get(id)})`);
      else added.set(id, where);
    }
    for (const id of step.focus ?? []) {
      if (!added.has(id)) problems.push(`${where}: focuses on ${id} before it is added`);
    }
  }
  for (const id of ids) if (!added.has(id)) problems.push(`no step adds ${id}`);
  return problems;
}
