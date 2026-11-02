// Tiny Engineer assembly viewer: the robot from the printed meshes, with a
// colour per part. Model: models/<preset>.glb, built by CI from the CAD
// (docs/3d/assembly-viewer.md).
import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { createStage, smoothNormals } from './stage.js';
import { PALETTE, TRANSLUCENT, decodeColors, defaultColor, encodeColors, normalizeHex, resolveColors } from './colors.js';
import { createBuild } from './build.js';
import { rig } from './joints.js';
import { ANIMATIONS, CENTRE, JOINT_IDS, blendPoses, normalizePose } from './poses.js';

const PRESET = 'sg90';
const MODEL_URL = `models/${PRESET}.glb`;
const STEPS_URL = 'steps.json';
/** Opacity of a part drawn as a ghost (see-through) in a build step. */
const GHOST_OPACITY = 0.16;
const STORAGE_KEY = 'te-assembly-colors';
const GROUP_TITLES = {
  PartsSetA: 'Copper set',
  PartsSetB: 'Black set',
  single: 'Printed on its own',
  hardware: 'Electronics',
};
/** How far apart "explode" spreads the parts, as a share of their distance from the centre. */
const EXPLODE_SCALE = 0.85;

const $ = (selector) => document.querySelector(selector);

/** "ForearmLeft" → "Forearm left", "servo-arm-left" → "Servo arm left". */
export function partLabel(id) {
  const words = id
    .replace(/Mirror$/, ' (mirrored)')
    .replace(/[-_]/g, ' ')
    .replace(/([a-z])([A-Z])/g, '$1 $2')
    .toLowerCase()
    .replace(/\boled\b/, 'OLED')
    .replace(/^ai\b/, 'AI');
  return words.charAt(0).toUpperCase() + words.slice(1);
}

const hashParams = () => new URLSearchParams(location.hash.slice(1));

/** Set (or with null, remove) one parameter of the URL hash, keeping the others. */
function setHashParam(key, value) {
  const params = hashParams();
  if (value === null || value === '') params.delete(key);
  else params.set(key, value);
  const url = new URL(location.href);
  url.hash = params.toString().replace(/%7E/gi, '~');
  history.replaceState(null, '', url);
}

function readSavedColors(parts) {
  const hash = hashParams().get('c');
  if (hash !== null) return decodeColors(hash, parts);
  try {
    return decodeColors(localStorage.getItem(STORAGE_KEY) ?? '', parts);
  } catch {
    return {};
  }
}

function saveColors(parts, changes) {
  const encoded = encodeColors(parts, changes);
  setHashParam('c', encoded || null);
  try { localStorage.setItem(STORAGE_KEY, encoded); } catch { /* private mode */ }
}

function groupOf(part) {
  if (part.kind === 'hardware') return 'hardware';
  return part.printGroup ?? 'single';
}

async function main() {
  const status = $('#asm-status');
  const stage = createStage($('#asm-stage'), 'Tiny Engineer, assembled. Drag to orbit, scroll to zoom, right-drag to pan.');
  if (!stage) {
    status.textContent = 'The 3D view needs WebGL. Turn on hardware acceleration in your browser and reload.';
    status.dataset.state = 'error';
    return;
  }

  let gltf;
  let stepData = null;
  try {
    const stepsRequest = fetch(STEPS_URL).then((response) => (response.ok ? response.json() : null)).catch(() => null);
    gltf = await new GLTFLoader().loadAsync(MODEL_URL, (event) => {
      if (event.total) status.textContent = `Loading model… ${Math.round((event.loaded / event.total) * 100)} %`;
    });
    stepData = await stepsRequest;
  } catch (error) {
    status.textContent = `Could not load the model (${MODEL_URL}). ${error?.message ?? ''}`;
    status.dataset.state = 'error';
    return;
  }

  const root = gltf.scene.getObjectByName('TinyEngineer');
  const parts = [];
  for (const node of root.children) {
    const extras = node.userData ?? {};
    const mesh = node.isMesh ? node : node.getObjectByProperty('isMesh', true);
    if (!mesh) continue;
    parts.push({
      id: node.name,
      component: extras.component ?? node.name,
      kind: extras.kind ?? 'printed',
      printGroup: extras.print_group ?? null,
      node,
      mesh,
    });
  }

  // One material per part, so each can take its own colour; normals with a crease angle.
  const geometries = new Map();
  for (const part of parts) {
    const source = part.mesh.geometry;
    if (!geometries.has(source)) geometries.set(source, smoothNormals(source));
    part.mesh.geometry = geometries.get(source);
    part.mesh.material = new THREE.MeshStandardMaterial({
      roughness: part.kind === 'hardware' ? 0.5 : 0.62,
      metalness: part.printGroup === 'PartsSetA' ? 0.35 : 0.04,
      transparent: TRANSLUCENT.has(part.id),
      opacity: TRANSLUCENT.has(part.id) ? 0.72 : 1,
    });
    part.mesh.castShadow = true;
    part.mesh.receiveShadow = true;
    part.mesh.userData.partId = part.id;
    part.base = part.node.position.clone();
    part.opacity = part.mesh.material.opacity;
  }
  stage.content.add(gltf.scene);
  gltf.scene.updateMatrixWorld(true);

  // Explode directions, in the root's (CAD, Z up) frame.
  const robotBox = new THREE.Box3().setFromObject(root);
  const rootInverse = root.matrixWorld.clone().invert();
  const centre = robotBox.getCenter(new THREE.Vector3()).applyMatrix4(rootInverse);
  for (const part of parts) {
    const box = new THREE.Box3().setFromObject(part.node).applyMatrix4(rootInverse);
    const offset = box.getCenter(new THREE.Vector3()).sub(centre);
    offset.z = Math.max(offset.z, 0) + 6; // spread upwards rather than sink into the floor
    part.explode = offset.multiplyScalar(EXPLODE_SCALE);
  }
  stage.frame(robotBox);

  // Servo joints: parts move under pivots on the shafts; positions are now in
  // the pivots' frames.
  const joints = rig(root, new Map(parts.map((part) => [part.id, part.node])));
  for (const part of parts) part.base = part.node.position.clone();

  let changes = readSavedColors(parts);
  let colors = resolveColors(parts, changes);
  let selected = null;

  const applyColors = () => {
    for (const part of parts) part.mesh.material.color.set(colors[part.id]);
    for (const part of parts) {
      const swatch = document.querySelector(`[data-part="${CSS.escape(part.id)}"] .asm-swatch`);
      if (swatch) swatch.style.background = colors[part.id];
    }
    for (const group of Object.keys(GROUP_TITLES)) {
      const members = parts.filter((part) => groupOf(part) === group);
      const input = document.querySelector(`[data-group="${group}"] input[type=color]`);
      if (input && members.length) input.value = colors[members[0].id];
    }
    stage.invalidate();
  };

  const setColor = (ids, hex) => {
    const value = normalizeHex(hex);
    if (!value) return;
    for (const id of ids) changes[id] = value;
    colors = resolveColors(parts, changes);
    saveColors(parts, changes);
    applyColors();
    renderDetail();
  };

  const select = (id) => {
    selected = id;
    for (const part of parts) {
      const on = part.id === id;
      part.mesh.material.emissive.set(on ? 0x2b4a26 : 0x000000);
      document.querySelector(`[data-part="${CSS.escape(part.id)}"]`)?.setAttribute('aria-selected', String(on));
    }
    renderDetail();
    stage.invalidate();
  };

  // Parts list, grouped by how they are printed.
  const list = $('#asm-parts');
  list.replaceChildren();
  for (const [group, title] of Object.entries(GROUP_TITLES)) {
    const members = parts.filter((part) => groupOf(part) === group);
    if (!members.length) continue;
    const section = document.createElement('section');
    section.className = 'asm-group';
    section.dataset.group = group;
    const head = document.createElement('div');
    head.className = 'asm-group-head';
    const label = document.createElement('label');
    label.className = 'asm-group-title';
    label.textContent = `${title} · ${members.length}`;
    const input = document.createElement('input');
    input.type = 'color';
    input.title = `Colour every part of the ${title.toLowerCase()}`;
    input.setAttribute('aria-label', input.title);
    input.addEventListener('input', () => setColor(members.map((part) => part.id), input.value));
    label.prepend(input);
    head.append(label);
    section.append(head);
    const ul = document.createElement('ul');
    ul.className = 'asm-part-list';
    for (const part of members) {
      const li = document.createElement('li');
      const button = document.createElement('button');
      button.type = 'button';
      button.className = 'asm-part';
      button.dataset.part = part.id;
      button.setAttribute('aria-selected', 'false');
      const swatch = document.createElement('span');
      swatch.className = 'asm-swatch';
      swatch.setAttribute('aria-hidden', 'true');
      const name = document.createElement('span');
      name.textContent = partLabel(part.id);
      button.append(swatch, name);
      button.addEventListener('click', () => select(selected === part.id ? null : part.id));
      li.append(button);
      ul.append(li);
    }
    section.append(ul);
    list.append(section);
  }

  // Colour picker for the selected part.
  const detail = $('#asm-detail');
  function renderDetail() {
    const part = parts.find((candidate) => candidate.id === selected);
    detail.hidden = !part;
    if (!part) return;
    $('#asm-detail-name').textContent = partLabel(part.id);
    $('#asm-detail-meta').textContent = part.kind === 'hardware' ? `${part.component} · bought` : `${part.id}.stl · ${GROUP_TITLES[groupOf(part)].toLowerCase()}`;
    const custom = $('#asm-detail-custom');
    custom.value = colors[part.id];
    for (const button of detail.querySelectorAll('.asm-palette button')) {
      button.setAttribute('aria-pressed', String(button.dataset.hex === colors[part.id]));
    }
  }
  const palette = detail.querySelector('.asm-palette');
  for (const { name, hex } of PALETTE) {
    const button = document.createElement('button');
    button.type = 'button';
    button.dataset.hex = hex;
    button.title = name;
    button.setAttribute('aria-label', name);
    button.style.background = hex;
    button.addEventListener('click', () => selected && setColor([selected], hex));
    palette.append(button);
  }
  $('#asm-detail-custom').addEventListener('input', (event) => selected && setColor([selected], event.target.value));
  $('#asm-detail-reset').addEventListener('click', () => {
    if (!selected) return;
    delete changes[selected];
    colors = resolveColors(parts, changes);
    saveColors(parts, changes);
    applyColors();
    renderDetail();
  });
  $('#asm-detail-close').addEventListener('click', () => select(null));

  // Click a part in the view to select it (a drag orbits instead).
  const raycaster = new THREE.Raycaster();
  let down = null;
  const canvas = stage.renderer.domElement;
  canvas.addEventListener('pointerdown', (event) => { down = [event.clientX, event.clientY]; });
  canvas.addEventListener('pointerup', (event) => {
    if (!down || Math.hypot(event.clientX - down[0], event.clientY - down[1]) > 4) return;
    const rect = canvas.getBoundingClientRect();
    const pointer = new THREE.Vector2(((event.clientX - rect.left) / rect.width) * 2 - 1, -((event.clientY - rect.top) / rect.height) * 2 + 1);
    raycaster.setFromCamera(pointer, stage.camera);
    const hit = raycaster.intersectObjects(parts.filter((part) => part.node.visible).map((part) => part.mesh), false)[0];
    const id = hit?.object.userData.partId ?? null;
    select(id === selected ? null : id);
    if (id) document.querySelector(`[data-part="${CSS.escape(id)}"]`)?.scrollIntoView({ block: 'nearest' });
  });

  // Where every part is and whether it shows: explode, the build step, the electronics toggle.
  const explode = $('#asm-explode');
  const electronics = $('#asm-electronics');
  let build = null;
  const rootRotation = new THREE.Quaternion();
  const parentRotation = new THREE.Quaternion();
  const shift = new THREE.Vector3();
  function layout() {
    const amount = Number(explode.value) / 100;
    root.getWorldQuaternion(rootRotation).invert();
    for (const part of parts) {
      // Explode and step offsets are in the CAD frame; turn them into the
      // frame of the joint the part hangs on.
      shift.copy(part.explode).multiplyScalar(amount);
      const offset = build?.offsetOf(part.id);
      if (offset) shift.add(offset);
      if (part.node.parent !== root) {
        part.node.parent.getWorldQuaternion(parentRotation).premultiply(rootRotation).invert();
        shift.applyQuaternion(parentRotation);
      }
      part.node.position.copy(part.base).add(shift);
      part.node.visible = (build ? build.shown(part.id) : true) && (part.kind !== 'hardware' || electronics.checked);
      const ghost = build?.ghost(part.id) ?? false;
      const material = part.mesh.material;
      const transparent = ghost || part.opacity < 1;
      if (material.transparent !== transparent) material.needsUpdate = true;
      material.transparent = transparent;
      material.opacity = ghost ? GHOST_OPACITY : part.opacity;
      material.depthWrite = !ghost;
      part.mesh.castShadow = !ghost;
    }
    stage.invalidate();
  }
  explode.addEventListener('input', layout);
  explode.addEventListener('change', () => stage.refit(new THREE.Box3().setFromObject(root)));
  electronics.addEventListener('change', layout);

  // Joints: sliders per servo, a few animation loops built from the firmware's poses.
  const sliders = new Map();
  const range = $('#asm-range');
  const animSelect = $('#asm-anim');
  const animPlay = $('#asm-anim-play');
  let pose = { ...CENTRE };
  let stopAnimation = null;
  const applyPose = (next) => {
    pose = normalizePose(next);
    for (const id of JOINT_IDS) {
      joints.set(id, pose[id], Number(range.value));
      const slider = sliders.get(id);
      if (slider) {
        slider.input.value = String(Math.round(pose[id] * 100));
        slider.output.value = pose[id].toFixed(2);
      }
    }
    layout();
  };
  const stopPlaying = () => {
    if (stopAnimation) stopAnimation();
    stopAnimation = null;
    animPlay.textContent = 'Play';
    animPlay.setAttribute('aria-pressed', 'false');
  };
  const play = (name) => {
    stopPlaying();
    const animation = ANIMATIONS[name];
    if (!animation) return;
    const start = performance.now();
    const from = { ...pose };
    stopAnimation = stage.onTick((time) => {
      const t = (time - start) / 1000;
      // Ease in from the current pose over the first half second.
      applyPose(blendPoses(from, normalizePose(animation.pose(t)), Math.min(1, t / 0.5)));
      return true;
    });
    animPlay.textContent = 'Pause';
    animPlay.setAttribute('aria-pressed', 'true');
  };
  if (joints.joints.length) {
    $('#asm-joints').hidden = false;
    for (const [name, animation] of Object.entries(ANIMATIONS)) animSelect.add(new Option(animation.label, name));
    const holder = $('#asm-joint-sliders');
    for (const joint of joints.joints) {
      const row = document.createElement('label');
      row.className = 'asm-joint';
      row.title = `${joint.hint} · servo ${joint.index}`;
      const name = document.createElement('span');
      name.textContent = joint.label;
      const input = document.createElement('input');
      input.type = 'range';
      input.min = '-100';
      input.max = '100';
      input.value = '0';
      input.setAttribute('aria-label', `${joint.label} (${joint.hint})`);
      const output = document.createElement('output');
      output.value = '0.00';
      input.addEventListener('input', () => {
        stopPlaying();
        applyPose({ ...pose, [joint.id]: Number(input.value) / 100 });
      });
      row.append(name, input, output);
      holder.append(row);
      sliders.set(joint.id, { input, output });
    }
    animPlay.addEventListener('click', () => (stopAnimation ? stopPlaying() : play(animSelect.value)));
    animSelect.addEventListener('change', () => { if (stopAnimation) play(animSelect.value); });
    $('#asm-pose-centre').addEventListener('click', () => { stopPlaying(); applyPose(CENTRE); });
    range.addEventListener('change', () => applyPose(pose));
  }

  // Build steps and colour tabs.
  const tabs = { build: $('#asm-tab-build'), colors: $('#asm-tab-colors') };
  const panels = { build: $('#asm-build'), colors: $('#asm-colors') };
  if (stepData) {
    build = createBuild({
      data: stepData,
      parts,
      stage,
      root,
      layout,
      label: partLabel,
      onStep: (index) => setHashParam('s', String(index + 1)),
      onActive: (on) => {
        $('#asm-build-intro').hidden = on;
        $('#asm-build-player').hidden = !on;
        // Steps show the robot as assembled: every servo at 90°.
        if (on) { stopPlaying(); applyPose(CENTRE); }
        $('#asm-joints').classList.toggle('is-disabled', on);
        for (const control of $('#asm-joints').querySelectorAll('input, select, button')) control.disabled = on;
      },
      elements: {
        section: $('#asm-step-section'), title: $('#asm-step-title'), text: $('#asm-step-text'),
        counter: $('#asm-step-counter'), progress: $('#asm-step-progress'), screws: $('#asm-step-screws'),
        adds: $('#asm-step-adds'), doc: $('#asm-step-doc'), photo: $('#asm-step-photo'),
        photoImage: $('#asm-step-photo-image'), prev: $('#asm-step-prev'), next: $('#asm-step-next'),
        play: $('#asm-step-play'), outline: $('#asm-step-outline'),
      },
    });
  } else {
    tabs.build.hidden = true;
  }
  const setBuilding = (on, at) => {
    if (!build) return;
    if (on) build.setActive(true, at);
    else if (build.active) {
      build.setActive(false);
      setHashParam('s', null);
    }
  };
  const showTab = (name) => {
    for (const key of Object.keys(tabs)) {
      tabs[key].setAttribute('aria-selected', String(key === name));
      panels[key].hidden = key !== name;
    }
    if (name === 'colors') setBuilding(false);
  };
  if (build) {
    $('#asm-build-summary').textContent = `${stepData.sections.length} sections, ${build.steps.length} steps, following the assembly guide. Each step shows the robot so far and slides the new parts in.`;
    $('#asm-build-start').addEventListener('click', () => setBuilding(true, 0));
    $('#asm-build-exit').addEventListener('click', () => setBuilding(false));
  }
  tabs.build.addEventListener('click', () => showTab('build'));
  tabs.colors.addEventListener('click', () => showTab('colors'));
  document.addEventListener('keydown', (event) => {
    if (!build?.active || event.target.closest?.('input, textarea, select')) return;
    if (event.key === 'ArrowRight') { build.next(); event.preventDefault(); }
    if (event.key === 'ArrowLeft') { build.prev(); event.preventDefault(); }
  });
  $('#asm-reset-view').addEventListener('click', () => stage.reset());
  $('#asm-reset-colors').addEventListener('click', () => {
    changes = {};
    colors = resolveColors(parts, changes);
    saveColors(parts, changes);
    applyColors();
    renderDetail();
  });
  $('#asm-share').addEventListener('click', async () => {
    const button = $('#asm-share');
    try {
      await navigator.clipboard.writeText(location.href);
      button.textContent = 'Link copied';
    } catch {
      button.textContent = 'Copy the address bar';
    }
    setTimeout(() => { button.textContent = 'Copy link'; }, 1800);
  });
  window.addEventListener('hashchange', () => {
    changes = readSavedColors(parts);
    colors = resolveColors(parts, changes);
    applyColors();
    renderDetail();
  });

  applyColors();
  // A link to a step (#s=12) opens it; a link with colours opens the colours.
  const linkedStep = Number(hashParams().get('s'));
  if (build && linkedStep) {
    showTab('build');
    setBuilding(true, linkedStep - 1);
  } else {
    showTab(build && !hashParams().get('c') ? 'build' : 'colors');
    layout();
  }
  status.textContent = `${parts.filter((part) => part.kind !== 'hardware').length} printed parts · ${parts.filter((part) => part.kind === 'hardware').length} electronics · ${PRESET.toUpperCase()}`;
  status.dataset.state = 'ready';
  document.body.dataset.ready = 'true';
  // For tests and the console.
  window.tinyEngineerAssembly = {
    get parts() { return parts.map(({ id, kind, printGroup, node }) => ({ id, kind, printGroup, color: colors[id], visible: node.visible })); },
    select,
    setColor,
    get pose() { return { ...pose }; },
    applyPose,
    play,
    stopPlaying,
    get build() { return build && { active: build.active, index: build.index, count: build.steps.length, go: build.go }; },
    stage,
  };
}

main();
