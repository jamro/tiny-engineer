/**
 * Copy buttons: .setup-copy[data-copy-target] → clipboard + icon flash.
 * Icon path taken from existing <use href="…#i-copy"> (hash swapped).
 */
(function () {
  const FAIL_MSG = "Copy failed — select the command and copy manually.";

  function showCopyFailed(button, field) {
    field?.focus();
    field?.select();
    const row = button.closest(".setup-curl-row") || button.parentElement;
    const host = row?.parentElement || button.parentElement;
    if (!host) {
      return;
    }
    let note = host.querySelector(".setup-copy-fail");
    if (!note) {
      note = document.createElement("p");
      note.className = "setup-copy-fail mono";
      note.setAttribute("role", "status");
      if (row?.nextSibling) {
        host.insertBefore(note, row.nextSibling);
      } else {
        host.appendChild(note);
      }
    }
    note.textContent = FAIL_MSG;
    button.setAttribute("aria-label", FAIL_MSG);
    window.setTimeout(() => {
      if (note.textContent === FAIL_MSG) {
        note.textContent = "";
      }
    }, 4000);
  }

  document.querySelectorAll(".setup-copy").forEach((button) => {
    const icon = button.querySelector("use");
    const defaultLabel = button.getAttribute("aria-label") || "Copy";
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
        if (!ok) {
          showCopyFailed(button, field);
        }
        return;
      }
      button.setAttribute("aria-label", defaultLabel);
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
