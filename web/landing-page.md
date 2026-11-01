# Landing page content brief

Source of truth for **what** `/` on [tinyengineer.dev](https://tinyengineer.dev) (GitHub Pages from [`web/`](../web/)) should say and link.

| Concern | Doc |
| --- | --- |
| **Content / messaging** (this file) | What each section says, CTAs, outbound links, claims |
| **Visual / UI** | [`web/STYLE.md`](../web/STYLE.md) — palette, components, voice, section spine |

Engineering depth stays in `docs/` on GitHub. The marketing site does not host a docs browser. Flash stays at `/flash/`; do not merge flasher UX into the landing.

Canonical product copy also lives in [`README.md`](../README.md). Prefer README wording when this brief and the README diverge on facts; prefer this brief for section order and CTA priority on `/`.

---

## Audience

Software engineers who already use AI coding agents (Cursor, Claude Code, Antigravity, Codex, Windsurf, or custom scripts). They understand HTTP and CLI; many have never built a servo robot. Speak to that gap without condescension.

---

## Goals

1. **Explain** — visitor knows Tiny Engineer is a physical desk robot for AI coding agents.
2. **Create demand** — agent work becomes visible (move, face, bell); feels like a teammate, not a toy.
3. **Show the build path** — Print → Connect → Flash → Assemble, with deep links into GitHub docs.
4. **Route** — builders to [getting-started](getting-started.md); flashers to `/flash/`; hackers to GitHub / API / integrations.

## Non-goals

- Full API reference, PCB contribute guide, or hardware primer chapters on the page.
- Kit storefront, fixed BOM price, or “official” IDE partnership claims.
- Multi-page marketing site; roadmap sold as shipped product.
- Rewriting `/flash/` or the robot embedded `ui/`.

---

## Information architecture

Match [`web/STYLE.md` homepage structure](../web/STYLE.md#homepage-structure-when-expanding):

1. Hero  
2. See it work  
3. Control it  
4. How it works  
5. Hardware  
6. Build your own  
7. Footer  

**Keep / cut / defer** (scored for SWE clarity, demand, funnel, STYLE fit, minimal other-page change):

| Verdict | Items |
| --- | --- |
| **Keep** | Sections 1–7 above; agent↔pose table; `npx` setup; compact hardware list; Show and tell + Star |
| **Cut** | Full roadmap as features; kit price; API tables; KiCad CTA; SWE primer dump; multi-page IA |
| **Defer** | Live animated hero robot; community gallery grid; vs-other-toys comparison; newsletter; storefront |

---

## Global CTA and nav

### CTA priority

| Priority | Label | Target |
| --- | --- | --- |
| **Primary** | Build one | [getting-started.md](https://github.com/jamro/tiny-engineer/blob/main/docs/getting-started.md) |
| **Secondary** | Watch demo | https://youtu.be/RX_QRxdXMjg |
| **Secondary** | GitHub | https://github.com/jamro/tiny-engineer |
| **Tertiary / nav** | Flash firmware | `/flash/` (same origin) |

Hero may show Build + Watch demo + GitHub. Flash remains in the site nav (and optionally a quiet tertiary link). Do not make Flash the only primary CTA on `/` — builders need the checklist first.

### Nav (minimal change)

Keep current pattern from [`web/index.html`](../web/index.html):

- Brand → `/`
- Flash → `/flash/`
- GitHub → repo root
- Build → `docs/getting-started.md` on GitHub

No new top-level marketing routes required for v1.

---

## Section briefs

URL conventions below use GitHub `blob/main/docs/…` for markdown and same-origin paths for Pages (`/flash/`). When assets live in `docs/` but need to appear on the site, copy or reference them from `web/` at implement time (do not hotlink raw.githubusercontent for critical hero media if avoidable).

### 1. Hero

**Intent:** One viewport: brand, value, desire, path. Physical product first.

**Eyebrow:** `Desk robot · ESP32-C3`  
*(keep current)*

**H1:** `Your smallest engineering teammate.`  
*(keep current)*

**Lede (near-final):**

> A physical companion for AI coding agents. It moves, changes its face, and rings a bell as your agent works. Flash over USB, then let it act out the work.

*(Merge current web lede with README spine. Alternate README tagline for social/meta only: “Give your AI coding agent a body.”)*

**CTAs:**

- Primary: `Build one` → getting-started  
- Secondary: `Watch demo` → YouTube  
- Secondary: `GitHub` → repo  
- Optional tertiary text link: `Flash firmware` → `/flash/`

**Media:** Prefer robot photo or GIF over PCB-only panel when implementing.

| Asset | Path |
| --- | --- |
| Demo GIF | [`docs/tiny-engineer-preview.gif`](tiny-engineer-preview.gif) |
| Still | [`docs/tiny-engineer-preview.jpg`](tiny-engineer-preview.jpg) |

Until the photo/GIF is in the hero, the existing **device-module** aside (READY TO FLASH, pin chips, bus line) remains acceptable per STYLE.md. Do not fill the hero with abstract gradients.

**Reuse:** README opening + current `web/index.html` hero shell.

**Deep link:** one — Build → getting-started (others are CTAs, not section footnotes).

---

### 2. See it work

**Intent:** Proof before BOM. Agent work → visible behavior.

**Section heading:** `See it work`

**Supporting line:**

> Your agent stays in the IDE. The robot makes the work visible on your desk.

**Table (required — from README):**

| Your agent… | The robot… |
| --- | --- |
| Reads files | Looks around |
| Writes code | Types |
| Thinks or waits | Looks up |
| Finishes the task | Rings the bell |

**Media:** Link or embed [YouTube demo](https://youtu.be/RX_QRxdXMjg); optional still [`tiny-engineer-preview.jpg`](tiny-engineer-preview.jpg) or GIF beside the table.

**CTA / deep link:** `Watch the demo →` (YouTube). Optional: `Community builds →` [Show and tell](https://github.com/jamro/tiny-engineer/discussions/categories/show-and-tell).

**Reuse:** README “How it works” table and demo links.

---

### 3. Control it

**Intent:** Familiar SWE interface first — CLI and HTTP — before PCB lore.

**Section heading:** `Control it`

**Supporting line:**

> One CLI for the common IDEs. Any script that can POST works too.

**Primary snippet:**

```bash
npx -y tiny-engineer setup
```

**Secondary snippet (monospace, STYLE “developer-first”):**

```bash
curl -X POST \
  "http://tiny-engineer.local/anim?name=thinking"
```

**Supported via CLI (factual, not partnership):** Cursor · Claude Code · Antigravity  

**Escape hatch line:**

> Codex, Windsurf, CI, or a custom agent: `POST /anim` over Wi‑Fi, or USB serial when the network is locked down.

**Optional comparison strip (STYLE):**

```text
YOU SEND                         TINY ENGINEER DOES
POST /anim?name=typing     →     types
POST /anim?name=reading    →     reads
POST /anim?name=thinking   →     looks up
POST /play                 →     speaks
```

**Deep links (pick one primary per subsection):**

- Integrations: [integration.md](https://github.com/jamro/tiny-engineer/blob/main/docs/integration.md)  
- HTTP API: [api.md](https://github.com/jamro/tiny-engineer/blob/main/docs/api.md)  
- Serial (optional footnote): [serial.md](https://github.com/jamro/tiny-engineer/blob/main/docs/serial.md)

**Reuse:** README “Works with” + integration.md install block.

---

### 4. How it works

**Intent:** Mental model in four steps. No firmware deep dive.

**Section heading:** `How it works`

**Stack (STYLE + README):**

```text
AI coding agent
      ↓
HTTP or USB serial
      ↓
ESP32-C3
      ↓
servos + OLED + speaker
```

**One-sentence body:**

> Your agent (or a hook script) sends poses over Wi‑Fi HTTP or USB serial. The ESP32-C3 runs the firmware and drives the servos, face, and speaker.

**Media:** [`docs/how-it-works.jpg`](how-it-works.jpg) when used on the site.

**Deep link:** [integration.md](https://github.com/jamro/tiny-engineer/blob/main/docs/integration.md) (or “How it works” region of README — prefer integration for builders who just saw Control it).

**Reuse:** README “How it works” + STYLE stack diagram.

---

### 5. Hardware

**Intent:** Compact “what’s on the desk,” not a datasheet. Pin chips may already appear in the hero device-module — this section names the build, not every net.

**Section heading:** `Hardware`

**Supporting line:**

> Open-source board, printed body, parts you can buy. Medium difficulty — a fair first hardware project for a software engineer, not a one-LED kit.

**What’s in the build (short list):**

- Main control board (PCA9685, audio amp, USB-C on board)  
- Waveshare ESP32-C3-Zero  
- Five SG90-class servos (default)  
- OLED face + speaker  
- 3D-printed enclosure  

**Safety one-liner (optional microcopy):** Supply **5 V / ≥2 A**. Never power servos from the ESP32 3.3 V LDO.

**Deep links:**

- Shopping list: [shopping.md](https://github.com/jamro/tiny-engineer/blob/main/docs/shopping.md)  
- Order PCB: [order-main-control-board.md](https://github.com/jamro/tiny-engineer/blob/main/docs/hardware/order-main-control-board.md)  
- Optional interest vote only (not a product claim): [discussion #50](https://github.com/jamro/tiny-engineer/discussions/50) — ~$15–20 assembled board *if* useful; do not present as available store SKU  

**Do not:** invent a total kit price; dump component tables from shopping.md.

**Reuse:** README “Build your own” difficulty line + shopping default cart names.

---

### 6. Build your own

**Intent:** Demand → action. Funnel with four deep links; checklist owns the rest.

**Section heading:** `Build your own`

**Funnel line:**

> Print → Connect → Flash → Assemble

**Step cards / links:**

| Step | Link |
| --- | --- |
| Full checklist | [getting-started.md](https://github.com/jamro/tiny-engineer/blob/main/docs/getting-started.md) |
| Parts | [shopping.md](https://github.com/jamro/tiny-engineer/blob/main/docs/shopping.md) |
| Flash | `/flash/` (site) · detail [flash.md](https://github.com/jamro/tiny-engineer/blob/main/docs/flash.md) |
| Assemble | [3d/assembly.md](https://github.com/jamro/tiny-engineer/blob/main/docs/3d/assembly.md) |

**Also useful (one line each, not a second checklist):**

- No printer: [order printed parts](https://github.com/jamro/tiny-engineer/blob/main/docs/3d/order-parts.md)  
- New to electronics: optional [From Code to Circuits](https://github.com/jamro/tiny-engineer/blob/main/docs/hardware-for-software-engineers/README.md)  
- Agent hooks after prove: [integration.md](https://github.com/jamro/tiny-engineer/blob/main/docs/integration.md)

**Closers:**

- `Built one? Share in Show and tell →` — [Show and tell](https://github.com/jamro/tiny-engineer/discussions/categories/show-and-tell)  
- `Star Tiny Engineer` — https://github.com/jamro/tiny-engineer  

**Primary CTA repeat:** `Build one` → getting-started.

**Reuse:** README “Build your own” + getting-started timeline short form.

---

### 7. Footer

**Intent:** Minimal, playful, on-brand.

**Keep:**

> Built because software engineers apparently needed another coworker.

**Optional second line (licenses, if space):**

> Software MIT · Hardware CERN-OHL-S-2.0 · Name and logo reserved

Link licenses to [`LICENSE`](https://github.com/jamro/tiny-engineer/blob/main/LICENSE) / [`LICENSING.md`](https://github.com/jamro/tiny-engineer/blob/main/LICENSING.md) only if the footer stays scannable. Do not paste trademark policy here.

---

## Meta / SEO (page head)

**Title:** `Tiny Engineer`  
**Description (near-final):**

> Tiny Engineer — a desk robot for AI coding agents. Build one from open hardware, flash firmware in the browser, and hook Cursor, Claude Code, or any script that can POST.

---

## Claims and legal checklist

Do **not** claim on `/`:

| Forbidden | Why |
| --- | --- |
| Fixed total kit / BOM price | Shopping list has no summed price |
| Ready-to-buy assembled main board as a SKU | Interest vote only ([discussion #50](https://github.com/jamro/tiny-engineer/discussions/50)) |
| “Official” Cursor / Claude / Antigravity product or partnership | CLI hooks only; third-party names are integrators |
| “Beginner electronics kit” / trivial first project | Docs: **medium**; fair first *hardware* project for an SWE |
| Roadmap items as shipped (multi-agent desk, new bodies, …) | README roadmap ≠ product |
| “Everything is MIT” | Software MIT; CAD/PCB CERN-OHL-S-2.0 |
| Trademark-free rebrand rights | [TRADEMARK.md](../TRADEMARK.md) — name, logo, branded images reserved |

**Do** claim (accurate):

- Open source (with dual license scope above)  
- Works with Cursor, Claude Code, Antigravity via `npx -y tiny-engineer setup`  
- Any agent/script via `POST /anim` / USB serial  
- Browser web flash for stock firmware (Chromium + HTTPS; detail on `/flash/`)  
- 5 V / ≥2 A power requirement  

---

## Implementation notes (later HTML pass)

Out of scope for this brief’s authoring, but required when implementing:

1. Extend [`web/index.html`](../web/index.html) only for new sections; reuse header/nav/footer patterns.  
2. Add section styles in [`web/css/base.css`](../web/css/base.css) / tokens if needed — follow [`web/STYLE.md`](../web/STYLE.md); no new visual system.  
3. **Do not** change [`web/flash/`](../web/flash/) behavior, `releases.json` flow, or [`web/q/`](../web/q/) redirects.  
4. **Do not** move engineering docs into Pages HTML; keep outbound links to GitHub `docs/`.  
5. Hero media: host under `web/` (or document relative path) so Pages serves GIF/JPG without depending on GitHub blob UI.  
6. Personality: short sarcasm in microcopy/footer only; section body stays clear ([STYLE microcopy](../web/STYLE.md#microcopy-personality)).  
7. Conventional commit when shipping HTML: prefer `docs` for this brief; site HTML later often `docs` or `chore` per CONTRIBUTING — match repo practice.

---

## Acceptance checklist

A visitor who only reads `/` can answer:

- [ ] **What is it?** Desk robot that gives an AI coding agent a physical body (move, face, bell).  
- [ ] **Why care?** Agent work becomes visible; feels like a tiny engineering teammate.  
- [ ] **How do I control it?** `npx -y tiny-engineer setup` or `POST /anim`.  
- [ ] **How do I build one?** Print → Connect → Flash → Assemble → [getting-started](getting-started.md).  
- [ ] **Where do I flash?** `/flash/` (and flash.md if stuck).  
- [ ] **Where is proof?** Demo video / GIF and optional Show and tell.  

Implementer can paste section-by-section from this file into `web/index.html` without inventing messaging or touching `/flash/`.
