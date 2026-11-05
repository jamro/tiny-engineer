/**
 * Shared site header for GitHub Pages under web/.
 * Mount: <header class="site-header" data-page="…" data-root="…"></header>
 * then <script src="…/js/site-header.js"></script> immediately after.
 *
 * data-page: home | flash | screens | assembly
 * data-root: relative prefix to web/ root ("./" on home, "../" on nested pages)
 */
(function () {
  const REPO = "https://github.com/jamro/tiny-engineer";
  const BUILD =
    REPO + "/blob/main/docs/getting-started.md";

  const LINKS = [
    { id: "build", label: "Build", href: BUILD },
    { id: "flash", label: "Flash", path: "flash/" },
    { id: "screens", label: "Screens", path: "screens/" },
    { id: "assembly", label: "Assembly", path: "assembly/" },
    { id: "github", label: "GitHub", href: REPO },
  ];

  const el = document.currentScript?.previousElementSibling;
  if (!el || !el.classList.contains("site-header")) return;

  const page = el.dataset.page || "";
  const root = el.dataset.root || "./";

  function resolve(path, id) {
    if (page === id) return "./";
    return root + path;
  }

  const brand = document.createElement("a");
  brand.className = "brand";
  brand.href = page === "home" ? "./" : root;
  if (page === "home") brand.setAttribute("aria-current", "page");
  brand.innerHTML =
    '<span class="brand-mark" aria-hidden="true"></span> Tiny Engineer';

  const nav = document.createElement("nav");
  nav.className = "nav";
  nav.setAttribute("aria-label", "Site");

  for (const link of LINKS) {
    const a = document.createElement("a");
    a.textContent = link.label;
    a.href = link.href || resolve(link.path, link.id);
    if (page === link.id) a.setAttribute("aria-current", "page");
    nav.appendChild(a);
  }

  el.replaceChildren(brand, nav);
})();
