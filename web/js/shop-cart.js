/**
 * Shop stage cart: load cart.json, servo pick, JLCPCB lane, checklist + localStorage.
 * Mount: empty nodes with ids shop-* in web/shop/index.html, then this script.
 */
import { read, write, loadChecked, saveChecked } from "./storage.js";

(function () {
  const SERVO_KEY = "tiny-engineer.shop.servo";
  const CHECKED_KEY = "tiny-engineer.shop.checked";

  const el = (id) => document.getElementById(id);

  const statusEl = el("shop-status");
  const setStatus = (text, state) => {
    if (!statusEl) return;
    statusEl.textContent = text;
    if (state) statusEl.dataset.state = state;
  };

  let cart = null;
  let servoId = null;
  let checked = loadChecked(CHECKED_KEY);

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

  const bindImageFallback = (img, altText) => {
    img.addEventListener(
      "error",
      () => {
        img.removeAttribute("src");
        img.alt = altText || "Image unavailable";
        img.classList.add("shop-img-fallback");
      },
      { once: true }
    );
  };

  const servoRadios = (root) =>
    Array.from(root.querySelectorAll('[role="radio"]'));

  const focusServoRadio = (root, id) => {
    const btn = root.querySelector(`[role="radio"][data-servo="${CSS.escape(id)}"]`);
    btn?.focus();
  };

  /** Select a servo preset; re-render and keep keyboard focus on the active radio. */
  const selectServo = (id, { focus = true } = {}) => {
    if (servoId !== id) {
      servoId = id;
      write(SERVO_KEY, servoId);
    }
    render({ restoreServoFocus: focus });
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
    // Roving tabindex: one tab stop for the radiogroup (selected / checked).
    btn.tabIndex = selected ? 0 : -1;
    btn.dataset.servo = s.id;
    if (s.size) {
      btn.title = `${s.label} · robot ~${s.size.replace(/^~/, "")}`;
    }

    const media = document.createElement("span");
    media.className = "shop-servo-card-media";
    const img = document.createElement("img");
    img.src = s.image;
    img.alt = s.label || s.short || "Servo";
    img.width = 800;
    img.height = 640;
    img.loading = "lazy";
    img.decoding = "async";
    bindImageFallback(img, s.label || s.short || "Servo");
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
      selectServo(s.id, { focus: true });
    });
    btn.addEventListener("keydown", (ev) => {
      const root = el("shop-servo-picker");
      if (!root) return;
      const radios = servoRadios(root);
      const i = radios.indexOf(ev.currentTarget);
      if (i < 0) return;
      let next = -1;
      if (ev.key === "ArrowRight" || ev.key === "ArrowDown") {
        next = (i + 1) % radios.length;
      } else if (ev.key === "ArrowLeft" || ev.key === "ArrowUp") {
        next = (i - 1 + radios.length) % radios.length;
      } else if (ev.key === "Home") {
        next = 0;
      } else if (ev.key === "End") {
        next = radios.length - 1;
      } else {
        return;
      }
      ev.preventDefault();
      const id = radios[next]?.dataset?.servo;
      if (id) selectServo(id, { focus: true });
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

  const renderServos = ({ restoreServoFocus = false } = {}) => {
    const root = el("shop-servo-picker");
    const status = el("shop-servo-status");
    if (!root || !cart) return;

    const hadFocus = root.contains(document.activeElement);

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

    // If nothing is checked yet, keep one tab stop on the first radio.
    const radios = servoRadios(root);
    if (radios.length && !radios.some((r) => r.tabIndex === 0)) {
      radios[0].tabIndex = 0;
    }

    if ((restoreServoFocus || hadFocus) && servoId) {
      focusServoRadio(root, servoId);
    }

    const s = servoById(servoId);
    if (status && s) {
      const qty = s.qty ?? 5;
      status.replaceChildren();
      const lead = document.createElement("span");
      lead.className = "shop-servo-outcome-lead";
      lead.textContent = "Your build";
      const detail = document.createElement("span");
      detail.className = "shop-servo-outcome-detail";
      detail.textContent =
        `${s.label || s.short}` +
        (s.size ? ` · robot ${s.size}` : "") +
        ` · buy ×${qty} identical · later print the matching parts set`;
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

  const updateContinueWarn = () => {
    const warn = el("shop-continue-warn");
    if (!warn || !cart) return;
    const { done, total } = allCheckProgress();
    if (!total || done >= total) {
      warn.hidden = true;
      warn.textContent = "";
      return;
    }
    const board = cart.lanes?.find((l) => l.id === "now");
    const boardDone = board ? laneProgress(board).done >= laneProgress(board).total : true;
    const bits = [];
    if (!boardDone) bits.push("board order still unmarked");
    bits.push(`${total - done} shopping check${total - done === 1 ? "" : "s"} left`);
    warn.textContent =
      "You can still continue — " +
      bits.join("; ") +
      ". Printing can wait; the board often takes weeks.";
    warn.hidden = false;
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
    updateContinueWarn();
  };

  const renderTimeline = () => {
    const root = el("shop-order-timeline");
    if (!root || !cart) return;
    root.replaceChildren();
    const { done: allDone, total: allTotal } = allCheckProgress();
    const steps = [
      {
        label: "1",
        title: "Choose a servo",
        meta: "✓ chosen",
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

  const appendSpriteIcon = (node, symbolId, { prepend = false } = {}) => {
    if (!symbolId) return;
    const icon = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    icon.setAttribute("class", "icon");
    icon.setAttribute("aria-hidden", "true");
    const use = document.createElementNS("http://www.w3.org/2000/svg", "use");
    use.setAttribute("href", `../media/icons.svg#${symbolId}`);
    icon.append(use);
    if (prepend) node.prepend(icon);
    else node.append(icon);
  };

  const appendExternalIcon = (node) => {
    appendSpriteIcon(node, "i-external");
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
    img.width = 800;
    img.height = 640;
    bindImageFallback(img, "Main control board");
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

    if (Array.isArray(j.downloads) && j.downloads.length) {
      const dl = document.createElement("div");
      dl.className = "shop-jlcpcb-downloads";
      const dlLabel = document.createElement("p");
      dlLabel.className = "shop-jlcpcb-downloads-label mono";
      dlLabel.textContent = "Download these three files";
      dl.append(dlLabel);
      const dlRow = document.createElement("div");
      dlRow.className = "shop-jlcpcb-download-row";
      j.downloads.forEach((item) => {
        if (!item?.url || !item?.label) return;
        const a = document.createElement("a");
        a.className = "btn shop-jlcpcb-download";
        a.href = item.url;
        const fileName =
          item.filename ||
          String(item.url).split("/").pop() ||
          item.label;
        a.setAttribute("download", fileName);
        if (item.tech) a.title = item.tech;
        if (item.icon) appendSpriteIcon(a, item.icon, { prepend: true });
        a.append(document.createTextNode(item.label));
        dlRow.append(a);
      });
      dl.append(dlRow);
      body.append(dl);
    }

    const downloadByLabel = new Map();
    (j.downloads || []).forEach((item) => {
      if (item?.label && item?.url) downloadByLabel.set(String(item.label), item);
    });
    const linkLabels = [...downloadByLabel.keys()].sort((a, b) => b.length - a.length);
    if (j.orderUrl) linkLabels.push("jlcpcb.com");

    const appendStepWithFabLinks = (li, text) => {
      if (!linkLabels.length) {
        li.textContent = text;
        return;
      }
      const escaped = linkLabels.map((l) =>
        l.replace(/[.*+?^${}()|[\]\\]/g, "\\$&")
      );
      const re = new RegExp(`(${escaped.join("|")})`, "g");
      const parts = String(text).split(re);
      parts.forEach((part) => {
        if (part === "jlcpcb.com" && j.orderUrl) {
          const a = document.createElement("a");
          a.className = "shop-jlcpcb-step-file";
          a.href = j.orderUrl;
          a.target = "_blank";
          a.rel = "noopener noreferrer";
          a.textContent = part;
          li.append(a);
          return;
        }
        const item = downloadByLabel.get(part);
        if (!item) {
          if (part) li.append(document.createTextNode(part));
          return;
        }
        const a = document.createElement("a");
        a.className = "shop-jlcpcb-step-file";
        a.href = item.url;
        const fileName =
          item.filename || String(item.url).split("/").pop() || item.label;
        a.setAttribute("download", fileName);
        a.textContent = item.label;
        li.append(a);
      });
    };

    const ol = document.createElement("ol");
    ol.className = "shop-jlcpcb-steps";
    (j.steps || []).forEach((step) => {
      const li = document.createElement("li");
      appendStepWithFabLinks(li, step);
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
      box.setAttribute(
        "aria-label",
        `Mark ${itemLabel(item)} as ordered or owned`
      );
      box.title = "Mark as ordered or owned";

      const mark = document.createElement("span");
      mark.className = "shop-card-check";
      mark.setAttribute("aria-hidden", "true");
      mark.textContent = "✓";
      mark.hidden = !isDone;

      const syncDone = () => {
        if (box.checked) checked.add(item.id);
        else checked.delete(item.id);
        saveChecked(checked, CHECKED_KEY);
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
        thumb.src = s?.thumb || s?.image || item.thumb || item.image;
        thumb.alt = s?.label || "";
      } else {
        thumb.src = item.thumb || item.image;
        thumb.alt = itemLabel(item);
      }
      thumb.width = 400;
      thumb.height = 320;
      thumb.loading = "lazy";
      thumb.decoding = "async";
      bindImageFallback(thumb, itemLabel(item));

      const checkHit = document.createElement("label");
      checkHit.className = "shop-item-check-hit";
      checkHit.htmlFor = box.id;
      checkHit.title = "Mark as ordered or owned";
      checkHit.append(box, mark);

      media.append(thumb, checkHit);

      const body = document.createElement("div");
      body.className = "shop-item-body";

      const itemHead = document.createElement("div");
      itemHead.className = "shop-item-head";

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
      itemHead.append(name, qty);
      body.append(itemHead);

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

      const actions = document.createElement("div");
      actions.className = "shop-item-actions";

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
        actions.append(buy);
      }

      if (item.tips) {
        const tipsBtn = document.createElement("button");
        tipsBtn.type = "button";
        tipsBtn.className = "btn shop-item-tips-btn";
        tipsBtn.textContent = "Tips";
        tipsBtn.setAttribute("aria-expanded", "false");
        const tipsPanel = document.createElement("p");
        tipsPanel.className = "shop-item-tips";
        tipsPanel.id = `shop-tips-${item.id}`;
        tipsPanel.hidden = true;
        tipsPanel.textContent = item.tips;
        tipsBtn.setAttribute("aria-controls", tipsPanel.id);
        const toggleTips = (ev) => {
          ev?.preventDefault?.();
          const open = tipsPanel.hasAttribute("hidden");
          if (open) tipsPanel.removeAttribute("hidden");
          else tipsPanel.setAttribute("hidden", "");
          tipsBtn.setAttribute("aria-expanded", open ? "true" : "false");
        };
        tipsBtn.addEventListener("click", toggleTips);
        actions.append(tipsBtn);
        body.append(actions, tipsPanel);
      } else if (actions.childNodes.length) {
        body.append(actions);
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

  const render = ({ restoreServoFocus = false } = {}) => {
    renderServos({ restoreServoFocus });
    renderJlcpcb();
    renderLanes();
    renderAdvanced();
    updateCheckProgress();
  };

  const clearChecks = () => {
    checked = new Set();
    saveChecked(checked, CHECKED_KEY);
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
