# Vendored browser libraries

Pinned copies served by GitHub Pages (no CDN).

| Package | Version | Path |
| --- | --- | --- |
| three | 0.186.0 | `three/` |
| esp-web-tools | 10.4.0 | `esp-web-tools/` |

Regenerate from the repo root:

```bash
./scripts/web/vendor-libs.sh
```

Override pins with `THREE_VER` / `EWT_VER` if needed, then update HTML import maps only when the three major/API changes.
