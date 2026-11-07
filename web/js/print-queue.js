/**
 * Print stage queue: parts.json, Shop servo lock, color lanes, checklist,
 * dual downloads, live STL thumbs (shared Three.js renderer).
 */
import * as THREE from "three";
import { STLLoader } from "three/addons/loaders/STLLoader.js";

const SERVO_KEY = "tiny-engineer.shop.servo";
const CHECKED_KEY = "tiny-engineer.print.checked";
const SUPPORTED = new Set(["sg90", "fs0307", "es9051"]);

const read = (key) => {
  try {
    return window.localStorage?.getItem(key) ?? null;
  } catch {
    return null;
  }
};

const write = (key, value) => {
  try {
    if (value == null) window.localStorage?.removeItem(key);
    else window.localStorage.setItem(key, String(value));
  } catch {
    /* private mode / quota */
  }
};

const loadChecked = (key = CHECKED_KEY) => {
  const raw = read(key);
  if (!raw) return new Set();
  try {
    const parsed = JSON.parse(raw);
    if (Array.isArray(parsed)) return new Set(parsed.map(String));
    if (parsed && typeof parsed === "object") {
      return new Set(Object.keys(parsed).filter((k) => parsed[k]));
    }
  } catch {
    /* ignore */
  }
  return new Set();
};

const saveChecked = (set, key = CHECKED_KEY) => {
  write(key, JSON.stringify([...set]));
};

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
      if (!view.active || !view.mesh || !view.ctx) continue;
      any = true;
      view.mesh.rotation.y = now * 0.00045;
      const w = view.canvas.width;
      const h = view.canvas.height;
      r.setSize(w, h, false);
      r.setClearColor(0xf3f1ea, 1);
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

  /** Place camera so the mesh's bounding sphere fits with padding (rotation-safe). */
  const fitCamera = (camera, mesh, padding = 1.55) => {
    mesh.updateWorldMatrix(true, true);
    const sphere = new THREE.Sphere();
    new THREE.Box3().setFromObject(mesh).getBoundingSphere(sphere);
    const fov = (camera.fov * Math.PI) / 180;
    const dist = (sphere.radius / Math.sin(fov / 2)) * padding;
    const dir = new THREE.Vector3(1.05, 0.72, 1.2).normalize();
    camera.position.copy(sphere.center).addScaledVector(dir, dist);
    camera.near = Math.max(dist / 200, 0.01);
    camera.far = dist * 200;
    camera.lookAt(sphere.center);
    camera.updateProjectionMatrix();
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
    if (!hex || hex === "#7a7a72") return "#5c5c56"; // testers — mid grey on white
    if (isDarkSwatch(color)) return "#2e2e2c";
    // Vivid silk-gold on the white stage (reads warmer than CAD copper).
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
    host.classList.add("is-light");
    host.classList.toggle("is-dark-part", dark);

    const scene = new THREE.Scene();
    // White stage — copper / black / grey all read clearly.
    scene.background = new THREE.Color(0xf3f1ea);
    const camera = new THREE.PerspectiveCamera(32, 1, 0.1, 5000);
    const amb = new THREE.AmbientLight(0xffffff, 0.7);
    const key = new THREE.DirectionalLight(0xffffff, 0.95);
    key.position.set(2.4, 3.4, 4.2);
    const fill = new THREE.DirectionalLight(0xffffff, 0.4);
    fill.position.set(-2.8, 1.4, -1.6);
    const rim = new THREE.DirectionalLight(0xffffff, 0.35);
    rim.position.set(-1.4, 2.6, -3.2);
    scene.add(amb, key, fill, rim);

    const view = {
      host,
      canvas,
      ctx: canvas.getContext("2d"),
      scene,
      camera,
      mesh: null,
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
        scene.add(mesh);
        fitCamera(camera, mesh, 1.6);
        view.mesh = mesh;
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

function makeRow(item, { filesBase, swatch, checked, onToggle, previews }) {
  const li = document.createElement("li");
  li.className =
    "shop-item print-item" + (checked.has(item.id) ? " is-done" : "");
  li.dataset.id = item.id;

  const media = document.createElement("div");
  media.className = "shop-item-media print-item-media is-light";
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
    a.className = "shop-item-buy print-dl";
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

function renderGlance(data, checked) {
  const root = el("print-order-timeline");
  const glance = el("print-glance");
  if (!root || !glance) return;

  const steps = [
    {
      id: "testers",
      label: "02",
      title: "Testers",
      ids: (data.testers || []).map((t) => t.id),
      href: "#print-testers",
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
      label: String(n).padStart(2, "0"),
      title: g.label,
      ids,
      href: `#print-lane-${g.id}`,
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
    a.href = step.href;
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
    const row = document.querySelector(`.print-item[data-id="${CSS.escape(id)}"]`);
    if (row) row.classList.toggle("is-done", on);
    updateProgress(data, checked);
  };

  const testerPanel = el("print-testers");
  const testerList = el("print-tester-list");
  if (testerList && testerPanel) {
    testerList.replaceChildren();
    for (const t of data.testers || []) {
      testerList.appendChild(
        makeRow(t, {
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
      const parts = (data.parts || []).filter((p) => p.group === g.id);
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
      finish.textContent = g.tip ? `${g.finish}. ${g.tip}` : g.finish;
      section.appendChild(finish);

      const ul = document.createElement("ul");
      ul.className = "print-check-list";
      for (const p of parts) {
        ul.appendChild(
          makeRow(p, {
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
      document.querySelectorAll(".print-item").forEach((row) => {
        row.classList.remove("is-done");
        const input = row.querySelector(".shop-item-check");
        if (input) input.checked = false;
        const mark = row.querySelector(".shop-card-check");
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
