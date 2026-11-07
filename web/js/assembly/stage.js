// The 3D stage: renderer, lights, floor, orbit camera. Draws only when
// something changed (camera move, resize, invalidate()), so an idle page
// does not keep the GPU busy.
import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { toCreasedNormals } from 'three/addons/utils/BufferGeometryUtils.js';

/** Faces meeting at more than this angle keep a hard edge. */
const CREASE_ANGLE = THREE.MathUtils.degToRad(30);

/** Smooth normals that keep CAD edges crisp. Returns the geometry to use. */
export function smoothNormals(geometry) {
  const smoothed = toCreasedNormals(geometry, CREASE_ANGLE);
  if (smoothed !== geometry) geometry.dispose();
  return smoothed;
}

/** Sets up the stage in `element`; null when WebGL is unavailable. Scene units: mm, Y up. */
export function createStage(element, label) {
  let renderer;
  try {
    renderer = new THREE.WebGLRenderer({ antialias: true, alpha: false, preserveDrawingBuffer: false });
  } catch {
    return null;
  }
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = THREE.PCFShadowMap;
  renderer.outputColorSpace = THREE.SRGBColorSpace;
  renderer.setClearColor(0x161616, 1);
  renderer.domElement.setAttribute('aria-label', label);
  renderer.domElement.setAttribute('role', 'img');
  element.appendChild(renderer.domElement);

  const scene = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera(32, 1, 1, 20000);
  const controls = new OrbitControls(camera, renderer.domElement);
  let dirty = true;
  const invalidate = () => { dirty = true; };
  controls.addEventListener('change', invalidate);
  controls.enableDamping = true;
  controls.dampingFactor = 0.08;
  controls.minDistance = 40;
  controls.maxDistance = 2500;

  scene.add(new THREE.HemisphereLight(0xffffff, 0x3a3a34, 2.2));
  const key = new THREE.DirectionalLight(0xfff4e6, 3.2);
  key.position.set(260, 420, 300);
  key.castShadow = true;
  key.shadow.mapSize.set(2048, 2048);
  Object.assign(key.shadow.camera, { left: -300, right: 300, top: 300, bottom: -300, far: 1500 });
  key.shadow.bias = -0.0004;
  scene.add(key);
  const fill = new THREE.DirectionalLight(0xdfe8ff, 1.2);
  fill.position.set(-300, 160, -200);
  scene.add(fill);

  const floorMaterial = new THREE.ShadowMaterial({ opacity: 0.35 });
  const floor = new THREE.Mesh(new THREE.PlaneGeometry(4000, 4000), floorMaterial);
  floor.rotation.x = -Math.PI / 2;
  floor.receiveShadow = true;
  scene.add(floor);
  const grid = new THREE.GridHelper(1200, 60, 0x34342e, 0x262622);
  scene.add(grid);

  const content = new THREE.Group();
  scene.add(content);

  const home = { position: new THREE.Vector3(300, 260, 380), target: new THREE.Vector3(0, 60, 0) };
  const reset = () => {
    camera.position.copy(home.position);
    controls.target.copy(home.target);
    controls.update();
    invalidate();
  };

  let framed = null;
  let flight = null;
  let zoomAnim = null;
  let userMoved = false;
  /** When true, resize only updates aspect — never snaps back to a default frame(). */
  let poseLocked = false;
  const resizeListeners = new Set();
  controls.addEventListener('start', () => { userMoved = true; });
  const tickers = new Set();
  /** Calls `tick(time)` every frame; a true return redraws. Returns a remover. */
  const onTick = (tick) => { tickers.add(tick); return () => tickers.delete(tick); };

  /** Dolly toward / away from the orbit target; smooth ease, respects min/max distance. */
  const ZOOM_MS = 280;
  const zoomBy = (factor) => {
    const from = camera.position.distanceTo(controls.target);
    const to = THREE.MathUtils.clamp(from * factor, controls.minDistance, controls.maxDistance);
    if (Math.abs(to - from) < 1e-3) return;
    const direction = camera.position.clone().sub(controls.target).normalize();
    const pivot = controls.target.clone();
    const start = performance.now();
    userMoved = true;
    if (zoomAnim) zoomAnim();
    zoomAnim = onTick((time) => {
      const t = Math.min(1, (time - start) / ZOOM_MS);
      const e = t * t * (3 - 2 * t);
      const distance = from + (to - from) * e;
      camera.position.copy(pivot).addScaledVector(direction, distance);
      controls.target.copy(pivot);
      if (t >= 1) { zoomAnim(); zoomAnim = null; }
      return true;
    });
  };
  const zoomIn = () => zoomBy(0.8);
  const zoomOut = () => zoomBy(1.25);

  /** Frame a bounding box: sets the floor under it and the home view around it. */
  const frame = (box, { keepFloor = false, direction: fromDir = null, margin = 0.9 } = {}) => {
    framed = box.clone();
    userMoved = false;
    poseLocked = false;
    const size = box.getSize(new THREE.Vector3());
    const centre = box.getCenter(new THREE.Vector3());
    if (!keepFloor) floor.position.y = grid.position.y = box.min.y - 0.05;
    const radius = size.length() / 2;
    // Fit the narrower of the two fields of view (a phone is taller than wide).
    const vertical = THREE.MathUtils.degToRad(camera.fov);
    const horizontal = 2 * Math.atan(Math.tan(vertical / 2) * camera.aspect);
    const distance = radius / Math.sin(Math.min(vertical, horizontal) / 2) * margin;
    const direction = (fromDir ? fromDir.clone() : new THREE.Vector3(0.62, 0.42, 0.66)).normalize();
    home.target.copy(centre);
    home.position.copy(centre).addScaledVector(direction, distance);
    key.position.copy(centre).add(new THREE.Vector3(260, 420, 300));
    key.target.position.copy(centre);
    key.target.updateMatrixWorld();
    reset();
  };

  const resize = new ResizeObserver(() => {
    const width = element.clientWidth;
    const height = element.clientHeight;
    if (!width || !height) return;
    renderer.setSize(width, height);
    camera.aspect = width / height;
    camera.updateProjectionMatrix();
    if (poseLocked) {
      // Keep the live camera pose (prep anim / commitView). Only sync home for reset().
      home.position.copy(camera.position);
      home.target.copy(controls.target);
    } else if (framed && !userMoved) {
      frame(framed, { keepFloor: true });
    }
    for (const listener of resizeListeners) listener(width, height);
    invalidate();
  });
  resize.observe(element);

  let animation = 0;
  const loop = (time) => {
    animation = requestAnimationFrame(loop);
    for (const tick of tickers) if (tick(time)) dirty = true;
    controls.update();
    if (dirty) {
      renderer.render(scene, camera);
      dirty = false;
    }
  };
  animation = requestAnimationFrame(loop);

  return {
    renderer, scene, camera, controls, content, invalidate, reset, zoomIn, zoomOut, frame, onTick,
    /**
     * Glide the camera to frame `box`, keeping the direction it looks from.
     * `margin` scales the distance (1 = fits snugly).
     */
    flyTo(box, { duration = 700, margin = 1.35, minRadius = 30 } = {}) {
      const size = box.getSize(new THREE.Vector3());
      const centre = box.getCenter(new THREE.Vector3());
      const vertical = THREE.MathUtils.degToRad(camera.fov);
      const horizontal = 2 * Math.atan(Math.tan(vertical / 2) * camera.aspect);
      const radius = Math.max(size.length() / 2, minRadius);
      const distance = Math.min(controls.maxDistance, (radius / Math.sin(Math.min(vertical, horizontal) / 2)) * margin * 0.75);
      const direction = camera.position.clone().sub(controls.target).normalize();
      const fromPosition = camera.position.clone();
      const fromTarget = controls.target.clone();
      const toPosition = centre.clone().addScaledVector(direction, distance);
      // Keep resize/reset aimed at this frame once the glide finishes.
      const toFramed = box.clone();
      const start = performance.now();
      if (flight) flight();
      const remove = onTick((time) => {
        const f = Math.min(1, (time - start) / duration);
        const e = f * f * (3 - 2 * f);
        camera.position.lerpVectors(fromPosition, toPosition, e);
        controls.target.lerpVectors(fromTarget, centre, e);
        if (f >= 1) {
          framed = toFramed;
          home.target.copy(centre);
          home.position.copy(toPosition);
          userMoved = false;
          poseLocked = true;
          remove();
          flight = null;
        }
        return true;
      });
      flight = remove;
    },
    /** Frame `box` again unless the user has moved the camera since the last framing. */
    refit(box) { if (!userMoved && !poseLocked) frame(box, { keepFloor: true }); },
    /** Stop an in-flight flyTo so another camera move can take over cleanly. */
    cancelFlight() {
      if (flight) { flight(); flight = null; }
    },
    /**
     * Remember `box` as the framed view using the current camera pose (no snap).
     * Locks the pose so container resize will not call frame() and jump the camera.
     * `keepFloor` leaves the grid where it is (tight zooms must not lift it into the mesh).
     */
    commitView(box, { keepFloor = false } = {}) {
      framed = box.clone();
      home.position.copy(camera.position);
      home.target.copy(controls.target);
      userMoved = false;
      poseLocked = true;
      if (box && !keepFloor) {
        floor.position.y = grid.position.y = box.min.y - 0.05;
      }
      invalidate();
    },
    /** Lock the current camera pose against resize reframing (e.g. during a scripted take). */
    lockPose() {
      poseLocked = true;
      home.position.copy(camera.position);
      home.target.copy(controls.target);
      invalidate();
    },
    /** Subscribe to container resizes (after aspect is updated). Returns a remover. */
    onResize(listener) {
      resizeListeners.add(listener);
      return () => resizeListeners.delete(listener);
    },
    /** Pin floor/grid under `box`, `gap` mm below min.y (scene Y-up). */
    setFloorUnder(box, gap = 0.05) {
      floor.position.y = grid.position.y = box.min.y - gap;
      invalidate();
    },
    /** Pin floor/grid to an absolute scene Y (does not track the mesh). */
    setFloorY(y) {
      floor.position.y = grid.position.y = y;
      invalidate();
    },
    dispose() {
      resize.disconnect();
      resizeListeners.clear();
      cancelAnimationFrame(animation);
      controls.dispose();
      renderer.dispose();
      renderer.domElement.remove();
    },
  };
}
