/**
 * Shop stage cart: load cart.json, servo pick, JLCPCB lane, checklist + localStorage.
 * Mount: empty nodes with ids shop-* in web/shop/index.html, then this script.
 */
(function () {
  const SERVO_KEY = "tiny-engineer.shop.servo";
  const CHECKED_KEY = "tiny-engineer.shop.checked";

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
      else window.localStorage?.setItem(key, String(value));
    } catch {
      /* private mode / quota */
    }
  };

  const loadChecked = () => {
    const raw = read(CHECKED_KEY);
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

  const saveChecked = (set) => {
    write(CHECKED_KEY, JSON.stringify([...set]));
  };

  const el = (id) => document.getElementById(id);

  const statusEl = el("shop-status");
  const setStatus = (text, state) => {
    if (!statusEl) return;
    statusEl.textContent = text;
    if (state) statusEl.dataset.state = state;
  };

  let cart = null;
  let servoId = null;
  let checked = loadChecked();

  const servoById = (id) => cart?.servos?.find((s) => s.id === id);

  const defaultServoId = () => {
    const saved = read(SERVO_KEY);
    if (saved && servoById(saved)) return saved;
    const def = cart.servos.find((s) => s.default) || cart.servos[0];
    return def?.id || "sg90";
  };

  const itemQty = (item) => {
    if (item.qtyFrom === "servo") {
      return servoById(servoId)?.qty ?? 5;
    }
    return item.qty ?? 1;
  };

  const itemLabel = (item) => {
    if (item.id === "servo") {
      return servoById(servoId)?.label || item.label;
    }
    return item.label;
  };

  const makeServoCard = (s) => {
    const selected = s.id === servoId;
    const tile = document.createElement("div");
    tile.className = "shop-servo-tile" + (selected ? " is-selected" : "");

    const btn = document.createElement("button");
    btn.type = "button";
    btn.className = "shop-servo-card";
    btn.setAttribute("role", "radio");
    btn.setAttribute("aria-checked", selected ? "true" : "false");
    btn.dataset.servo = s.id;
    if (s.size) {
      btn.title = `${s.label} · robot ~${s.size.replace(/^~/, "")}`;
    }

    const media = document.createElement("span");
    media.className = "shop-servo-card-media";
    const img = document.createElement("img");
    img.src = s.image;
    img.alt = "";
    img.width = 1000;
    img.height = 800;
    img.loading = "lazy";
    media.append(img);

    if (selected) {
      const mark = document.createElement("span");
      mark.className = "shop-servo-card-check";
      mark.setAttribute("aria-hidden", "true");
      mark.textContent = "✓";
      media.append(mark);
    }

    const body = document.createElement("span");
    body.className = "shop-servo-card-body";

    const title = document.createElement("span");
    title.className = "shop-servo-card-title";

    const name = document.createElement("span");
    name.className = "shop-servo-card-name";
    name.textContent = s.label || s.short;
    title.append(name);

    if (s.group === "recommended") {
      const tag = document.createElement("span");
      tag.className = "shop-servo-pill shop-servo-pill--accent";
      tag.textContent = "Start here";
      title.append(tag);
    }

    const meta = document.createElement("span");
    meta.className = "shop-servo-card-meta";
    meta.textContent = s.summary || "";

    body.append(title);
    if (s.size) {
      const size = document.createElement("span");
      size.className = "shop-servo-card-size mono";
      size.textContent = `Robot ${s.size}`;
      body.append(size);
    }
    body.append(meta);
    btn.append(media, body);
    btn.addEventListener("click", () => {
      if (servoId === s.id) return;
      servoId = s.id;
      write(SERVO_KEY, servoId);
      render();
    });
    tile.append(btn);

    if (s.url) {
      const buy = document.createElement("a");
      buy.className = "shop-item-buy shop-servo-card-buy";
      buy.href = s.url;
      buy.target = "_blank";
      buy.rel = "noopener noreferrer";
      buy.append("Buy ×5");
      appendExternalIcon(buy);
      tile.append(buy);
    }

    return tile;
  };

  const renderServos = () => {
    const root = el("shop-servo-picker");
    const status = el("shop-servo-status");
    if (!root || !cart) return;

    root.replaceChildren();

    const recommended = cart.servos.filter((s) => s.group === "recommended");
    const compact = cart.servos.filter((s) => s.group === "compact");
    const other = cart.servos.filter(
      (s) => s.group !== "recommended" && s.group !== "compact"
    );

    if (recommended.length) {
      const path = document.createElement("div");
      path.className = "shop-path shop-path--big";
      const label = document.createElement("p");
      label.className = "shop-path-label mono";
      label.textContent = "Larger robot · ~13 × 15 × 12 cm";
      path.append(label);
      recommended.forEach((s) => path.append(makeServoCard(s)));
      root.append(path);
    }

    if (compact.length) {
      const path = document.createElement("div");
      path.className = "shop-path shop-path--small";
      const label = document.createElement("p");
      label.className = "shop-path-label mono";
      label.textContent = "Compact robot · ~11 × 13 × 10 cm — pick either";
      path.append(label);

      const grid = document.createElement("div");
      grid.className = "shop-servo-compact-grid";
      grid.setAttribute("role", "presentation");
      compact.forEach((s) => grid.append(makeServoCard(s)));
      path.append(grid);
      root.append(path);
    }

    other.forEach((s) => root.append(makeServoCard(s)));

    const s = servoById(servoId);
    if (status && s) {
      const qty = s.qty ?? 5;
      status.replaceChildren();
      const lead = document.createElement("span");
      lead.className = "shop-servo-outcome-lead";
      lead.textContent = "Your build";
      const detail = document.createElement("span");
      detail.className = "shop-servo-outcome-detail";
      detail.append(
        document.createTextNode(
          `${s.label || s.short}` +
            (s.size ? ` · robot ${s.size}` : "") +
            ` · buy ×${qty} identical · print `
        )
      );
      const code = document.createElement("code");
      code.textContent = s.printFolder;
      detail.append(code);
      status.append(lead, detail);
    }
  };

  const laneProgress = (lane) => {
    const ids = (lane.items || []).map((item) => item.id);
    const done = ids.filter((id) => checked.has(id)).length;
    return { done, total: ids.length };
  };

  const allCheckProgress = () => {
    let done = 0;
    let total = 0;
    (cart?.lanes || []).forEach((lane) => {
      const p = laneProgress(lane);
      done += p.done;
      total += p.total;
    });
    return { done, total };
  };

  const progressLabel = (done, total) => {
    if (!total) return "";
    if (done >= total) return "✓ done";
    return `${done}/${total}`;
  };

  const updateCheckProgress = () => {
    if (!cart) return;
    const overall = el("shop-check-progress");
    if (overall) {
      const { done, total } = allCheckProgress();
      overall.textContent = total ? `· checks ${done}/${total}` : "";
      overall.classList.toggle("is-complete", total > 0 && done >= total);
    }

    (cart.lanes || []).forEach((lane) => {
      const badge = document.querySelector(`[data-lane-progress="${lane.id}"]`);
      if (!badge) return;
      const { done, total } = laneProgress(lane);
      badge.textContent = progressLabel(done, total);
      badge.hidden = !total;
      badge.classList.toggle("is-complete", total > 0 && done >= total);
    });

    renderTimeline();
  };

  const renderTimeline = () => {
    const root = el("shop-order-timeline");
    if (!root || !cart) return;
    root.replaceChildren();
    const { done: allDone, total: allTotal } = allCheckProgress();
    const steps = [
      {
        label: "1",
        title: "Pick a servo",
        meta: "✓ done",
        complete: true,
        href: "#shop-step-servo",
      },
      ...cart.lanes.map((lane) => {
        const { done, total } = laneProgress(lane);
        return {
          label: lane.label,
          title: lane.title,
          meta: progressLabel(done, total),
          complete: total > 0 && done >= total,
          href: lane.id === "now" ? "#shop-step-board" : `#shop-lane-${lane.id}`,
        };
      }),
      {
        label: "5",
        title: "Continue to print",
        meta: allTotal > 0 && allDone >= allTotal ? "✓ ready" : "…",
        complete: allTotal > 0 && allDone >= allTotal,
        href: "#shop-step-continue",
      },
    ];
    steps.forEach((step) => {
      const item = document.createElement("li");
      const link = document.createElement("a");
      link.className =
        "shop-order-timeline-item" + (step.complete ? " is-complete" : "");
      link.href = step.href;
      const main = document.createElement("span");
      main.className = "shop-order-timeline-main";
      const label = document.createElement("span");
      label.className = "shop-order-timeline-label mono";
      label.textContent = step.label;
      const title = document.createElement("span");
      title.className = "shop-order-timeline-title";
      title.textContent = step.title;
      main.append(label, title);
      link.append(main);
      if (step.meta) {
        const meta = document.createElement("span");
        meta.className = "shop-order-timeline-meta mono";
        meta.textContent = step.meta;
        link.append(meta);
      }
      link.addEventListener("click", (ev) => {
        const id = step.href.replace(/^#/, "");
        const target = document.getElementById(id);
        if (!target) return;
        ev.preventDefault();
        target.scrollIntoView({ behavior: "smooth", block: "start" });
        history.replaceState(null, "", step.href);
      });
      item.append(link);
      root.append(item);
    });
  };

  const appendExternalIcon = (node) => {
    const icon = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    icon.setAttribute("class", "icon");
    icon.setAttribute("aria-hidden", "true");
    const use = document.createElementNS("http://www.w3.org/2000/svg", "use");
    use.setAttribute("href", "../media/icons.svg#i-external");
    icon.append(use);
    node.append(icon);
  };

  const renderJlcpcb = () => {
    const root = el("shop-jlcpcb");
    if (!root || !cart?.jlcpcb) return;
    const j = cart.jlcpcb;
    root.replaceChildren();

    const media = document.createElement("div");
    media.className = "shop-jlcpcb-media";
    const img = document.createElement("img");
    img.src = j.image;
    img.alt = "Tiny Engineer main control board";
    img.width = 480;
    img.height = 360;
    media.append(img);

    const body = document.createElement("div");
    body.className = "shop-jlcpcb-body";

    if (j.pathLabel) {
      const path = document.createElement("p");
      path.className = "shop-path-label mono";
      path.textContent = j.pathLabel;
      body.append(path);
    }

    const title = document.createElement("h2");
    title.className = "shop-section-title";
    title.textContent = j.title;
    body.append(title);

    if (j.disclaimer) {
      const disc = document.createElement("p");
      disc.className = "shop-jlcpcb-disclaimer";
      disc.textContent = j.disclaimer;
      body.append(disc);
    }

    const ol = document.createElement("ol");
    ol.className = "shop-jlcpcb-steps";
    (j.steps || []).forEach((step) => {
      const li = document.createElement("li");
      li.textContent = step;
      ol.append(li);
    });
    body.append(ol);

    const actions = document.createElement("div");
    actions.className = "shop-jlcpcb-actions";
    if (j.orderUrl) {
      const order = document.createElement("a");
      order.className = "btn btn-primary";
      order.href = j.orderUrl;
      order.target = "_blank";
      order.rel = "noopener noreferrer";
      order.append("Open JLCPCB");
      appendExternalIcon(order);
      actions.append(order);
    }
    if (j.fullGuideUrl) {
      const guide = document.createElement("a");
      guide.className = "btn";
      guide.href = j.fullGuideUrl;
      guide.target = "_blank";
      guide.rel = "noopener noreferrer";
      guide.append("Full order guide");
      appendExternalIcon(guide);
      actions.append(guide);
    }
    body.append(actions);

    root.append(media, body);
  };

  const renderLane = (lane) => {
    const section = document.createElement("section");
    section.className = "panel shop-lane";
    section.id = `shop-lane-${lane.id}`;
    section.dataset.lane = lane.id;
    if (lane.id === "now") section.classList.add("shop-lane--featured");

    const head = document.createElement("div");
    head.className = "shop-lane-head";

    const label = document.createElement("p");
    label.className = "panel-label";
    label.textContent = `${lane.label} · ${lane.title}`;

    const { done, total } = laneProgress(lane);
    const badge = document.createElement("span");
    badge.className = "shop-lane-progress mono";
    badge.dataset.laneProgress = lane.id;
    badge.textContent = progressLabel(done, total);
    badge.hidden = !total;
    badge.classList.toggle("is-complete", total > 0 && done >= total);
    head.append(label, badge);
    section.append(head);

    if (lane.lede) {
      const lede = document.createElement("p");
      lede.className = "shop-step-lede";
      lede.textContent = lane.lede;
      section.append(lede);
    }

    const list = document.createElement("ul");
    list.className = "shop-check-list";
    if (lane.items.length === 1) list.classList.add("shop-check-list--solo");

    lane.items.forEach((item) => {
      const li = document.createElement("li");
      const isDone = checked.has(item.id);
      const featured = lane.id === "now";
      li.className =
        "shop-item" +
        (isDone ? " is-done" : "") +
        (featured ? " shop-item--featured" : "");

      const box = document.createElement("input");
      box.type = "checkbox";
      box.id = `shop-check-${item.id}`;
      box.className = "shop-item-check";
      box.checked = isDone;
      box.title = "Mark as ordered / owned";

      const mark = document.createElement("span");
      mark.className = "shop-card-check";
      mark.setAttribute("aria-hidden", "true");
      mark.textContent = "✓";
      mark.hidden = !isDone;

      const syncDone = () => {
        if (box.checked) checked.add(item.id);
        else checked.delete(item.id);
        saveChecked(checked);
        li.classList.toggle("is-done", box.checked);
        mark.hidden = !box.checked;
        updateCheckProgress();
      };
      box.addEventListener("change", syncDone);

      const media = document.createElement("div");
      media.className = "shop-item-media";

      const thumb = document.createElement("img");
      thumb.className = "shop-item-photo";
      if (item.id === "servo") {
        const s = servoById(servoId);
        thumb.src = s?.image || item.image;
        thumb.alt = s?.label || "";
      } else {
        thumb.src = item.image;
        thumb.alt = itemLabel(item);
      }
      thumb.width = 480;
      thumb.height = 360;
      thumb.loading = "lazy";
      media.append(thumb, box, mark);

      const body = document.createElement("div");
      body.className = "shop-item-body";

      const head = document.createElement("div");
      head.className = "shop-item-head";

      const name = document.createElement("label");
      name.className = "shop-item-name";
      name.htmlFor = box.id;
      name.textContent = itemLabel(item);

      const qty = document.createElement("span");
      const n = itemQty(item);
      qty.className = "shop-item-qty mono";
      qty.title = `Quantity: ${n}`;
      qty.setAttribute("aria-label", `Quantity ${n}`);
      qty.textContent = `×${n}`;
      head.append(name, qty);
      body.append(head);

      const blurbText = item.blurb || item.notes || "";
      if (blurbText) {
        const notes = document.createElement("p");
        notes.className = "shop-item-notes";
        notes.textContent = blurbText;
        if (item.notes && item.blurb && item.notes !== item.blurb) {
          notes.title = item.notes;
        }
        body.append(notes);
      }

      const buyUrl =
        item.id === "servo" ? servoById(servoId)?.url || item.url : item.url;
      if (buyUrl && !item.checkOnly) {
        const buy = document.createElement("a");
        buy.className = "shop-item-buy";
        buy.href = buyUrl;
        buy.target = "_blank";
        buy.rel = "noopener noreferrer";
        buy.append("Buy");
        appendExternalIcon(buy);
        body.append(buy);
      }

      li.append(media, body);
      list.append(li);
    });

    section.append(list);
    return section;
  };

  const renderLanes = () => {
    const root = el("shop-lanes");
    if (!root || !cart) return;
    root.replaceChildren();
    cart.lanes.forEach((lane) => root.append(renderLane(lane)));
  };

  const renderAdvanced = () => {
    const link = el("shop-advanced-link");
    if (!link || !cart?.advancedBreakoutsDoc) return;
    link.href = cart.advancedBreakoutsDoc;
  };

  const render = () => {
    renderServos();
    renderJlcpcb();
    renderLanes();
    renderAdvanced();
    updateCheckProgress();
  };

  const clearChecks = () => {
    checked = new Set();
    saveChecked(checked);
    renderLanes();
    updateCheckProgress();
  };

  const clearBtn = el("shop-clear-checks");
  if (clearBtn) clearBtn.addEventListener("click", clearChecks);

  const boot = async () => {
    setStatus("Loading cart…", "loading");
    try {
      const res = await fetch("./cart.json", { cache: "no-cache" });
      if (!res.ok) throw new Error(`cart.json ${res.status}`);
      cart = await res.json();
      if (!cart?.servos?.length || !cart?.lanes?.length) {
        throw new Error("cart.json missing servos or lanes");
      }
      servoId = defaultServoId();
      write(SERVO_KEY, servoId);
      render();
      setStatus("", "ready");
      if (statusEl) statusEl.hidden = true;
    } catch (err) {
      console.error(err);
      setStatus("Could not load cart.json.", "error");
      if (statusEl) statusEl.hidden = false;
    }
  };

  boot();
})();
