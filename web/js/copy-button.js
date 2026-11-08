/**
 * Copy buttons: .setup-copy[data-copy-target] → clipboard + icon flash.
 * Icon path taken from existing <use href="…#i-copy"> (hash swapped).
 */
(function () {
  document.querySelectorAll(".setup-copy").forEach((button) => {
    const icon = button.querySelector("use");
    button.addEventListener("click", async () => {
      const field = document.getElementById(button.dataset.copyTarget);
      const text = field?.value ?? "";
      let ok = true;
      try {
        await navigator.clipboard.writeText(text);
      } catch {
        field?.focus();
        field?.select();
        ok = document.execCommand("copy");
      }
      if (!icon || !ok) {
        return;
      }
      const href = icon.getAttribute("href") || "";
      const base = href.replace(/#.*$/, "");
      icon.setAttribute("href", `${base}#i-check`);
      button.classList.add("is-copied");
      window.setTimeout(() => {
        icon.setAttribute("href", `${base}#i-copy`);
        button.classList.remove("is-copied");
      }, 1200);
    });
  });
})();
