# Tiny Engineer UI Style Guide

Source of truth for anything under [`web/`](.). Engineering markdown docs stay in [`docs/`](../docs/); this guide is for the public site (GitHub Pages).

When changing the site:

1. Follow this file.
2. Reuse tokens and components in [`css/tokens.css`](css/tokens.css) and [`css/base.css`](css/base.css).
3. Prefer extending existing panel / button / status patterns over inventing a new look per page.

---

## Design Intent

The Tiny Engineer website should feel like a **small physical engineering product**, not a generic SaaS or AI landing page.

The visual identity should combine:

- developer tooling
- electronics / robotics
- maker culture
- industrial product design
- subtle humor
- a slightly sarcastic AI teammate personality

A useful mental model:

> **terminal + robotics lab + indie developer tool + physical desk gadget**

The UI should feel **engineered, not over-designed**.

---

## Core Principles

### 1. Physical over abstract

Tiny Engineer is a real robot, so the interface should feel tactile and grounded.

Prefer:

- PCB-inspired details
- LEDs
- physical control metaphors
- hardware status indicators
- terminal output
- pin labels
- servo / audio / Wi-Fi metadata
- subtle industrial styling

Avoid:

- generic AI gradients
- glowing abstract blobs
- excessive glassmorphism
- futuristic “AI magic” visuals

### 2. Technical, but friendly

The project should look like a serious developer tool with personality.

It should feel:

- precise
- compact
- functional
- playful in small doses
- approachable

It should **not** feel childish or toy-like.

Humor belongs mostly in:

- status messages
- empty states
- diagnostics
- microcopy

Do not sacrifice clarity for jokes.

---

## Visual Style

### Color palette

Use a mostly dark interface. CSS custom properties live in [`css/tokens.css`](css/tokens.css):

| Role | Hex | CSS token |
| --- | --- | --- |
| Background | `#111111` | `--bg` |
| Panel | `#191919` | `--panel` |
| Raised panel | `#222222` | `--raised` |
| Primary text | `#F2F2ED` | `--text` |
| Secondary text | `#9C9C95` | `--muted` |
| Accent green | `#7CFF6B` | `--accent` |
| Warning amber | `#FFC857` | `--amber` |
| Error red | `#FF5C5C` | `--error` |
| Border | `#2E2E2A` | `--border` |

Use the accent green sparingly. It should evoke:

- terminal success
- PCB LEDs
- robot online state
- successful builds

Do not flood the UI with neon color. Do not introduce a second accent (purple, cyan glow, etc.) without updating this guide and `tokens.css` together.

### Typography

Two families only (loaded via Bunny Fonts in HTML):

| Role | Face | CSS token | Use for |
| --- | --- | --- | --- |
| Primary UI | **IBM Plex Sans** | `--font-sans` | headings, paragraphs, navigation, general UI |
| Monospace | **IBM Plex Mono** | `--font-mono` | API paths, commands, IPs, status labels, hardware metadata, diagnostics, code |

Do not use monospace for all body text. Do not swap in Inter / Roboto / system-ui as the brand face.

Example hierarchy:

```text
Tiny Engineer
Your smallest engineering teammate.

● ONLINE
192.168.1.42
I2S · 22.05 kHz
```

### Layout

Prefer:

- dense but readable layouts
- strong hierarchy
- modular panels
- technical metadata
- compact spacing
- content width around `--max` (~52rem) unless a section truly needs full bleed

Avoid oversized SaaS-style cards with huge rounded corners.

Recommended card / panel style (see `.panel` in `base.css`):

- radius: approximately 6–10 px (`--radius` is `8px`)
- thin borders
- dark panel background
- small uppercase labels (`.panel-label`)
- subtle shadows only
- status indicators where useful

Example:

```text
┌────────────────────────────┐
│ ● ROBOT STATUS             │
│                            │
│ Tiny Engineer     ONLINE   │
│ IP      192.168.1.42       │
│ Wi-Fi   -48 dBm            │
│ Audio   READY              │
└────────────────────────────┘
```

---

## Component Language

Components should resemble:

- hardware modules
- terminal panels
- device dashboards
- diagnostics tools

Good labels:

```text
STATUS
AUDIO
SERVO 03
OLED
I2S
WIFI
BUILD
API
```

Use small status lights (`.led`, `.led-ok`, `.led-warn`, `.led-err`) and short technical labels as visual texture.

Reuse existing building blocks before adding new ones:

| Pattern | Class / file |
| --- | --- |
| Page shell | `.wrap`, `.site-header`, `.site-footer` |
| Panel | `.panel`, `.panel-raised`, `.panel-label` |
| Buttons | `.btn`, `.btn-primary` |
| Status LED | `.led*` |
| Metadata grid | `.meta-grid` |
| Device / PCB motif | `.device-module`, `.pin`, `.pin-row` |
| Code / offsets | `.mono`, `.offset-table` |

---

## Buttons

Buttons should feel slightly physical.

Prefer:

- restrained radius
- small press / depth feedback
- subtle 1–2 px movement on click (`translateY` on `:active`)
- clear active / disabled states
- primary = accent green fill; secondary = raised panel + border

Avoid oversized pill buttons unless there is a strong reason.

---

## Motion

Motion should feel slightly mechanical.

Preferred motion characteristics:

```text
move
→ slight overshoot
→ settle
```

Use:

- short easing (`--ease-mech`, `--dur`)
- subtle pauses
- controlled movement
- tiny overshoot

Avoid:

- floaty animation
- excessive springiness
- liquid / glossy motion
- long cinematic transitions

UI motion may subtly mirror servo behavior (blink, LED pulse, small head nod). Keep decorative motion optional and interruptible.

---

## Hero Section

The robot should be a major visual element when a photo or render is available.

Prefer a layout where the Tiny Engineer render / photo takes approximately **40–50%** of the hero.

Example structure:

```text
TINY ENGINEER

Your smallest
engineering teammate.

A physical companion
for AI coding agents.

[ Build one ] [ GitHub ]

● ONLINE
```

Alongside (when assets exist):

- robot render or photo
- subtle eye blink
- small head movement
- LED pulse
- optional hand typing motion

Until then, a compact device-module / PCB motif panel is acceptable (as on the current landing). Do not fill the hero with abstract gradients instead of the product.

---

## Developer-First Presentation

Code and APIs are part of the product identity and should be visually prominent.

Example:

```bash
curl -X POST \
  "http://tiny-engineer.local/anim?name=thinking"
```

Pair API snippets with a visual or labeled response from the robot when possible.

Useful comparison layout:

```text
YOU SEND                         TINY ENGINEER DOES

POST /anim?name=typing     →     types
POST /anim?name=reading    →     reads
POST /anim?name=thinking   →     looks up
POST /play                 →     speaks
```

Use monospace for paths and commands; keep surrounding explanation in the sans UI font.

---

## Hardware Motifs

Use subtle visual references to:

- PCB traces
- ESP32
- I2C
- I2S
- servo channels
- pin headers
- OLED
- signal paths
- measurement lines

Example:

```text
ESP32-C3 ── I2C ── PCA9685
    │
   I2S
    │
MAX98357A
```

These should support the visual language, not dominate it. Prefer restrained pin chips and dashed module frames over full schematic wallpaper.

---

## Microcopy Personality

The robot should sound like a competent but slightly sarcastic engineering teammate.

| Instead of | Prefer |
| --- | --- |
| Device connected | Tiny Engineer is online. |
| Audio test successful | Speaker works. Dignity intact. |
| No tasks running | Waiting for something to break. |
| Build successful | Build passed. Suspiciously easy. |

Keep jokes short and secondary to usability. Error copy should still say what failed and what to try next.

---

## Avoid

Do not make the UI look like:

- generic SaaS
- generic AI startup
- crypto dashboard
- cyberpunk HUD
- children's robot toy
- corporate enterprise portal

Avoid:

- purple/blue AI gradients
- excessive blur
- glass cards everywhere
- huge rounded corners
- giant decorative typography
- meaningless animations
- excessive emojis

---

## Inspiration Direction

The visual direction should sit somewhere between:

- modern developer tools
- GitHub-style clarity
- Linear-style polish
- Teenage Engineering-style physical product thinking
- electronics lab / maker bench
- embedded-device diagnostics

Do not copy any one reference literally.

---

## Site map (current vs future)

### Current

- `/` — thin landing (hero + CTAs)
- `/flash/` — USB web installer (ESP Web Tools)

### Homepage structure (when expanding)

Recommended order:

#### 1. Hero

Tiny Engineer identity, value proposition, robot visual.

#### 2. See It Work

Show:

- typing
- reading
- thinking
- speaking
- task completion

#### 3. Control It

Show API examples and agent integrations (Cursor, Claude Code, Antigravity, raw HTTP).

#### 4. How It Works

```text
AI coding agent
      ↓
HTTP API
      ↓
ESP32-C3
      ↓
servos + OLED + speaker
```

#### 5. Hardware

Show:

- ESP32-C3
- servos
- PCA9685
- OLED
- speaker
- 3D printed enclosure

#### 6. Build Your Own

Links to:

- GitHub
- BOM / shopping list
- firmware / web flasher
- print files
- setup guide (`docs/getting-started.md`)

#### 7. Footer

Keep it minimal and slightly playful.

Example:

```text
Tiny Engineer
Built because software engineers apparently needed another coworker.
```

New pages should share header/nav/footer patterns and this palette so the site feels like one product, not a pile of demos.

---

## Accessibility and browser notes

- Maintain contrast for primary text on `--bg` / `--panel` (do not lighten muted text further for body copy).
- Do not convey state by color alone; pair LEDs with a text label (`ONLINE`, `READY`, error sentence).
- Web Serial flasher requires Chromium (Chrome/Edge) and HTTPS; say so in UI copy on `/flash/`.
- Prefer semantic headings and real `<button>` / links over clickable `<div>`s.

---

## Final Rule

When making design decisions, prefer the option that feels:

> **more like a small engineered machine and less like a generic software product.**

---

## Deploy and firmware install

GitHub Pages is deployed from `web/` via [`.github/workflows/pages.yml`](../.github/workflows/pages.yml). One-time: repo **Settings → Pages → Source → GitHub Actions**.

Firmware install manifests ship on each `v*` Release as `manifest.json`. Pages deploy mirrors those assets via `scripts/build_flash_catalog.py` into `web/flash/firmware/<tag>/` and `web/flash/releases.json` (same-origin; browser cannot fetch Release CDN bins due to CORS). The flasher must load `releases.json`, not GitHub download URLs.
