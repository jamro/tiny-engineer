// Step-by-step build mode: shows the robot as it is at one step of
// docs/3d/assembly.md, slides that step's parts in, and frames them.
// Sub-assemblies built off the robot wait beside it until the step that
// mounts them (`move` in steps.json, see offsetsAt).
import * as THREE from 'three';
import { flatten, offsetsAt, sceneAt, showsCenterCallout } from './steps.js';

/** Guide anchor for the shared centering recipe (docs/3d/assembly.md §2a). */
const CENTER_DOC = '#2a-bench-window-centering';

/** Seconds a step's parts take to slide in. */
const MOVE_SECONDS = 0.9;
/** Seconds autoplay rests on a step after its parts are in place. */
const PLAY_REST_SECONDS = 1.6;

const ease = (t) => t * t * (3 - 2 * t);

/**
 * `parts`: the viewer's parts (id, node). `layout()` re-applies positions and
 * visibility. `label(id)` names a part. Returns the mode's controls.
 */
export function createBuild({ data, parts, stage, root, layout, label, elements, onStep, onActive }) {
  const steps = flatten(data);
  const byId = new Map(parts.map((part) => [part.id, part]));
  let active = false;
  let index = 0;
  let progress = 1;
  let scene = sceneAt(steps, 0);
  let playing = false;
  let stopMove = null;
  let restTimer = 0;

  const offset = new THREE.Vector3();
  let offsets = new Map();
  let offsetsFor = null;
  /** Where a part is drawn relative to its assembled pose (CAD frame), or null. */
  const offsetOf = (id) => {
    if (!active) return null;
    const key = `${index}:${progress}`;
    if (offsetsFor !== key) { offsets = offsetsAt(steps, index, ease(progress)); offsetsFor = key; }
    const value = offsets.get(id);
    return value ? offset.fromArray(value) : null;
  };
  const shown = (id) => !active || scene.shown.has(id) || scene.preview.has(id);
  const ghost = (id) => active && scene.ghost.has(id);

  /** The step's focus parts, where they end up (not where they start moving from). */
  const focusBox = () => {
    const box = new THREE.Box3();
    const saved = progress;
    progress = 1;
    layout();
    root.updateMatrixWorld(true);
    for (const id of scene.focus) {
      const part = byId.get(id);
      if (part) box.expandByObject(part.node);
    }
    progress = saved;
    layout();
    return box.isEmpty() ? new THREE.Box3().setFromObject(root) : box;
  };

  /** How many neighbor steps stay visible above/below the current one. */
  const OUTLINE_PAD = 3;

  const makeMore = (where, count) => {
    const more = document.createElement('li');
    more.className = `asm-outline-more asm-outline-more-${where}`;
    more.setAttribute('aria-hidden', 'true');
    more.textContent = `··· ${count} more`;
    return more;
  };

  /** Rebuild the ±pad step window; mount the player under the current step. */
  const renderOutline = () => {
    if (!elements.outline) return;
    elements.outline.replaceChildren();
    if (!active) {
      if (elements.player) {
        elements.player.hidden = true;
        elements.outline.closest('.asm-build')?.append(elements.player);
      }
      // Intro: first pad+1 steps, cue if the guide continues.
      const end = Math.min(steps.length - 1, OUTLINE_PAD);
      for (let i = 0; i <= end; i++) {
        const step = steps[i];
        const item = document.createElement('li');
        item.className = 'asm-outline-item';
        const label = document.createElement('span');
        label.className = 'asm-outline-label';
        label.textContent = `${i + 1}. ${step.title}`;
        item.append(label);
        elements.outline.append(item);
      }
      if (end < steps.length - 1) {
        elements.outline.append(makeMore('after', steps.length - 1 - end));
      }
      return;
    }

    const from = Math.max(0, index - OUTLINE_PAD);
    const to = Math.min(steps.length - 1, index + OUTLINE_PAD);
    if (from > 0) elements.outline.append(makeMore('before', from));

    for (let i = from; i <= to; i++) {
      const step = steps[i];
      const current = i === index;
      const item = document.createElement('li');
      item.className = 'asm-outline-item' + (current ? ' is-current' : '');
      const label = document.createElement('span');
      label.className = 'asm-outline-label';
      label.textContent = `${i + 1}. ${step.title}`;
      if (current) label.setAttribute('aria-current', 'step');
      item.append(label);
      if (current && elements.player) {
        const panel = document.createElement('div');
        panel.className = 'asm-outline-panel';
        panel.append(elements.player);
        elements.player.hidden = false;
        item.append(panel);
      }
      elements.outline.append(item);
    }

    if (to < steps.length - 1) {
      elements.outline.append(makeMore('after', steps.length - 1 - to));
    }
  };

  const render = () => {
    const step = steps[index];
    elements.title.textContent = step.title;
    const showCenter = showsCenterCallout(step);
    elements.center.hidden = !showCenter;
    if (elements.centerDoc) {
      elements.centerDoc.href = `${data.docs}${CENTER_DOC}`;
    }
    elements.text.textContent = step.text;
    elements.counter.textContent = `Step ${index + 1} of ${steps.length}`;
    elements.progress.value = index + 1;
    elements.progress.max = steps.length;
    elements.screws.hidden = !step.screws;
    elements.screws.textContent = step.screws ? `Screws: ${step.screws}` : '';
    // Screws and nuts are in the step's screw line, not among its parts.
    const added = step.add.filter((id) => !step.fasten?.includes(id)).map(label);
    elements.adds.hidden = !added.length;
    elements.adds.textContent = added.length ? `Adds: ${added.join(', ')}` : '';
    const photo = step.section.photo;
    elements.photo.hidden = !photo;
    if (photo) {
      elements.photoImage.src = `${data.photos}${photo}`;
      elements.photoImage.alt = `Photo: ${step.section.title}`;
      elements.photo.href = `${data.photos}${photo}`;
    }
    elements.prev.disabled = index === 0;
    elements.next.disabled = index === steps.length - 1;
    const playIcon = elements.play.querySelector('use');
    if (playIcon) {
      playIcon.setAttribute('href', playing ? '../media/icons.svg#i-pause' : '../media/icons.svg#i-play');
    }
    elements.play.setAttribute('aria-label', playing ? 'Pause' : 'Play');
    elements.play.setAttribute('aria-pressed', String(playing));
    renderOutline();
  };

  const go = (next, { animate = true } = {}) => {
    if (!active) { active = true; onActive?.(true); }
    index = Math.max(0, Math.min(steps.length - 1, next));
    scene = sceneAt(steps, index);
    progress = animate && scene.moving.size ? 0 : 1;
    if (stopMove) stopMove();
    clearTimeout(restTimer);
    const start = performance.now();
    stopMove = stage.onTick((time) => {
      progress = Math.min(1, (time - start) / (MOVE_SECONDS * 1000));
      layout();
      if (progress >= 1) {
        stopMove();
        stopMove = null;
        if (playing) restTimer = setTimeout(advance, PLAY_REST_SECONDS * 1000);
      }
      return true;
    });
    layout();
    stage.flyTo(focusBox(), { margin: 2.1, minRadius: 45 });
    render();
    onStep?.(index);
  };

  const advance = () => {
    if (index >= steps.length - 1) { setPlaying(false); return; }
    go(index + 1);
  };

  const setPlaying = (value) => {
    playing = value;
    clearTimeout(restTimer);
    if (playing && progress >= 1) restTimer = setTimeout(advance, 300);
    render();
  };

  elements.prev.addEventListener('click', () => { setPlaying(false); go(index - 1); });
  elements.next.addEventListener('click', () => { setPlaying(false); go(index + 1); });
  elements.play.addEventListener('click', () => {
    if (!playing && index === steps.length - 1) go(0);
    setPlaying(!playing);
  });

  // Step outline: ±3 around current; more-before / more-after cues when clipped.
  renderOutline();

  return {
    steps,
    offsetOf,
    shown,
    ghost,
    get active() { return active; },
    get index() { return index; },
    /** Switch build mode on (at `at`, or where it was) or off (the finished robot). */
    setActive(value, at = index) {
      if (value) { go(at); return; }
      if (active) onActive?.(false);
      active = false;
      setPlaying(false);
      if (stopMove) { stopMove(); stopMove = null; }
      progress = 1;
      renderOutline();
      layout();
      stage.flyTo(new THREE.Box3().setFromObject(root), { margin: 1.25 });
    },
    go,
    next: () => { setPlaying(false); go(index + 1); },
    prev: () => { setPlaying(false); go(index - 1); },
  };
}
