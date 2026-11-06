// Tiny Engineer assembly viewer: stock SG90 robot from the printed meshes.
// Model: models/sg90.glb, built by CI from the CAD (docs/3d/assembly-viewer.md).
import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { createStage, smoothNormals } from './stage.js';
import { FASTENER_FINISH, TRANSLUCENT, resolveColors } from './colors.js';
import { createBuild } from './build.js';
import { withFasteners } from './steps.js';

const MODEL_URL = 'models/sg90.glb';
const STEPS_URL = 'steps.json';
/** Opacity of a part drawn as a ghost (see-through) in a build step. */
const GHOST_OPACITY = 0.16;

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
  const fasteners = [];
  for (const node of root.children) {
    const extras = node.userData ?? {};
    const mesh = node.isMesh ? node : node.getObjectByProperty('isMesh', true);
    if (!mesh) continue;
    if (extras.kind === 'fastener') {
      fasteners.push({ id: node.name, kind: 'fastener', component: extras.component ?? node.name, fastener: extras.fastener, node, mesh });
      continue;
    }
    parts.push({
      id: node.name,
      component: extras.component ?? node.name,
      kind: extras.kind ?? 'printed',
      printGroup: extras.print_group ?? null,
      node,
      mesh,
    });
  }

  const everything = [...parts, ...fasteners];
  const geometries = new Map();
  for (const part of everything) {
    const source = part.mesh.geometry;
    if (!geometries.has(source)) geometries.set(source, smoothNormals(source));
    part.mesh.geometry = geometries.get(source);
    part.mesh.material = new THREE.MeshStandardMaterial({
      vertexColors: Boolean(part.mesh.geometry.getAttribute('color')),
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

  const robotBox = new THREE.Box3().setFromObject(root);
  stage.frame(robotBox);

  const colors = resolveColors(parts);
  for (const part of parts) part.mesh.material.color.set(colors[part.id]);
  for (const item of fasteners) {
    item.mesh.material.color.set(FASTENER_FINISH.hex);
    item.mesh.material.metalness = FASTENER_FINISH.metalness;
    item.mesh.material.roughness = FASTENER_FINISH.roughness;
  }

  let build = null;
  const shift = new THREE.Vector3();
  function layout() {
    for (const part of everything) {
      shift.set(0, 0, 0);
      const offset = build?.offsetOf(part.id);
      if (offset) shift.add(offset);
      part.node.position.copy(part.base).add(shift);
      part.node.visible = build ? build.shown(part.id) : true;
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

  if (stepData) {
    build = createBuild({
      data: withFasteners(stepData, fasteners.map(({ id, fastener }) => ({ id, fastener }))),
      parts: everything,
      stage,
      root,
      layout,
      label: partLabel,
      onStep: (index) => setHashParam('s', String(index + 1)),
      onActive: (on) => {
        $('#asm-build-intro').hidden = on;
        $('#asm-build-player').hidden = !on;
      },
      elements: {
        section: $('#asm-step-section'), title: $('#asm-step-title'), text: $('#asm-step-text'),
        center: $('#asm-step-center'), centerDoc: $('#asm-step-center-doc'),
        counter: $('#asm-step-counter'), progress: $('#asm-step-progress'), screws: $('#asm-step-screws'),
        adds: $('#asm-step-adds'), photo: $('#asm-step-photo'),
        photoImage: $('#asm-step-photo-image'), prev: $('#asm-step-prev'), next: $('#asm-step-next'),
        play: $('#asm-step-play'), outline: $('#asm-step-outline'),
      },
    });
    $('#asm-build-summary').textContent = `${stepData.sections.length} sections, ${build.steps.length} steps, following the assembly guide. Each step shows the robot so far and slides the new parts in.`;
    $('#asm-build-start').addEventListener('click', () => {
      build.setActive(true, 0);
    });
    $('#asm-build-exit').addEventListener('click', () => {
      if (build.active) {
        build.setActive(false);
        setHashParam('s', null);
      }
    });
  } else {
    $('#asm-build').hidden = true;
  }

  document.addEventListener('keydown', (event) => {
    if (!build?.active || event.target.closest?.('input, textarea, select')) return;
    if (event.key === 'ArrowRight') { build.next(); event.preventDefault(); }
    if (event.key === 'ArrowLeft') { build.prev(); event.preventDefault(); }
  });
  $('#asm-reset-view').addEventListener('click', () => stage.reset());

  const linkedStep = Number(hashParams().get('s'));
  if (build && linkedStep) {
    build.setActive(true, linkedStep - 1);
  } else {
    layout();
  }
  status.textContent = '';
  status.dataset.state = 'ready';
  document.body.dataset.ready = 'true';
  window.tinyEngineerAssembly = {
    get parts() { return parts.map(({ id, kind, printGroup, node }) => ({ id, kind, printGroup, color: colors[id], visible: node.visible })); },
    get fasteners() {
      return fasteners.map(({ id, fastener, mesh, node }) => ({ id, kind: fastener?.kind, size: fastener?.size, color: `#${mesh.material.color.getHexString()}`, visible: node.visible }));
    },
    get build() { return build && { active: build.active, index: build.index, count: build.steps.length, go: build.go }; },
    stage,
  };
}

main();
