/**
 * Shared site header for GitHub Pages under web/.
 * Mount: <header class="site-header" data-page="…" data-root="…"></header>
 * then <script src="…/js/site-header.js"></script> immediately after.
 *
 * data-page: home | build | shop | print | flash | assembly | setup | customise | screens
 * data-root: relative prefix to web/ root ("./" on home, "../" on nested pages)
 *
 * Top-nav: Build · Customise · GitHub.
 * Build-stage pages (shop…setup) are not top-nav links — use the stage timeline.
 * Top-nav Build → /build/, which redirects to /shop/.
 * Customise → /customise/ (tile hub). Screens editor also highlights Customise.
 */
(function () {
  const REPO = "https://github.com/jamro/tiny-engineer";
  const BUILD_PAGES = new Set([
    "build", "shop", "print", "flash", "assembly", "setup",
  ]);
  const CUSTOMISE_PAGES = new Set(["customise", "screens"]);

  const LINKS = [
    { id: "build", label: "Build", path: "build/" },
    { id: "customise", label: "Customise", path: "customise/" },
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
    const current =
      link.id === "build"
        ? BUILD_PAGES.has(page)
        : link.id === "customise"
          ? CUSTOMISE_PAGES.has(page)
          : page === link.id;
    if (current) a.setAttribute("aria-current", "page");
    nav.appendChild(a);
  }

  el.replaceChildren(brand, nav);
})();
