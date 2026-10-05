#!/usr/bin/env python3
"""Build same-origin flash catalog for GitHub Pages (avoids Release-asset CORS).

Writes:
  web/flash/releases.json
  web/flash/firmware/<tag>/*.bin

Run from repo root. Used by .github/workflows/pages.yml.
"""

from __future__ import annotations

import json
import re
import ssl
import sys
import urllib.error
import urllib.request
from pathlib import Path

OWNER = "jamro"
REPO = "tiny-engineer"
API = f"https://api.github.com/repos/{OWNER}/{REPO}/releases?per_page=30"
UA = "tiny-engineer-pages-catalog"
ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "web" / "flash"
FIRMWARE_DIR = OUT_DIR / "firmware"
RELEASES_JSON = OUT_DIR / "releases.json"
TAG_RE = re.compile(r"^v\d+")


def api_get(url: str) -> bytes:
    req = urllib.request.Request(
        url,
        headers={
            "Accept": "application/vnd.github+json",
            "User-Agent": UA,
        },
    )
    with urllib.request.urlopen(req, context=ssl.create_default_context()) as res:
        return res.read()


def download(url: str, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    req = urllib.request.Request(url, headers={"User-Agent": UA})
    with urllib.request.urlopen(req, context=ssl.create_default_context()) as res:
        dest.write_bytes(res.read())


def basename(url: str) -> str:
    return url.split("?")[0].rstrip("/").split("/")[-1]


def main() -> int:
    try:
        releases = json.loads(api_get(API).decode("utf-8"))
    except (urllib.error.URLError, urllib.error.HTTPError, json.JSONDecodeError) as exc:
        print(f"Failed to list releases: {exc}", file=sys.stderr)
        return 1

    catalog = []
    for rel in releases:
        if not isinstance(rel, dict):
            continue
        if rel.get("draft") or rel.get("prerelease"):
            continue
        tag = rel.get("tag_name") or ""
        if not TAG_RE.match(tag):
            continue
        assets = {a["name"]: a for a in rel.get("assets") or [] if "name" in a}
        man_asset = assets.get("manifest.json")
        if not man_asset:
            continue

        man_url = man_asset.get("browser_download_url")
        if not man_url:
            continue

        tag_dir = FIRMWARE_DIR / tag
        try:
            manifest = json.loads(
                urllib.request.urlopen(
                    urllib.request.Request(man_url, headers={"User-Agent": UA}),
                    context=ssl.create_default_context(),
                ).read()
            )
        except (urllib.error.URLError, urllib.error.HTTPError, json.JSONDecodeError) as exc:
            print(f"Skip {tag}: manifest download failed: {exc}", file=sys.stderr)
            continue

        builds = manifest.get("builds") or []
        if not builds or not builds[0].get("parts"):
            print(f"Skip {tag}: manifest has no parts", file=sys.stderr)
            continue

        parts_out = []
        ok = True
        for part in builds[0]["parts"]:
            path = part.get("path") or ""
            offset = part.get("offset")
            name = basename(path)
            if not name or offset is None:
                ok = False
                break
            dest = tag_dir / name
            try:
                print(f"Download {tag}/{name}")
                download(path, dest)
            except (urllib.error.URLError, urllib.error.HTTPError) as exc:
                print(f"Skip {tag}: failed {name}: {exc}", file=sys.stderr)
                ok = False
                break
            parts_out.append({"file": name, "offset": int(offset)})

        if not ok or not parts_out:
            continue

        # Keep a copy of the release manifest next to bins (debug / mirror).
        (tag_dir / "manifest.json").write_text(
            json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
        )

        catalog.append(
            {
                "tag": tag,
                "version": manifest.get("version") or tag.lstrip("v"),
                "parts": parts_out,
            }
        )
        print(f"Added {tag} ({len(parts_out)} parts)")

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    RELEASES_JSON.write_text(json.dumps(catalog, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {RELEASES_JSON} ({len(catalog)} releases)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
