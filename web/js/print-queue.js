/**
 * Print stage queue: parts.json, Shop servo lock, color lanes, checklist,
 * dual downloads, live STL thumbs (shared Three.js renderer).
 */
import * as THREE from "three";
import { STLLoader } from "three/addons/loaders/STLLoader.js";
import { read, loadChecked, saveChecked } from "./storage.js";

const SERVO_KEY = "tiny-engineer.shop.servo";
const CHECKED_KEY = "tiny-engineer.print.checked";
const SUPPORTED = new Set(["sg90", "fs0307", "es9051"]);

const el = (id) => document.getElementById(id);

const hexColor = (hex, fallback = 0x888888) => {
  const m = /^#?([0-9a-f]{6})$/i.exec(String(hex || ""));
  return m ? parseInt(m[1], 16) : fallback;
};

/** Shared WebGL → 2D canvas thumbs (one context, many views). */
function createPreviewPool() {
  const loader = new STLLoader();
  const geomCache = new Map();
  const views = new Map();
  let renderer = null;
  let webglOk = true;
  let raf = 0;

  const ensureRenderer = () => {
    if (renderer || !webglOk) return renderer;
    try {
      renderer = new THREE.WebGLRenderer({
        antialias: true,
        alpha: true,
        powerPreference: "low-power",
      });
      renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
      renderer.outputColorSpace = THREE.SRGBColorSpace;
      renderer.domElement.style.position = "fixed";
      renderer.domElement.style.left = "-9999px";
      renderer.domElement.style.top = "0";
      renderer.domElement.style.pointerEvents = "none";
      document.body.appendChild(renderer.domElement);
    } catch {
      webglOk = false;
      renderer = null;
    }
    return renderer;
  };

  const loadGeometry = (url) => {
    if (geomCache.has(url)) return geomCache.get(url);
    const promise = new Promise((resolve, reject) => {
      loader.load(
        url,
        (geom) => {
          // Print STLs are Z-up (bed plane); Three.js cameras assume Y-up.
          geom.rotateX(-Math.PI / 2);
          geom.computeVertexNormals();
          geom.center();
          geom.computeBoundingSphere();
          resolve(geom);
        },
        undefined,
        reject,
      );
    });
    geomCache.set(url, promise);
    return promise;
  };

  const tick = (now) => {
    raf = 0;
    const r = ensureRenderer();
    if (!r) return;
    let any = false;
    for (const view of views.values()) {
      if (!view.active || !view.root || !view.ctx) continue;
      any = true;
      // Part + bed grid spin together.
      view.root.rotation.y = now * 0.00045;
      const w = view.canvas.width;
      const h = view.canvas.height;
      r.setSize(w, h, false);
      r.setClearColor(0x161616, 1);
      r.render(view.scene, view.camera);
      view.ctx.clearRect(0, 0, w, h);
      view.ctx.drawImage(r.domElement, 0, 0);
    }
    if (any) raf = requestAnimationFrame(tick);
  };

  const requestTick = () => {
    if (!raf) raf = requestAnimationFrame(tick);
  };

  const showFallback = (host, swatch, label) => {
    host.replaceChildren();
    host.classList.add("is-fallback");
    const sw = document.createElement("span");
    sw.className = "print-preview-swatch";
    sw.style.background = swatch || "#555";
    sw.title = label || "";
    host.appendChild(sw);
  };

  const isDarkSwatch = (color) => {
    const hex = String(color || "").toLowerCase();
    if (hex === "#262624" || hex === "262624") return true;
    const n = hexColor(color, 0xffffff);
    const r = (n >> 16) & 255;
    const g = (n >> 8) & 255;
    const b = n & 255;
    return (r + g + b) / 3 < 70;
  };

  /**
   * Shared printer-bed size (mm) for every thumb — largest stock part footprint
   * is ~120×145 (Desk); 180 leaves margin and keeps scale comparable.
   * Grid look matches assembly `GridHelper` (colors + dark stage).
   */
  const BED_SIZE = 180;
  // 10 mm cells — denser than assembly's 20 mm so thumbs still read the mesh.
  const BED_DIVISIONS = 18;

  /** Same framing for every preview so the XY grid reads the same size. */
  const fitCameraToBed = (camera, bedSize = BED_SIZE) => {
    const center = new THREE.Vector3(0, bedSize * 0.12, 0);
    const dist = bedSize * 1.55;
    const dir = new THREE.Vector3(1.05, 0.72, 1.2).normalize();
    camera.position.copy(center).addScaledVector(dir, dist);
    camera.near = Math.max(dist / 200, 0.01);
    camera.far = dist * 200;
    camera.lookAt(center);
    camera.updateProjectionMatrix();
  };

  /**
   * Same GridHelper palette as assembly/stage.js on XZ (printer XY after STL rotate).
   * Fixed world size for every preview.
   */
  const makeBedGrid = () => {
    const grid = new THREE.GridHelper(
      BED_SIZE,
      BED_DIVISIONS,
      0x34342e,
      0x262622,
    );
    grid.position.y = 0;
    return grid;
  };

  const disposeGrid = (grid) => {
    if (!grid) return;
    grid.geometry?.dispose();
    const mats = Array.isArray(grid.material) ? grid.material : [grid.material];
    for (const mat of mats) mat?.dispose();
  };

  const isCopperSwatch = (color) => {
    const hex = String(color || "").toLowerCase();
    return (
      hex === "#c08a3e" ||
      hex === "#d4a84b" ||
      hex === "#dbb24a" ||
      hex === "c08a3e" ||
      hex === "d4a84b"
    );
  };

  const displayColor = (color) => {
    const hex = String(color || "").toLowerCase();
    if (!hex || hex === "#7a7a72") return "#8a8a82"; // testers — mid grey on dark stage
    if (isDarkSwatch(color)) return "#3a3a38"; // lift black parts off dark clear
    // Vivid silk-gold on the dark stage (reads warmer than CAD copper).
    if (isCopperSwatch(color)) return "#e0b84f";
    return color;
  };

  const attach = (host, { url, color, label }) => {
    if (!host) return () => {};
    if (!ensureRenderer()) {
      showFallback(host, color, label);
      return () => {};
    }

    const dark = isDarkSwatch(color);
    const canvas = document.createElement("canvas");
    canvas.className = "print-preview-canvas";
    canvas.width = 288;
    canvas.height = 288;
    canvas.setAttribute("aria-hidden", "true");
    host.replaceChildren(canvas);
    host.classList.remove("is-fallback");
    host.classList.toggle("is-dark-part", dark);

    const scene = new THREE.Scene();
    // Match assembly stage: dark clear + GridHelper bed.
    scene.background = new THREE.Color(0x161616);
    const camera = new THREE.PerspectiveCamera(32, 1, 0.1, 5000);
    scene.add(new THREE.HemisphereLight(0xffffff, 0x3a3a34, 1.4));
    const key = new THREE.DirectionalLight(0xfff4e6, 1.6);
    key.position.set(2.4, 3.4, 4.2);
    const fill = new THREE.DirectionalLight(0xdfe8ff, 0.7);
    fill.position.set(-2.8, 1.4, -1.6);
    const rim = new THREE.DirectionalLight(0xffffff, 0.35);
    rim.position.set(-1.4, 2.6, -3.2);
    scene.add(key, fill, rim);

    const view = {
      host,
      canvas,
      ctx: canvas.getContext("2d"),
      scene,
      camera,
      root: null,
      mesh: null,
      grid: null,
      active: false,
      color,
      label,
      url,
    };
    views.set(host, view);

    const setActive = (on) => {
      view.active = on;
      if (on) requestTick();
    };

    const io = new IntersectionObserver(
      (entries) => {
        for (const entry of entries) setActive(entry.isIntersecting);
      },
      { rootMargin: "80px", threshold: 0.05 },
    );
    io.observe(host);

    loadGeometry(url)
      .then((geom) => {
        if (!views.has(host)) return;
        const copper = isCopperSwatch(color);
        const mat = new THREE.MeshStandardMaterial({
          color: hexColor(displayColor(color)),
          metalness: copper ? 0.55 : dark ? 0.08 : 0.1,
          roughness: copper ? 0.32 : dark ? 0.62 : 0.55,
        });
        const mesh = new THREE.Mesh(geom.clone(), mat);
        // geom.center() → lift so the part sits on the shared bed (y = 0).
        const box = new THREE.Box3().setFromObject(mesh);
        mesh.position.y = -box.min.y;

        const grid = makeBedGrid();
        const root = new THREE.Group();
        root.add(grid);
        root.add(mesh);
        scene.add(root);

        fitCameraToBed(camera);
        view.root = root;
        view.mesh = mesh;
        view.grid = grid;
        requestTick();
      })
      .catch(() => {
        if (!views.has(host)) return;
        showFallback(host, color, label);
        io.disconnect();
        views.delete(host);
      });

    return () => {
      io.disconnect();
      if (view.mesh) {
        view.mesh.geometry.dispose();
        view.mesh.material.dispose();
      }
      disposeGrid(view.grid);
      if (view.root) scene.remove(view.root);
      views.delete(host);
    };
  };

  return { attach, webglAvailable: () => webglOk && !!ensureRenderer() };
}

function resolveServo(data) {
  const fromJson = data.servoStorageKey || SERVO_KEY;
  const raw = read(fromJson);
  const defaultId = data.defaultServo || "sg90";
  if (!raw) {
    return { id: defaultId, note: null, usedFallback: false };
  }
  if (SUPPORTED.has(raw) && data.servos?.[raw]) {
    return { id: raw, note: null, usedFallback: false };
  }
  return {
    id: defaultId,
    note: `Saved servo “${raw}” is not on the Print page. Showing ${data.servos?.[defaultId]?.short || defaultId}. Change it in Shop.`,
    usedFallback: true,
  };
}

function allCheckIds(data) {
  return [
    ...(data.testers || []).map((t) => t.id),
    ...(data.parts || []).map((p) => p.id),
  ];
}

/** Relative href under current print dir. */
function relFile(filesBase, name) {
  const base = filesBase.endsWith("/") ? filesBase : `${filesBase}/`;
  return base + name;
}

function makeTile(item, { filesBase, swatch, checked, onToggle, previews }) {
  const li = document.createElement("li");
  li.className =
    "shop-item print-item" + (checked.has(item.id) ? " is-done" : "");
  li.dataset.id = item.id;

  const media = document.createElement("div");
  media.className = "shop-item-media print-item-media";
  const dark =
    String(swatch || "").toLowerCase() === "#262624" ||
    String(swatch || "").toLowerCase() === "262624";
  if (dark) media.classList.add("is-dark-part");

  const preview = document.createElement("div");
  preview.className = "print-preview";
  preview.dataset.preview = item.id;
  media.appendChild(preview);

  const checkId = `print-check-${item.id}`;
  const input = document.createElement("input");
  input.type = "checkbox";
  input.id = checkId;
  input.className = "shop-item-check";
  input.checked = checked.has(item.id);
  input.setAttribute("aria-label", `Mark ${item.label} printed`);
  input.title = "Mark as printed";

  const mark = document.createElement("span");
  mark.className = "shop-card-check";
  mark.setAttribute("aria-hidden", "true");
  mark.textContent = "✓";
  mark.hidden = !input.checked;

  const hit = document.createElement("label");
  hit.className = "shop-item-check-hit";
  hit.htmlFor = checkId;
  hit.title = "Mark as printed";
  hit.append(input, mark);
  media.appendChild(hit);

  input.addEventListener("change", () => {
    onToggle(item.id, input.checked);
    mark.hidden = !input.checked;
  });

  const body = document.createElement("div");
  body.className = "shop-item-body print-item-body";

  const head = document.createElement("div");
  head.className = "shop-item-head";

  const name = document.createElement("label");
  name.className = "shop-item-name";
  name.htmlFor = checkId;
  name.textContent = item.label;
  head.appendChild(name);

  if (item.qty && item.qty > 1) {
    const qty = document.createElement("span");
    qty.className = "shop-item-qty mono";
    qty.title = `Quantity: ${item.qty}`;
    qty.setAttribute("aria-label", `Quantity ${item.qty}`);
    qty.textContent = `×${item.qty}`;
    head.appendChild(qty);
  }
  body.appendChild(head);

  if (item.role) {
    const role = document.createElement("p");
    role.className = "shop-item-notes";
    role.textContent = item.role;
    body.appendChild(role);
  }

  const actions = document.createElement("div");
  actions.className = "shop-item-actions print-item-actions";
  for (const fmt of ["3mf", "stl"]) {
    const a = document.createElement("a");
    // Same size; 3MF primary green, STL secondary via print-dl--stl.
    a.className =
      fmt === "3mf"
        ? "shop-item-buy print-dl"
        : "shop-item-buy print-dl print-dl--stl";
    a.href = relFile(filesBase, item.files[fmt]);
    a.download = item.files[fmt];
    a.title = `Download ${item.files[fmt]}`;
    const icon = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    icon.setAttribute("class", "icon");
    icon.setAttribute("aria-hidden", "true");
    const use = document.createElementNS("http://www.w3.org/2000/svg", "use");
    use.setAttribute("href", "../media/icons.svg#i-download");
    icon.appendChild(use);
    const span = document.createElement("span");
    span.textContent = fmt.toUpperCase();
    a.appendChild(icon);
    a.appendChild(span);
    actions.appendChild(a);
  }
  body.appendChild(actions);

  li.appendChild(media);
  li.appendChild(body);

  const stlName = item.files?.stl;
  if (stlName && previews) {
    previews.attach(preview, {
      url: relFile(filesBase, stlName),
      color: swatch,
      label: item.label,
    });
  }

  return li;
}

function glanceTitle(group) {
  // Short labels in the glance strip; full names stay on the lane panels.
  if (group.id === "white") return "White";
  return group.label;
}

function renderGlance(data, checked) {
  const root = el("print-order-timeline");
  const glance = el("print-glance");
  if (!root || !glance) return;

  // Numbers match panel labels (2 · Testers, 3 · Copper …). Set stays panel-only.
  const steps = [
    {
      id: "testers",
      label: "2",
      title: "Testers",
      ids: (data.testers || []).map((t) => t.id),
      targetId: "print-testers",
    },
  ];
  const groups = [...(data.groups || [])].sort(
    (a, b) => (a.order || 0) - (b.order || 0),
  );
  let n = 3;
  for (const g of groups) {
    const ids = (data.parts || [])
      .filter((p) => p.group === g.id)
      .map((p) => p.id);
    if (!ids.length) continue;
    steps.push({
      id: g.id,
      label: String(n),
      title: glanceTitle(g),
      ids,
      targetId: `print-lane-${g.id}`,
      swatch: g.swatch,
    });
    n += 1;
  }

  root.replaceChildren();
  root.style.setProperty("--print-glance-cols", String(Math.max(steps.length, 1)));
  for (const step of steps) {
    const done = step.ids.filter((id) => checked.has(id)).length;
    const total = step.ids.length;
    const li = document.createElement("li");
    const a = document.createElement("a");
    a.className =
      "shop-order-timeline-item" + (done === total && total > 0 ? " is-complete" : "");
    a.href = `#${step.targetId}`;
    a.addEventListener("click", (ev) => {
      ev.preventDefault();
      const target = document.getElementById(step.targetId);
      if (target) {
        target.scrollIntoView({ behavior: "smooth", block: "start" });
      }
      const hash = `#${step.targetId}`;
      if (history.replaceState) {
        history.replaceState(null, "", hash);
      } else {
        location.hash = hash;
      }
    });
    const main = document.createElement("span");
    main.className = "shop-order-timeline-main";
    const lab = document.createElement("span");
    lab.className = "shop-order-timeline-label";
    lab.textContent = step.label;
    const title = document.createElement("span");
    title.className = "shop-order-timeline-title";
    if (step.swatch) {
      const sw = document.createElement("span");
      sw.className = "print-swatch";
      sw.style.background = step.swatch;
      title.appendChild(sw);
      title.appendChild(document.createTextNode(` ${step.title}`));
    } else {
      title.textContent = step.title;
    }
    main.append(lab, title);
    const meta = document.createElement("span");
    meta.className = "shop-order-timeline-meta mono";
    meta.textContent = `${done}/${total}`;
    a.append(main, meta);
    li.appendChild(a);
    root.appendChild(li);
  }
  glance.hidden = false;
}

function updateProgress(data, checked) {
  const ids = allCheckIds(data);
  const done = ids.filter((id) => checked.has(id)).length;
  const total = ids.length;
  const prog = el("print-check-progress");
  if (prog) {
    prog.textContent = `${done} / ${total}`;
    prog.classList.toggle("is-complete", done === total && total > 0);
  }

  const testerIds = (data.testers || []).map((t) => t.id);
  const tDone = testerIds.filter((id) => checked.has(id)).length;
  const tEl = el("print-tester-progress");
  if (tEl) {
    tEl.textContent = `${tDone} / ${testerIds.length}`;
    tEl.classList.toggle(
      "is-complete",
      tDone === testerIds.length && testerIds.length > 0,
    );
  }

  document.querySelectorAll(".shop-lane[data-group]").forEach((lane) => {
    const gid = lane.dataset.group;
    const partIds = (data.parts || [])
      .filter((p) => p.group === gid)
      .map((p) => p.id);
    const n = partIds.filter((id) => checked.has(id)).length;
    const span = lane.querySelector(".shop-lane-progress");
    if (span) {
      span.textContent = `${n} / ${partIds.length}`;
      span.classList.toggle("is-complete", n === partIds.length && partIds.length > 0);
    }
  });

  renderGlance(data, checked);
}

async function main() {
  const status = el("print-status");
  const setStatus = (text, state) => {
    if (!status) return;
    status.textContent = text;
    status.dataset.state = state || "";
    status.hidden = state === "ready";
  };

  let data;
  try {
    const pageRes = await fetch("./parts.json");
    if (!pageRes.ok) throw new Error(`parts.json HTTP ${pageRes.status}`);
    data = await pageRes.json();
  } catch (err) {
    setStatus(`Could not load parts list (${err.message || err}).`, "error");
    return;
  }

  // Normalize keys from JSON (task 1.2)
  const servoKey = data.servoStorageKey || SERVO_KEY;
  const checkedKey = data.checkedStorageKey || CHECKED_KEY;
  if (servoKey !== SERVO_KEY || checkedKey !== CHECKED_KEY) {
    console.warn("print-queue: unexpected storage keys in parts.json", {
      servoKey,
      checkedKey,
    });
  }

  const { id: servoId, note } = resolveServo(data);
  const servo = data.servos[servoId];
  const filesBase = servo.filesBase;
  const checked = loadChecked(checkedKey);
  const previews = createPreviewPool();

  const variant = el("print-variant");
  const labelEl = el("print-variant-label");
  const metaEl = el("print-variant-meta");
  const noteEl = el("print-variant-note");
  if (labelEl) labelEl.textContent = servo.label;
  if (metaEl) {
    metaEl.textContent = `${servo.short} · ${servo.size}`;
  }
  if (noteEl) {
    if (note) {
      noteEl.hidden = false;
      noteEl.textContent = note;
    } else {
      noteEl.hidden = true;
      noteEl.textContent = "";
    }
  }
  if (variant) variant.hidden = false;

  const onToggle = (id, on) => {
    if (on) checked.add(id);
    else checked.delete(id);
    saveChecked(checked, checkedKey);
    const tile = document.querySelector(`.print-item[data-id="${CSS.escape(id)}"]`);
    if (tile) tile.classList.toggle("is-done", on);
    updateProgress(data, checked);
  };

  const testerPanel = el("print-testers");
  const testerList = el("print-tester-list");
  if (testerList && testerPanel) {
    testerList.replaceChildren();
    for (const t of data.testers || []) {
      testerList.appendChild(
        makeTile(t, {
          filesBase,
          swatch: "#7a7a72",
          checked,
          onToggle,
          previews,
        }),
      );
    }
    testerPanel.hidden = false;
  }

  const lanesRoot = el("print-lanes");
  if (lanesRoot) {
    lanesRoot.replaceChildren();
    const groups = [...(data.groups || [])].sort(
      (a, b) => (a.order || 0) - (b.order || 0),
    );
    let stepNo = 3;
    for (const g of groups) {
      const parts = (data.parts || [])
        .filter((p) => p.group === g.id)
        .slice()
        .sort((a, b) => (a.sort ?? 999) - (b.sort ?? 999));
      if (!parts.length) continue;
      const section = document.createElement("section");
      section.className = "panel shop-lane";
      section.id = `print-lane-${g.id}`;
      section.dataset.group = g.id;
      section.setAttribute("aria-label", g.label);

      const head = document.createElement("div");
      head.className = "shop-lane-head";
      const pl = document.createElement("p");
      pl.className = "panel-label";
      const sw = document.createElement("span");
      sw.className = "print-swatch";
      sw.style.background = g.swatch;
      pl.appendChild(document.createTextNode(`${stepNo} · `));
      pl.appendChild(sw);
      pl.appendChild(document.createTextNode(` ${g.label}`));
      const prog = document.createElement("span");
      prog.className = "shop-lane-progress mono";
      head.appendChild(pl);
      head.appendChild(prog);
      section.appendChild(head);

      const finish = document.createElement("p");
      finish.className = "shop-step-lede";
      const finishBits = ["Print in any order within this color."];
      if (g.finish) {
        finishBits.push(
          /[.!?]$/.test(g.finish) ? g.finish : `${g.finish}.`,
        );
      }
      if (g.tip) finishBits.push(g.tip);
      finish.textContent = finishBits.join(" ");
      section.appendChild(finish);

      const ul = document.createElement("ul");
      ul.className = "print-check-list";
      for (const p of parts) {
        ul.appendChild(
          makeTile(p, {
            filesBase,
            swatch: g.swatch,
            checked,
            onToggle,
            previews,
          }),
        );
      }
      section.appendChild(ul);
      lanesRoot.appendChild(section);
      stepNo += 1;
    }
    lanesRoot.hidden = false;
  }

  const footer = el("print-continue");
  if (footer) footer.hidden = false;

  const clearBtn = el("print-clear-checks");
  if (clearBtn) {
    clearBtn.addEventListener("click", () => {
      checked.clear();
      saveChecked(checked, checkedKey);
      document.querySelectorAll(".print-item").forEach((tile) => {
        tile.classList.remove("is-done");
        const input = tile.querySelector(".shop-item-check");
        if (input) input.checked = false;
        const mark = tile.querySelector(".shop-card-check");
        if (mark) mark.hidden = true;
      });
      updateProgress(data, checked);
    });
  }

  updateProgress(data, checked);
  setStatus("Ready.", "ready");
}

main().catch((err) => {
  const status = el("print-status");
  if (status) {
    status.hidden = false;
    status.dataset.state = "error";
    status.textContent = `Print page failed: ${err.message || err}`;
  }
  console.error(err);
});
