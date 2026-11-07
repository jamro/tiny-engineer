// Flash-page board prep: seat the ESP32 on the main board, then show which
// USB-C to use. Reuses the assembly stage + published SG90 GLB; only the
// bench electronics stay in the scene (docs/flash.md — before assembly).
import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { createStage, smoothNormals } from './assembly/stage.js?v=usbc69';
import { resolveColors } from './assembly/colors.js?v=usbc69';

const MODEL_URL = '../assembly/models/sg90.glb';
const KEEP = new Set(['MainControlBoard', 'ESP32-C3-Zero', 'ESP32-pin-headers']);
const MOVE_IDS = ['ESP32-C3-Zero', 'ESP32-pin-headers'];
/** Seat slide height in CAD mm (Z up under the TinyEngineer root). */
const SEAT_FROM_Z = 30;
/** Keep the floor/grid this many mm below the board (scene Y-up). */
const FLOOR_GAP = 10;
const MOVE_SECONDS = 0.9;
/** Seat + turntable yaw (CAD Z-up → scene Y), same duration. */
const YAW_RAD = Math.PI / 4;
const FLIP_SECONDS = 1.25;
const PLUG_SECONDS = 1.05;
const RIPPLE_SECONDS = 0.675;
/** Glowing USB-mark frame size (CAD mm) — flat rectangle matching silk USB outline. */
const ALIGN_FRAME_W = 7;
const ALIGN_FRAME_H = 2.2;
const ALIGN_FRAME_STROKE = 0.35;
/** Vertical guide posts from base-marker corners (CAD mm, +Z). */
const ALIGN_POST_LEN = -13;
/** How far the plug starts from the receptacle (CAD mm). */
const PLUG_GAP = 42;
/** Seat the metal tip into the receptacle; leave a bit proud so the tip reads. */
const PLUG_INSERT = 2.0;
/** Dolly in toward the board during the plug anim (CAD mm). */
const PLUG_DOLLY = 90;
const ease = (t) => t * t * (3 - 2 * t);

/** Stadium (USB-C) cross-section in the XY plane (width × thickness). */
function usbCShape(width, height, radius) {
  const shape = new THREE.Shape();
  const hw = width / 2;
  const hh = height / 2;
  const r = Math.min(radius, hw, hh);
  shape.moveTo(-hw + r, -hh);
  shape.lineTo(hw - r, -hh);
  shape.quadraticCurveTo(hw, -hh, hw, -hh + r);
  shape.lineTo(hw, hh - r);
  shape.quadraticCurveTo(hw, hh, hw - r, hh);
  shape.lineTo(-hw + r, hh);
  shape.quadraticCurveTo(-hw, hh, -hw, hh - r);
  shape.lineTo(-hw, -hh + r);
  shape.quadraticCurveTo(-hw, -hh, -hw + r, -hh);
  return shape;
}

/** Extrude a 2D shape along −Y (depth mm). Tip face ends at y = 0. */
function extrudeNegY(shape, depth) {
  const geo = new THREE.ExtrudeGeometry(shape, {
    depth,
    bevelEnabled: false,
    curveSegments: 12,
  });
  // ExtrudeGeometry grows +Z → map to −Y.
  geo.rotateX(-Math.PI / 2);
  geo.translate(0, -depth, 0);
  return geo;
}

/** Flat white rectangle + stroke ring in local XY. Scales as a rectangle, not a box. */
function createAlignFrame(w, h, stroke = ALIGN_FRAME_STROKE) {
  const group = new THREE.Group();
  group.name = 'UsbAlignFrame';
  const shared = {
    transparent: true,
    depthTest: false,
    depthWrite: false,
    side: THREE.DoubleSide,
  };
  // Inner fill — white @ 50% opacity.
  const fillShape = new THREE.Shape();
  fillShape.moveTo(-w / 2 + stroke, -h / 2 + stroke);
  fillShape.lineTo(w / 2 - stroke, -h / 2 + stroke);
  fillShape.lineTo(w / 2 - stroke, h / 2 - stroke);
  fillShape.lineTo(-w / 2 + stroke, h / 2 - stroke);
  fillShape.lineTo(-w / 2 + stroke, -h / 2 + stroke);
  const fillMat = new THREE.MeshBasicMaterial({
    color: 0xffffff,
    opacity: 0.5,
    ...shared,
  });
  const fill = new THREE.Mesh(new THREE.ShapeGeometry(fillShape), fillMat);
  fill.renderOrder = 10;
  group.add(fill);
  // Outer white stroke ring.
  const outer = new THREE.Shape();
  outer.moveTo(-w / 2, -h / 2);
  outer.lineTo(w / 2, -h / 2);
  outer.lineTo(w / 2, h / 2);
  outer.lineTo(-w / 2, h / 2);
  outer.lineTo(-w / 2, -h / 2);
  const hw = w / 2 - stroke;
  const hh = h / 2 - stroke;
  const hole = new THREE.Path();
  hole.moveTo(-hw, -hh);
  hole.lineTo(-hw, hh);
  hole.lineTo(hw, hh);
  hole.lineTo(hw, -hh);
  hole.lineTo(-hw, -hh);
  outer.holes.push(hole);
  const strokeMat = new THREE.MeshBasicMaterial({
    color: 0xffffff,
    opacity: 0.95,
    ...shared,
  });
  const ring = new THREE.Mesh(new THREE.ShapeGeometry(outer), strokeMat);
  ring.renderOrder = 11;
  group.add(ring);
  group.userData.setOpacity = (opacity) => {
    strokeMat.opacity = opacity;
    fillMat.opacity = 0.5 * Math.min(1, opacity / 0.95);
  };
  return group;
}

/** Fixed +Z posts on the bottom long edge of the base marker (do not pulse/scale). */
function createAlignCornerPosts(w, h, length = ALIGN_POST_LEN) {
  const group = new THREE.Group();
  group.name = 'UsbAlignPosts';
  const mat = new THREE.LineBasicMaterial({
    color: 0xffffff,
    transparent: true,
    opacity: 0.95,
    depthTest: false,
  });
  // Longer edge is along X; bottom = local y = −h/2.
  const corners = [
    [-w / 2, -h / 2],
    [w / 2, -h / 2],
  ];
  for (const [x, y] of corners) {
    const geo = new THREE.BufferGeometry().setFromPoints([
      new THREE.Vector3(x, y, 0),
      new THREE.Vector3(x, y, length),
    ]);
    const line = new THREE.Line(geo, mat);
    line.renderOrder = 12;
    group.add(line);
  }
  return group;
}

/**
 * Procedural USB-C cable (CAD mm). Tip face at local origin; metal plug
 * extends along −Y; insert direction is +Y into the receptacle.
 * Slightly oversized (~1.35×) so the stadium tip reads at viewer distance.
 */
function createUsbCCable() {
  const group = new THREE.Group();
  group.name = 'UsbCCable';

  // Mid silver — board USB shell reads cooler/darker than raw ALUMINIUM under lights.
  const metal = new THREE.MeshStandardMaterial({
    color: 0xa8adb4,
    metalness: 0.35,
    roughness: 0.38,
  });
  const shell = new THREE.MeshStandardMaterial({
    color: 0xf2f2f4,
    metalness: 0.08,
    roughness: 0.52,
  });
  const cordMat = new THREE.MeshStandardMaterial({
    color: 0xececef,
    metalness: 0.04,
    roughness: 0.62,
  });
  const tongueMat = new THREE.MeshStandardMaterial({
    color: 0xe8ebf0,
    metalness: 0.2,
    roughness: 0.4,
  });

  // Match board USB-C receptacle (~8.94 × 3.2 outer). Plug tip sits inside:
  // USB-IF metal tip ≈ 8.25 × 2.40; keep tip ≤ shell so it can enter the port.
  const tipW = 8.25;
  const tipH = 2.4;
  const tipDepth = 6.5;
  const tip = new THREE.Mesh(extrudeNegY(usbCShape(tipW, tipH, tipH / 2), tipDepth), metal);
  tip.name = 'UsbCTip';
  tip.castShadow = true;

  const tongue = new THREE.Mesh(extrudeNegY(usbCShape(6.5, 0.65, 0.2), 4.2), tongueMat);
  tongue.position.set(0, -0.5, 0);

  // Overmold just behind the tip — only a hair wider than the metal tip.
  const houseDepth = 14;
  const housing = new THREE.Mesh(extrudeNegY(usbCShape(9.4, 5.0, 1.7), houseDepth), shell);
  housing.position.set(0, -tipDepth + 0.3, 0);
  housing.castShadow = true;

  const collarLen = 6;
  const collarR = 2.5;
  // Straight cylinder strain-relief (not a frustum) between housing and cord.
  const collar = new THREE.Mesh(new THREE.CylinderGeometry(collarR, collarR, collarLen, 20), shell);
  collar.position.set(0, -(tipDepth + houseDepth + collarLen / 2 - 0.3), 0);

  const cordLen = 48;
  const cordR = 1.85;
  const cord = new THREE.Mesh(new THREE.CylinderGeometry(cordR, cordR, cordLen, 16), cordMat);
  const cordStart = tipDepth + houseDepth + collarLen - 0.3;
  cord.position.set(0, -(cordStart + cordLen / 2), 0);

  group.add(tip, tongue, housing, collar, cord);
  return group;
}

/** Side panel copy — combined prep instructions (no per-step UI). */
const PREP_COPY = {
  title: 'Prepare the board',
  body:
    'Push the ESP32 module into the sockets on the main board. Line it up with the USB mark printed on the board. '
    + 'Use a USB-C data cable into the board’s USB-C. Do not plug into the small port on the ESP32 module.',
};
/** Pause after step-1 USB zoom before auto-playing step 2. */
const STEP2_DELAY_MS = 1000;
/** Hold the finished plug pose before looping back to the start. */
const LOOP_END_DELAY_MS = 2000;
/** Pause after rewind, before the next seat loop starts. */
const LOOP_RESTART_DELAY_MS = 1000;
/** Smooth rewind: unflip, retract cable, camera → home. */
const LOOP_RESET_SECONDS = 1.35;

const $ = (sel) => document.querySelector(sel);

function setStatus(el, text, state) {
  if (!el) return;
  el.textContent = text;
  if (state) el.dataset.state = state;
  else delete el.dataset.state;
}

async function main() {
  const stageEl = $('#flash-prep-stage');
  const status = $('#flash-prep-status');
  const titleEl = $('#flash-prep-title');
  const bodyEl = $('#flash-prep-body');

  if (!stageEl || !titleEl || !bodyEl) return;

  titleEl.textContent = PREP_COPY.title;
  bodyEl.textContent = PREP_COPY.body;
  bodyEl.classList.remove('flash-prep-body-warn');

  // Animation phase (0 = seat/align, 1 = flip/plug) — not shown in the panel.
  let stepIndex = 0;
  let step2Timer = null;
  let loopEndTimer = null;
  let loopRestartTimer = null;
  const clearStep2Timer = () => {
    if (step2Timer != null) {
      clearTimeout(step2Timer);
      step2Timer = null;
    }
  };
  const clearLoopTimers = () => {
    clearStep2Timer();
    if (loopEndTimer != null) {
      clearTimeout(loopEndTimer);
      loopEndTimer = null;
    }
    if (loopRestartTimer != null) {
      clearTimeout(loopRestartTimer);
      loopRestartTimer = null;
    }
  };

  const stage = createStage(stageEl, 'Main control board and ESP32 preparation animation.');
  if (!stage) {
    stageEl.hidden = true;
    setStatus(status, '3D view needs WebGL. Follow the steps beside the install panel.', 'error');
    return;
  }
  // Scripted take only — no orbit / pan / zoom / roll from the user.
  stage.controls.enabled = false;
  stage.controls.enableRotate = false;
  stage.controls.enablePan = false;
  stage.controls.enableZoom = false;
  stage.controls.enableDamping = false;

  setStatus(status, 'Loading model…', 'loading');

  let gltf;
  try {
    gltf = await new GLTFLoader().loadAsync(MODEL_URL, (event) => {
      if (event.total) setStatus(status, `Loading model… ${Math.round((event.loaded / event.total) * 100)} %`, 'loading');
    });
  } catch (error) {
    stage.dispose();
    stageEl.hidden = true;
    setStatus(status, `Could not load the board model. ${error?.message ?? ''} Follow the steps in text.`, 'error');
    return;
  }

  const root = gltf.scene.getObjectByName('TinyEngineer');
  if (!root) {
    stage.dispose();
    stageEl.hidden = true;
    setStatus(status, 'Board model is missing parts. Follow the steps in text.', 'error');
    return;
  }

  const parts = [];
  for (const node of [...root.children]) {
    if (!KEEP.has(node.name)) {
      root.remove(node);
      continue;
    }
    const extras = node.userData ?? {};
    const mesh = node.isMesh ? node : node.getObjectByProperty('isMesh', true);
    if (!mesh) {
      root.remove(node);
      continue;
    }
    parts.push({
      id: node.name,
      component: extras.component ?? node.name,
      kind: extras.kind ?? 'hardware',
      printGroup: extras.print_group ?? null,
      node,
      mesh,
    });
  }

  if (parts.length < 2 || !parts.some((p) => p.id === 'MainControlBoard') || !parts.some((p) => p.id === 'ESP32-C3-Zero')) {
    stage.dispose();
    stageEl.hidden = true;
    setStatus(status, 'Board model is missing electronics. Follow the steps in text.', 'error');
    return;
  }

  const geometries = new Map();
  for (const part of parts) {
    const source = part.mesh.geometry;
    if (!geometries.has(source)) geometries.set(source, smoothNormals(source));
    part.mesh.geometry = geometries.get(source);
    part.mesh.material = new THREE.MeshStandardMaterial({
      vertexColors: Boolean(part.mesh.geometry.getAttribute('color')),
      roughness: 0.5,
      metalness: 0.04,
    });
    part.mesh.castShadow = true;
    part.mesh.receiveShadow = true;
    part.base = part.node.position.clone();
  }

  const colors = resolveColors(parts);
  for (const part of parts) part.mesh.material.color.set(colors[part.id]);

  stage.content.add(gltf.scene);
  gltf.scene.updateMatrixWorld(true);

  const byId = new Map(parts.map((part) => [part.id, part]));
  const moving = MOVE_IDS.map((id) => byId.get(id)).filter(Boolean);

  const board = byId.get('MainControlBoard');
  const module = byId.get('ESP32-C3-Zero');

  /** Local-space point on a mesh bbox edge, then expressed in `node` local space. */
  const markerOn = (node, mesh, pick) => {
    mesh.geometry.computeBoundingBox();
    const bb = mesh.geometry.boundingBox;
    const localOnMesh = pick(bb);
    mesh.updateWorldMatrix(true, false);
    node.updateWorldMatrix(true, false);
    const world = localOnMesh.applyMatrix4(mesh.matrixWorld);
    const inNode = world.applyMatrix4(new THREE.Matrix4().copy(node.matrixWorld).invert());
    const marker = new THREE.Object3D();
    marker.position.copy(inNode);
    node.add(marker);
    return marker;
  };

  // Board USB-C mouth: footprint centre ≈ (-11.08, -8.35); metal shell overhangs
  // to bbox.min.y. Do NOT use bbox.max.y (speaker J1) or min.z+… (below the PCB).
  const BOARD_USB_X = -11.08;
  // After −90° Z cable twist, plug slides on board X — lateral miss is board Y.
  const BOARD_USB_X_NUDGE = -1.0;
  const BOARD_USB_Y_NUDGE = 9.85;
  const boardMarker = markerOn(board.node, board.mesh, (bb) => {
    const x = Math.min(Math.max(BOARD_USB_X + BOARD_USB_X_NUDGE, bb.min.x + 0.5), bb.max.x - 0.5);
    const y = bb.min.y + 0.8 + BOARD_USB_Y_NUDGE;
    // Mid-height of the USB-C shell on the PCB (was 1.8 — cable clipped under the mouth).
    const cz = 3.2;
    return new THREE.Vector3(x, y, cz);
  });
  // Module USB-C faces +Y in its mesh frame (scripts/assembly/boards.py).
  const moduleMarker = markerOn(module.node, module.mesh, (bb) => {
    const cx = (bb.min.x + bb.max.x) / 2;
    const cz = (bb.min.z + bb.max.z) / 2;
    return new THREE.Vector3(cx, bb.max.y, cz);
  });

  // USB-align frames on the silk mark + two ripple rings (50% phase offset).
  // USB faces board +X when seated — frame long axis along board Y (USB width).
  const silkFrame = createAlignFrame(ALIGN_FRAME_W, ALIGN_FRAME_H);
  const alignPosts = createAlignCornerPosts(ALIGN_FRAME_W, ALIGN_FRAME_H);
  silkFrame.add(alignPosts);
  const rippleFrameA = createAlignFrame(ALIGN_FRAME_W, ALIGN_FRAME_H);
  const rippleFrameB = createAlignFrame(ALIGN_FRAME_W, ALIGN_FRAME_H);
  silkFrame.visible = false;
  rippleFrameA.visible = false;
  rippleFrameB.visible = false;
  silkFrame.rotation.z = Math.PI / 2;
  rippleFrameA.rotation.z = Math.PI / 2;
  rippleFrameB.rotation.z = Math.PI / 2;
  board.node.add(silkFrame);
  board.node.add(rippleFrameA);
  board.node.add(rippleFrameB);

  const syncAlignFrames = () => {
    board.node.updateMatrixWorld(true);
    module.node.updateMatrixWorld(true);
    const usbBoard = new THREE.Vector3();
    moduleMarker.getWorldPosition(usbBoard);
    board.node.worldToLocal(usbBoard);
    // Float just above PCB silk so the tight 45° shot still reads the mark.
    silkFrame.position.set(usbBoard.x - 0.5, usbBoard.y-0.2, -0.1);
    rippleFrameA.position.copy(silkFrame.position);
    rippleFrameA.position.z += 0.04;
    rippleFrameB.position.copy(silkFrame.position);
    rippleFrameB.position.z += 0.08;
    return usbBoard;
  };

  // USB-C cable — child of the board so insert axis stays in PCB space.
  const cable = createUsbCCable();
  cable.visible = false;
  board.node.add(cable);

  let plug = 0;
  let stopPlug = null;
  let stopRipple = null;

  const layoutCable = () => {
    if (!cable.visible && plug <= 0) return;
    board.node.updateMatrixWorld(true);

    // boardMarker is already a child of board.node — its position is board-local.
    const port = boardMarker.position.clone();
    board.mesh.geometry.computeBoundingBox();
    const boardMid = board.mesh.geometry.boundingBox.getCenter(new THREE.Vector3());
    // Mesh-local centre → node-local (same space as the marker).
    if (board.mesh !== board.node) {
      board.mesh.updateWorldMatrix(true, false);
      board.node.updateWorldMatrix(true, false);
      boardMid.applyMatrix4(board.mesh.matrixWorld);
      boardMid.applyMatrix4(new THREE.Matrix4().copy(board.node.matrixWorld).invert());
    }

    // Insert along the board-edge normal (USB faces −Y), not toward the board
    // centre — that diagonal made the plug miss the shell.
    const into = new THREE.Vector3(0, Math.sign(boardMid.y - port.y) || 1, 0);

    // Flat USB-C face: cable local Z ≈ board local Z (component up).
    const yAxis = into;
    const zAxis = new THREE.Vector3(0, 0, 1);
    const xAxis = new THREE.Vector3().crossVectors(yAxis, zAxis);
    if (xAxis.lengthSq() < 1e-6) xAxis.set(1, 0, 0);
    else xAxis.normalize();
    zAxis.crossVectors(xAxis, yAxis).normalize();

    const orient = new THREE.Quaternion()
      .setFromRotationMatrix(new THREE.Matrix4().makeBasis(xAxis, yAxis, zAxis))
      // Twist −90° about local Z so the flat USB-C face matches the receptacle.
      .multiply(new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(0, 0, 1), -Math.PI / 2));
    cable.quaternion.copy(orient);
    // After the −90° Z twist, mesh +Y (insert axis) lands on parent xAxis — slide on that,
    // not on `into`/yAxis (that was the pre-twist cable axis).
    const slide = xAxis;
    const along = -PLUG_GAP + ease(plug) * (PLUG_GAP + PLUG_INSERT);
    cable.position.copy(port).addScaledVector(slide, along);
  };

  const setCableVisible = (on) => {
    cable.visible = on;
    if (!on) plug = 0;
    layoutCable();
    stage.invalidate();
  };

  // Pivot around the board centre so step 2 can flip the board.
  const pivot = new THREE.Group();
  stage.content.add(pivot);
  {
    root.updateMatrixWorld(true);
    const centre = new THREE.Box3().setFromObject(root).getCenter(new THREE.Vector3());
    pivot.position.copy(centre);
    pivot.attach(root);
  }
  const pivotBase = pivot.position.clone();
  const identityQuat = new THREE.Quaternion();

  // Step 2: flip board+ESP 180° about X and 180° about Y.
  const flippedQuat = new THREE.Quaternion().setFromEuler(new THREE.Euler(Math.PI, Math.PI, 0, 'XYZ'));

  let progress = 1;
  let flip = 0;
  let stopMove = null;
  let stopGlide = null;
  /** Locked once from the resting seated pose — never tracks flip lift / zoom boxes. */
  let floorPinnedY = null;

  const layout = () => {
    const lift = (1 - ease(progress)) * SEAT_FROM_Z;
    for (const part of moving) {
      part.node.position.set(part.base.x, part.base.y, part.base.z + lift);
    }
    const e = ease(flip);
    pivot.quaternion.slerpQuaternions(identityQuat, flippedQuat, e);
    // Peak mid-flip so corners clear the floor; return to the same resting height.
    const midLift = Math.sin(e * Math.PI) * 28;
    pivot.position.set(pivotBase.x, pivotBase.y + midLift, pivotBase.z);
    stage.invalidate();
  };

  const focusBox = () => {
    const box = new THREE.Box3();
    root.updateMatrixWorld(true);
    for (const part of parts) box.expandByObject(part.node);
    return box;
  };

  /** Keep FLOOR_GAP under the resting board for the whole prep (no grid jumps). */
  const pinFloor = () => {
    if (floorPinnedY == null) {
      const savedProgress = progress;
      const savedFlip = flip;
      progress = 1;
      flip = 0;
      layout();
      root.updateMatrixWorld(true);
      floorPinnedY = focusBox().min.y - FLOOR_GAP;
      progress = savedProgress;
      flip = savedFlip;
      layout();
    }
    stage.setFloorY(floorPinnedY);
  };

  /** Aim between module USB mouth and the silk align marker; tight zoom. */
  const moduleUsbBox = () => {
    root.updateMatrixWorld(true);
    syncAlignFrames();
    const usb = new THREE.Vector3();
    moduleMarker.getWorldPosition(usb);
    const mark = new THREE.Vector3();
    silkFrame.getWorldPosition(mark);
    const centre = usb.clone().lerp(mark, 0.5);
    return new THREE.Box3().setFromCenterAndSize(centre, new THREE.Vector3(12, 10, 12));
  };

  /** 45° to the board plane, aimed into the USB↔silk gap. */
  const moduleUsbViewDirection = () => {
    root.updateMatrixWorld(true);
    syncAlignFrames();
    const usb = new THREE.Vector3();
    moduleMarker.getWorldPosition(usb);
    const mark = new THREE.Vector3();
    silkFrame.getWorldPosition(mark);
    // Horizontal: camera on USB side of the gap, slight side offset for depth.
    const across = mark.clone().sub(usb);
    across.y = 0;
    if (across.lengthSq() < 1e-6) across.set(1, 0, 0);
    else across.normalize();
    const side = new THREE.Vector3(-across.z, 0, across.x);
    const horizontal = across.multiplyScalar(-0.7).add(side.multiplyScalar(0.7)).normalize();
    // Equal horizontal + up → 45° incidence to the PCB.
    return horizontal.add(new THREE.Vector3(0, 1, 0)).normalize();
  };

  const glideTo = (box, direction, { margin = 1.05, duration = 900, minRadius = 7, extraDistance = 0, onDone } = {}) => {
    if (stopGlide) { stopGlide(); stopGlide = null; }
    stage.cancelFlight();
    const centre = box.getCenter(new THREE.Vector3());
    const size = box.getSize(new THREE.Vector3());
    const radius = Math.max(size.length() / 2, minRadius);
    // Match stage.frame(): fit the narrower FOV so the end pose equals a framed view.
    const vertical = THREE.MathUtils.degToRad(stage.camera.fov);
    const horizontal = 2 * Math.atan(Math.tan(vertical / 2) * stage.camera.aspect);
    const distance = (radius / Math.sin(Math.min(vertical, horizontal) / 2)) * margin + extraDistance;
    const fromPosition = stage.camera.position.clone();
    const fromTarget = stage.controls.target.clone();
    const toPosition = centre.clone().addScaledVector(direction.clone().normalize(), distance);
    // Skip a no-op glide (avoids a tiny frame snap when already there).
    if (fromPosition.distanceTo(toPosition) < 0.4 && fromTarget.distanceTo(centre) < 0.4) {
      stage.camera.position.copy(toPosition);
      stage.controls.target.copy(centre);
      stage.controls.update();
      stage.commitView(box, { keepFloor: true });
      pinFloor();
      onDone?.();
      return;
    }
    const start = performance.now();
    stopGlide = stage.onTick((time) => {
      const f = Math.min(1, (time - start) / duration);
      const e = f * f * (3 - 2 * f);
      stage.camera.position.lerpVectors(fromPosition, toPosition, e);
      stage.controls.target.lerpVectors(fromTarget, centre, e);
      if (f >= 1) {
        stopGlide();
        stopGlide = null;
        stage.camera.position.copy(toPosition);
        stage.controls.target.copy(centre);
        stage.controls.update();
        // Lock resize/reset to this pose — do not call frame() (that snaps).
        stage.commitView(box, { keepFloor: true });
        pinFloor();
        onDone?.();
      }
      return true;
    });
  };

  const setAlignFramesVisible = (on) => {
    silkFrame.visible = on;
    if (!on) {
      if (stopRipple) { stopRipple(); stopRipple = null; }
      rippleFrameA.visible = false;
      rippleFrameB.visible = false;
      silkFrame.userData.setOpacity(0.95);
      rippleFrameA.scale.set(1, 1, 1);
      rippleFrameB.scale.set(1, 1, 1);
    }
    stage.invalidate();
  };

  const applyRipple = (frame, cycle) => {
    const s = 1 + cycle * 2.2;
    frame.scale.set(s, s, 1);
    frame.userData.setOpacity(0.85 * (1 - cycle) * (1 - cycle));
  };

  /** Two water-ripple frames, 50% phase apart, while step 1 align is shown. */
  const startAlignRipple = () => {
    if (stopRipple) { stopRipple(); stopRipple = null; }
    if (stepIndex !== 0) return;
    syncAlignFrames();
    rippleFrameA.visible = true;
    rippleFrameB.visible = true;
    silkFrame.visible = true;
    const start = performance.now();
    stopRipple = stage.onTick((time) => {
      if (stepIndex !== 0 || !silkFrame.visible) {
        stopRipple();
        stopRipple = null;
        rippleFrameA.visible = false;
        rippleFrameB.visible = false;
        return true;
      }
      const cycle = ((time - start) / (RIPPLE_SECONDS * 1000)) % 1;
      applyRipple(rippleFrameA, cycle);
      applyRipple(rippleFrameB, (cycle + 0.5) % 1);
      // Soft pulse on the base mark.
      const pulse = 0.65 + 0.35 * (0.5 + 0.5 * Math.sin(cycle * Math.PI * 2));
      silkFrame.userData.setOpacity(pulse);
      stage.invalidate();
      return true;
    });
  };

  const flyToModuleUsb = () => {
    if (stepIndex !== 0) return;
    syncAlignFrames();
    silkFrame.visible = true;
    stage.invalidate();
    glideTo(moduleUsbBox(), moduleUsbViewDirection(), {
      margin: 1.05,
      duration: 950,
      minRadius: 6,
      extraDistance: 5,
      onDone: () => {
        if (stepIndex !== 0) return;
        startAlignRipple();
        clearStep2Timer();
        step2Timer = setTimeout(() => {
          step2Timer = null;
          if (stepIndex !== 0) return;
          go(1, { animate: true });
        }, STEP2_DELAY_MS);
      },
    });
  };

  const framingDistance = (box, margin = 0.92, minRadius = 16) => {
    const size = box.getSize(new THREE.Vector3());
    const radius = Math.max(size.length() / 2, minRadius);
    const vertical = THREE.MathUtils.degToRad(stage.camera.fov);
    const horizontal = 2 * Math.atan(Math.tan(vertical / 2) * stage.camera.aspect);
    return (radius / Math.sin(Math.min(vertical, horizontal) / 2)) * margin;
  };

  /** Seat ESP down and yaw the camera 45° in sync, then hand off. */
  const playSeat = ({ onDone } = {}) => {
    if (stopMove) stopMove();
    if (stopGlide) { stopGlide(); stopGlide = null; }
    stage.cancelFlight();
    progress = 0;
    flip = 0;
    layout();
    const target = stage.controls.target.clone();
    const fromOffset = stage.camera.position.clone().sub(target);
    const axis = new THREE.Vector3(0, 1, 0);
    const start = performance.now();
    stopMove = stage.onTick((time) => {
      progress = Math.min(1, (time - start) / (MOVE_SECONDS * 1000));
      layout();
      const e = ease(progress);
      const offset = fromOffset.clone().applyAxisAngle(axis, YAW_RAD * e);
      stage.camera.position.copy(target).add(offset);
      stage.controls.target.copy(target);
      stage.controls.update();
      if (progress >= 1) {
        stopMove();
        stopMove = null;
        stage.commitView(focusBox(), { keepFloor: true });
        pinFloor();
        onDone?.();
      }
      return true;
    });
  };

  /**
   * Step 2 as one continuous take: flip then plug yaw/dolly.
   * Look-at stays on pivotBase (ignores midLift bob). One camera formula across the seam.
   */
  const playStep2 = ({ onDone } = {}) => {
    if (stopMove) stopMove();
    if (stopGlide) { stopGlide(); stopGlide = null; }
    if (stopPlug) { stopPlug(); stopPlug = null; }
    stage.cancelFlight();
    progress = 1;
    flip = 0;
    plug = 0;
    setCableVisible(false);
    layout();

    // Resting board size — do not flip the mesh just to measure (that caused a hitch).
    const endDist = framingDistance(focusBox());
    // Allow closer than orbit minDistance so the plug take actually reads as a zoom.
    const plugDist = Math.max(22, endDist - PLUG_DOLLY);

    const fromTarget = stage.controls.target.clone();
    const toTarget = pivotBase.clone();
    const fromOffset = stage.camera.position.clone().sub(fromTarget);
    const startDist = fromOffset.length();
    const startDir = fromOffset.clone().normalize();
    const axis = new THREE.Vector3(0, 1, 0);
    const flipMs = FLIP_SECONDS * 1000;
    const plugMs = PLUG_SECONDS * 1000;
    const totalMs = flipMs + plugMs;
    const start = performance.now();

    stopMove = stage.onTick((time) => {
      const elapsed = Math.min(totalMs, Math.max(0, time - start));
      if (elapsed < flipMs) {
        flip = elapsed / flipMs;
        plug = 0;
      } else {
        flip = 1;
        plug = (elapsed - flipMs) / plugMs;
      }
      layout();

      if (plug > 0) {
        if (!cable.visible) setCableVisible(true);
        layoutCable();
      }

      const flipE = ease(flip);
      const plugE = ease(plug);
      // Stable target — board may midLift, camera does not chase it.
      const target = fromTarget.clone().lerp(toTarget, flipE);
      const dist = startDist + (endDist - startDist) * flipE;
      const offset = startDir.clone().multiplyScalar(dist);
      if (plugE > 0) {
        offset.applyAxisAngle(axis, -YAW_RAD * plugE);
        offset.setLength(dist + (plugDist - dist) * plugE);
      }
      stage.controls.target.copy(target);
      stage.camera.position.copy(target).add(offset);
      stage.controls.update();
      stage.invalidate();

      if (elapsed >= totalMs) {
        flip = 1;
        plug = 1;
        layout();
        layoutCable();
        stopMove();
        stopMove = null;
        stage.commitView(focusBox(), { keepFloor: true });
        pinFloor();
        onDone?.();
      }
      return true;
    });
  };

  /**
   * Smooth loop rewind: unflip, retract cable, lift ESP off the board,
   * camera back to the opening frame. Seat cycle restarts from detached.
   */
  const playLoopReset = ({ onDone } = {}) => {
    if (stopMove) stopMove();
    if (stopGlide) { stopGlide(); stopGlide = null; }
    if (stopPlug) { stopPlug(); stopPlug = null; }
    stage.cancelFlight();
    setAlignFramesVisible(false);
    const fromFlip = flip;
    const fromPlug = plug;
    const fromProgress = progress;
    if (fromPlug > 0) setCableVisible(true);
    const fromPos = stage.camera.position.clone();
    const fromTarget = stage.controls.target.clone();
    const toPos = loopHome.position.clone();
    const toTarget = loopHome.target.clone();
    const start = performance.now();
    stopMove = stage.onTick((time) => {
      const f = Math.min(1, (time - start) / (LOOP_RESET_SECONDS * 1000));
      const e = ease(f);
      flip = fromFlip * (1 - e);
      plug = fromPlug * (1 - e);
      // Detach ESP while rewinding so the next seat take starts in the air.
      progress = fromProgress * (1 - e);
      layout();
      if (cable.visible) layoutCable();
      if (plug <= 0.05) setCableVisible(false);
      stage.camera.position.lerpVectors(fromPos, toPos, e);
      stage.controls.target.lerpVectors(fromTarget, toTarget, e);
      stage.controls.update();
      stage.invalidate();
      if (f >= 1) {
        flip = 0;
        plug = 0;
        progress = 0;
        setCableVisible(false);
        layout();
        stopMove();
        stopMove = null;
        stage.camera.position.copy(toPos);
        stage.controls.target.copy(toTarget);
        stage.controls.update();
        stage.commitView(focusBox(), { keepFloor: true });
        pinFloor();
        onDone?.();
      }
      return true;
    });
  };

  const setFlipped = (value) => {
    flip = value ? 1 : 0;
    layout();
  };

  const go = (next, { animate = true } = {}) => {
    clearLoopTimers();
    stepIndex = next <= 0 ? 0 : 1;
    if (stopMove) { stopMove(); stopMove = null; }
    if (stopGlide) { stopGlide(); stopGlide = null; }
    if (stopPlug) { stopPlug(); stopPlug = null; }
    if (stopRipple) { stopRipple(); stopRipple = null; }
    stage.cancelFlight();
    if (stepIndex !== 1) setCableVisible(false);
    if (stepIndex !== 0) setAlignFramesVisible(false);
    if (stepIndex === 0) {
      setFlipped(false);
      setAlignFramesVisible(false);
      if (animate) {
        // Seat + 45° yaw together, then USB zoom. Auto-advances to step 2 after delay.
        playSeat({ onDone: () => {
          if (stepIndex !== 0) return;
          flyToModuleUsb();
        } });
      } else {
        progress = 1;
        layout();
        flyToModuleUsb();
      }
    } else {
      progress = 1;
      if (animate) {
        playStep2({
          onDone: () => {
            if (stepIndex !== 1) return;
            clearLoopTimers();
            loopEndTimer = setTimeout(() => {
              loopEndTimer = null;
              if (stepIndex !== 1) return;
              playLoopReset({
                onDone: () => {
                  loopRestartTimer = setTimeout(() => {
                    loopRestartTimer = null;
                    go(0, { animate: true });
                  }, LOOP_RESTART_DELAY_MS);
                },
              });
            }, LOOP_END_DELAY_MS);
          },
        });
      } else {
        setFlipped(true);
        setCableVisible(true);
        plug = 1;
        layoutCable();
      }
    }
  };

  // Resting seated pose first so FLOOR_GAP is locked before seat anim lifts the ESP.
  progress = 1;
  flip = 0;
  layout();
  stage.frame(focusBox(), { keepFloor: true });
  pinFloor();
  // Lock pose so container/window resize cannot snap back to a default frame().
  stage.commitView(focusBox(), { keepFloor: true });
  const restingSize = focusBox().getSize(new THREE.Vector3());
  const loopHome = {
    position: stage.camera.position.clone(),
    target: stage.controls.target.clone(),
  };
  /** Rebuild loop-home distance for the new aspect — never mutates flip/progress. */
  const refreshLoopHome = () => {
    const target = pivotBase.clone();
    const dir = loopHome.position.clone().sub(loopHome.target);
    if (dir.lengthSq() < 1e-6) dir.set(0.62, 0.42, 0.66);
    else dir.normalize();
    const box = new THREE.Box3().setFromCenterAndSize(target, restingSize);
    const dist = framingDistance(box, 0.9, 16);
    loopHome.target.copy(target);
    loopHome.position.copy(target).addScaledVector(dir, dist);
  };
  stage.onResize(() => {
    stage.lockPose();
    refreshLoopHome();
    pinFloor();
  });
  // Scripted takes keep the pose locked for the whole loop.
  stage.lockPose();
  setStatus(status, 'Playing preparation animation', 'ready');
  go(0, { animate: true });
}

main().catch((error) => {
  console.error('flash-prep failed', error);
  const status = document.querySelector('#flash-prep-status');
  if (status) {
    status.textContent = `Prep viewer failed. ${error?.message ?? error}`;
    status.dataset.state = 'error';
  }
});
