/**
 * Build-process stage dots for GitHub Pages under web/.
 * Mount: <nav class="build-timeline" data-stage="…" data-root="…"></nav>
 * then <script src="…/js/build-timeline.js"></script> after it (build-progress.js
 * may sit between them).
 *
 * data-stage: id of the current stage (shop | print | connect | flash | assembly | setup | use | …)
 * data-root: relative prefix to web/ root ("../" on nested pages)
 *
 * Add stages to STAGES as the build guide grows. Mount on each stage page
 * with data-stage set to that page's id.
 */
(function () {
  const STAGES = [
    { id: "shop", label: "Shop", path: "shop/" },
    { id: "print", label: "Print", path: "print/" },
    { id: "connect", label: "Connect", path: "connect/" },
    { id: "flash", label: "Flash", path: "flash/" },
    { id: "assembly", label: "Assembly", path: "assembly/" },
    { id: "setup", label: "Setup", path: "setup/" },
    { id: "use", label: "Use", path: "use/" },
  ];

  let el = document.currentScript?.previousElementSibling;
  while (el && !el.classList?.contains("build-timeline")) {
    el = el.previousElementSibling;
  }
  if (!el) return;

  const current = el.dataset.stage || "";
  const root = el.dataset.root || "./";
  const currentIndex = STAGES.findIndex((stage) => stage.id === current);

  // Remember this stage so /build/ can resume here.
  if (currentIndex >= 0 && window.TinyEngineerBuild) {
    window.TinyEngineerBuild.saveStage(current);
  }

  el.setAttribute("aria-label", "Build stages");

  const list = document.createElement("ol");
  list.className = "build-timeline-list";

  STAGES.forEach((stage, index) => {
    const item = document.createElement("li");
    item.className = "build-timeline-item";
    if (index < currentIndex) item.classList.add("is-done");
    if (index === currentIndex) item.classList.add("is-current");
    if (index > currentIndex) item.classList.add("is-todo");

    const link = document.createElement("a");
    link.className = "build-timeline-link";
    link.href = index === currentIndex ? "./" : root + stage.path;
    if (index === currentIndex) link.setAttribute("aria-current", "step");

    const marker = document.createElement("span");
    marker.className = "build-timeline-dot";
    marker.setAttribute("aria-hidden", "true");

    const label = document.createElement("span");
    label.className = "build-timeline-label";
    label.textContent = stage.label;

    link.append(marker, label);
    item.append(link);
    list.append(item);
  });

  el.replaceChildren(list);
})();
