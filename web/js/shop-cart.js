/**
 * Shop stage cart: load cart.json, Elecrow board card, checklist + localStorage.
 * Servo preset (SG90 default) lives on the plug-in modules servo row.
 * Mount: empty nodes with ids shop-* in web/build/shop/index.html, then this script.
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

  const servoBlurb = (item) => {
    const s = servoById(servoId);
    if (!s) return item.blurb || item.notes || "";
    const size = s.size ? `Robot ${s.size}. ` : "";
    const tag =
      s.group === "recommended" || s.default
        ? "Standard build. "
        : "Compact build. ";
    return `${tag}${size}Five identical — never mix.`;
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

  /** Select a servo preset and refresh lanes (print stage reads the same key). */
  const selectServo = (id) => {
    if (!servoById(id)) return;
    if (servoId !== id) {
      servoId = id;
      write(SERVO_KEY, servoId);
    }
    render();
  };

  /** Compact / standard switcher on the plug-in modules servo checklist row. */
  const appendServoModelControl = (actions) => {
    const wrap = document.createElement("label");
    wrap.className = "shop-servo-model";
    const caption = document.createElement("span");
    caption.className = "shop-servo-model-label mono";
    caption.textContent = "Model";
    const select = document.createElement("select");
    select.className = "shop-servo-model-select";
    select.setAttribute("aria-label", "Servo model — sets whole robot size");
    (cart.servos || []).forEach((s) => {
      if (s.id === "hd1370a") return;
      const opt = document.createElement("option");
      opt.value = s.id;
      const kind =
        s.group === "recommended" || s.default ? "standard" : "compact";
      opt.textContent = `${s.short || s.label} (${kind})`;
      if (s.id === servoId) opt.selected = true;
      select.append(opt);
    });
    select.addEventListener("change", () => {
      selectServo(select.value);
    });
    wrap.append(caption, select);
    actions.append(wrap);
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
      ". Buy the board when you are ready.";
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
        label: "4",
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
    use.setAttribute("href", `../../media/icons.svg#${symbolId}`);
    icon.append(use);
    if (prepend) node.prepend(icon);
    else node.append(icon);
  };

  const appendExternalIcon = (node) => {
    appendSpriteIcon(node, "i-external");
  };

  const boardLane = () => cart?.lanes?.find((l) => l.id === "now");
  const boardItem = () => boardLane()?.items?.[0] || null;

  const renderBoard = () => {
    const root = el("shop-jlcpcb");
    const panel = el("shop-step-board");
    if (!root || !cart?.board) return;
    const board = cart.board;
    const item = boardItem();
    const isDone = item ? checked.has(item.id) : false;
    root.replaceChildren();
    if (panel) panel.classList.toggle("is-done", isDone);

    const media = document.createElement("div");
    media.className = "shop-jlcpcb-media";
    const img = document.createElement("img");
    img.src = board.image;
    img.alt = "Tiny Engineer main control board";
    img.width = 800;
    img.height = 640;
    bindImageFallback(img, "Main control board");
    media.append(img);

    if (item) {
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
        mark.hidden = !box.checked;
        if (panel) panel.classList.toggle("is-done", box.checked);
        updateCheckProgress();
      };
      box.addEventListener("change", syncDone);

      const checkHit = document.createElement("label");
      checkHit.className = "shop-item-check-hit";
      checkHit.htmlFor = box.id;
      checkHit.title = "Mark as ordered or owned";
      checkHit.append(box, mark);
      media.append(checkHit);

      const qty = document.createElement("span");
      const n = itemQty(item);
      qty.className = "shop-item-qty shop-jlcpcb-qty mono";
      qty.title = `Quantity: ${n}`;
      qty.setAttribute("aria-label", `Quantity ${n}`);
      qty.textContent = `×${n}`;
      media.append(qty);
    }

    const body = document.createElement("div");
    body.className = "shop-jlcpcb-body";

    if (board.pathLabel) {
      const path = document.createElement("p");
      path.className = "shop-path-label mono";
      path.textContent = board.pathLabel;
      body.append(path);
    }

    const titleRow = document.createElement("div");
    titleRow.className = "shop-jlcpcb-title-row";
    const title = document.createElement("h2");
    title.className = "shop-section-title";
    title.textContent = board.title;
    titleRow.append(title);
    body.append(titleRow);

    if (board.blurb) {
      const disc = document.createElement("p");
      disc.className = "shop-jlcpcb-disclaimer";
      disc.textContent = board.blurb;
      body.append(disc);
    }

    if (board.buyUrl) {
      const actions = document.createElement("div");
      actions.className = "shop-jlcpcb-actions";
      const buy = document.createElement("a");
      buy.className = "btn btn-primary";
      buy.href = board.buyUrl;
      buy.target = "_blank";
      buy.rel = "noopener noreferrer";
      buy.append(board.buyLabel || "Buy on Elecrow");
      appendExternalIcon(buy);
      actions.append(buy);
      body.append(actions);
    }

    root.append(media, body);
  };

  const renderLane = (lane) => {
    const section = document.createElement("section");
    section.className = "panel shop-lane";
    section.id = `shop-lane-${lane.id}`;
    section.dataset.lane = lane.id;

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
      li.className = "shop-item" + (isDone ? " is-done" : "");

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

      const blurbText =
        item.id === "servo" ? servoBlurb(item) : item.blurb || item.notes || "";
      if (blurbText) {
        const notes = document.createElement("p");
        notes.className = "shop-item-notes";
        notes.textContent = blurbText;
        if (
          item.id !== "servo" &&
          item.notes &&
          item.blurb &&
          item.notes !== item.blurb
        ) {
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
        buy.append(item.id === "servo" ? "Buy ×5" : "Buy");
        appendExternalIcon(buy);
        actions.append(buy);
      }

      if (item.id === "servo") {
        appendServoModelControl(actions);
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
    // Board lane lives inside #shop-step-board, not a second panel.
    cart.lanes
      .filter((lane) => lane.id !== "now")
      .forEach((lane) => root.append(renderLane(lane)));
  };

  const renderAdvanced = () => {
    const link = el("shop-advanced-link");
    if (!link || !cart?.advancedBreakoutsDoc) return;
    link.href = cart.advancedBreakoutsDoc;
  };

  const render = () => {
    renderBoard();
    renderLanes();
    renderAdvanced();
    updateCheckProgress();
  };

  /** Deep links (#shop-lane-next) land after async cart render — browser hash scroll is too early. */
  const scrollToHash = () => {
    const id = (location.hash || "").replace(/^#/, "");
    if (!id) return;
    const target = document.getElementById(id);
    if (!target) return;
    target.scrollIntoView({ behavior: "auto", block: "start" });
  };

  const clearChecks = () => {
    checked = new Set();
    saveChecked(checked, CHECKED_KEY);
    renderBoard();
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
      requestAnimationFrame(scrollToHash);
    } catch (err) {
      console.error(err);
      setStatus("Could not load cart.json.", "error");
      if (statusEl) statusEl.hidden = false;
    }
  };

  boot();
})();
