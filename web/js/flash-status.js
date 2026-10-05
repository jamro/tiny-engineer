(() => {
  const statusEl = document.getElementById("flash-status");
  const ledEl = document.getElementById("flash-led");
  const versionEl = document.getElementById("flash-version");
  const selectEl = document.getElementById("flash-release");
  const partsEl = document.getElementById("flash-parts");
  const manifestUrlEl = document.getElementById("flash-manifest-url");
  const installWrap = document.getElementById("flash-install");

  let releases = [];
  let blobUrl = null;
  let loadId = 0;

  /** Directory URL for this page (handles missing trailing slash / index.html). */
  function pageDir() {
    let href = window.location.href.split("#")[0].split("?")[0];
    if (/\/index\.html$/i.test(href)) {
      href = href.replace(/\/index\.html$/i, "/");
    } else if (!href.endsWith("/")) {
      href += "/";
    }
    return href;
  }

  function setStatus(kind, text) {
    if (!statusEl) return;
    statusEl.textContent = text;
    statusEl.classList.remove("is-ok", "is-err");
    if (kind === "ok") statusEl.classList.add("is-ok");
    if (kind === "err") statusEl.classList.add("is-err");
    if (ledEl) {
      ledEl.classList.remove("led-ok", "led-warn", "led-err");
      if (kind === "ok") ledEl.classList.add("led-ok");
      else if (kind === "err") ledEl.classList.add("led-err");
      else ledEl.classList.add("led-warn");
    }
  }

  function hideInstall() {
    if (installWrap) installWrap.hidden = true;
  }

  function showInstall() {
    if (installWrap) installWrap.hidden = false;
  }

  function formatOffset(offset) {
    const n = Number(offset);
    if (!Number.isFinite(n)) return String(offset);
    return "0x" + n.toString(16).toUpperCase();
  }

  function partLabel(file) {
    return String(file || "")
      .replace(/^tiny-engineer-v[\w.-]+-/, "")
      .replace(/\.bin$/i, "");
  }

  function clearParts(message) {
    if (!partsEl) return;
    partsEl.innerHTML = "";
    const tr = document.createElement("tr");
    const td = document.createElement("td");
    td.colSpan = 2;
    td.className = "muted";
    td.textContent = message || "Select a release…";
    tr.appendChild(td);
    partsEl.appendChild(tr);
  }

  function renderParts(parts) {
    if (!partsEl) return;
    if (!Array.isArray(parts) || !parts.length) {
      clearParts("Manifest has no parts.");
      return;
    }
    partsEl.innerHTML = "";
    parts.forEach((part) => {
      const tr = document.createElement("tr");
      const nameTd = document.createElement("td");
      const offTd = document.createElement("td");
      nameTd.textContent = partLabel(part.file);
      offTd.textContent = formatOffset(part.offset);
      tr.appendChild(nameTd);
      tr.appendChild(offTd);
      partsEl.appendChild(tr);
    });
  }

  function revokeBlob() {
    if (blobUrl) {
      URL.revokeObjectURL(blobUrl);
      blobUrl = null;
    }
  }

  function buildManifest(release) {
    const base = new URL("firmware/" + release.tag + "/", pageDir());
    return {
      name: "Tiny Engineer",
      version: release.version || release.tag,
      new_install_prompt_erase: true,
      builds: [
        {
          chipFamily: "ESP32-C3",
          parts: (release.parts || []).map((part) => ({
            path: new URL(part.file, base).href,
            offset: part.offset,
          })),
        },
      ],
    };
  }

  function remountInstallButton(manifestObjectUrl) {
    if (!installWrap) return;
    installWrap.innerHTML = "";
    const btn = document.createElement("esp-web-install-button");
    btn.setAttribute("manifest", manifestObjectUrl);
    const activate = document.createElement("button");
    activate.setAttribute("slot", "activate");
    activate.className = "btn btn-primary";
    activate.textContent = "Connect & install";
    btn.appendChild(activate);
    installWrap.appendChild(btn);
  }

  function selectedRelease() {
    if (!selectEl || selectEl.selectedIndex < 0) return null;
    const tag = selectEl.value;
    return releases.find((r) => r.tag === tag) || null;
  }

  function loadSelectedRelease() {
    const requestId = ++loadId;
    const release = selectedRelease();
    revokeBlob();
    hideInstall();

    if (!release) {
      clearParts("Select a release…");
      if (manifestUrlEl) manifestUrlEl.textContent = "—";
      if (versionEl) versionEl.textContent = "—";
      return;
    }

    if (manifestUrlEl) {
      manifestUrlEl.textContent = "firmware/" + release.tag + "/ (Pages mirror)";
      manifestUrlEl.title = new URL(
        "firmware/" + release.tag + "/",
        pageDir()
      ).href;
    }

    if (versionEl) versionEl.textContent = release.version || release.tag;
    renderParts(release.parts);
    const manifest = buildManifest(release);
    blobUrl = URL.createObjectURL(
      new Blob([JSON.stringify(manifest)], { type: "application/json" })
    );
    remountInstallButton(blobUrl);
    if (requestId !== loadId) return;
    showInstall();
    setStatus(
      "ok",
      "Release " +
        (release.version || release.tag) +
        " ready. Plug in the C3, then install."
    );
  }

  function populateSelect(list) {
    if (!selectEl) return;
    selectEl.innerHTML = "";
    list.forEach((rel, index) => {
      const opt = document.createElement("option");
      opt.value = rel.tag;
      opt.textContent = index === 0 ? rel.tag + " (latest)" : rel.tag;
      selectEl.appendChild(opt);
    });
    selectEl.disabled = false;
  }

  if (!window.isSecureContext || !navigator.serial) {
    setStatus(
      "err",
      "Web Serial needs Chrome or Edge over HTTPS. Safari and Firefox will not cut it."
    );
    hideInstall();
    if (selectEl) {
      selectEl.innerHTML = "";
      const opt = document.createElement("option");
      opt.value = "";
      opt.textContent = "Web Serial unavailable";
      selectEl.appendChild(opt);
      selectEl.disabled = true;
    }
    clearParts("Web Serial unavailable");
    return;
  }

  setStatus("warn", "Loading flashable releases…");

  fetch(new URL("releases.json", pageDir()).href, {
    method: "GET",
    cache: "no-store",
  })
    .then((res) => {
      if (!res.ok) throw new Error("releases.json " + res.status);
      return res.json();
    })
    .then((list) => {
      releases = Array.isArray(list) ? list : [];
      if (!releases.length) {
        if (selectEl) {
          selectEl.innerHTML = "";
          const opt = document.createElement("option");
          opt.value = "";
          opt.textContent = "No flashable releases";
          selectEl.appendChild(opt);
          selectEl.disabled = true;
        }
        if (versionEl) versionEl.textContent = "—";
        if (manifestUrlEl) manifestUrlEl.textContent = "—";
        clearParts("No flashable releases yet");
        hideInstall();
        setStatus(
          "err",
          "No flashable releases in the Pages catalog yet. Upload manifest.json + bins to a Release, then redeploy Pages."
        );
        return;
      }

      populateSelect(releases);
      if (selectEl) {
        selectEl.addEventListener("change", loadSelectedRelease);
      }
      loadSelectedRelease();
    })
    .catch(() => {
      if (selectEl) {
        selectEl.innerHTML = "";
        const opt = document.createElement("option");
        opt.value = "";
        opt.textContent = "Catalog missing";
        selectEl.appendChild(opt);
        selectEl.disabled = true;
      }
      if (versionEl) versionEl.textContent = "—";
      clearParts("Could not load release catalog");
      hideInstall();
      setStatus(
        "err",
        "Could not load releases.json. Redeploy GitHub Pages after a Release has manifest.json."
      );
    });
})();
