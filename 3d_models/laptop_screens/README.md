# Laptop screen inserts

Paper inserts that turn the plain black `LaptopScreen` into a terminal, an IDE, an agent chat, a crash dump, a retro computer, an ELIZA-style chat, a Tiny Engineer splash, a rubber duck, a Linux boot with Tux, pixel aliens, a blocky green neighbour or a red-eyed TE-9000 console. The pop-culture ones are original drawings in the spirit of the classics, not copies; Tux is credited to Larry Ewing, as he asks. No electronics: print, cut, slide into the lid. Inspired by a [community build](https://github.com/jamro/tiny-engineer/discussions/13#discussioncomment-18741594) that used a printed ELIZA screenshot behind clear PET.

**Print:** one PDF per servo preset — download the one matching the folder you printed from [`parts/`](../parts/):

| Preset | PDF |
| --- | --- |
| `sg90` | [`laptop-screens-sg90.pdf`](laptop-screens-sg90.pdf) |
| `fs0307` | [`laptop-screens-fs0307.pdf`](laptop-screens-fs0307.pdf) |
| `es9051` | [`laptop-screens-es9051.pdf`](laptop-screens-es9051.pdf) |
| `hd1370a` | [`laptop-screens-hd1370a.pdf`](laptop-screens-hd1370a.pdf) |

Each is A4 with one page per print style, every design on each:

- **Printed background** — each design on its own dark (or light) background. Most ink.
- **No background, colour text** — only the text prints; each colour keeps its hue but is darkened so it reads on pale paper.
- **No background, black text** — only the text prints, in black and grey. Least ink.

Two questions pick the page: does your printer do colour, and do you want the background printed?

| Printer | Printed background | No background |
| --- | --- | --- |
| Colour | Printed background | No background, colour text |
| Black only | Printed background (comes out in greys; heavy on toner) | No background, black text |

With no background the paper is the screen: green paper gives an old-LCD look, and any pale colour works. Ink is translucent, so printed colours mix with the paper (blue on yellow turns greenish). Printers cannot print white, so dark paper (black, navy) will not show the text.

## Print, cut, install

1. Open the PDF for your servo preset (table above) and print **the page for your printer and background** from the table above. A style with more designs than fit one page continues onto the next. Each insert is labelled with its preset.
2. Print at **100% / Actual size**. Turn off "Fit to page" / "Shrink to printable area".
3. Check the scale: the square at the top must measure **10 × 10 mm** and the ruler **50 mm**. If not, fix the print settings and reprint — a scaled insert will not fit the pocket.
4. Cut along the thin grey outline. Snip the four corners slightly: the pocket has rounded corners.
5. Cut a piece of clear PET (a drink bottle or blister pack works) to the same size.
6. Slide the paper insert, printed side towards you, into the pocket on the underside of `LaptopCase` with the PET in front of it, then slide `LaptopScreen` in behind them as usual ([assembly §14](../../docs/3d/assembly.md#14-desk-props--laptop-mug-bell)). The top of the design goes to the top of the lid, away from the hinge.

The pocket has about **0.6 mm** of spare depth behind the lip. Plain 80 g/m² paper (~0.1 mm) plus thin bottle PET (~0.2–0.3 mm) fits. If it binds, leave out the PET or use thinner paper.

Only the top part of each insert shows through the bezel; the bottom ~6–8 mm hides behind the hinge. The designs keep their content in the visible window.

## Sizes

Measured from the exported meshes (`LaptopScreen.stl` for the insert, the lid pocket and lip in `LaptopCase.stl` for the window). Window offsets are from the left and top edge of the insert.

| Preset | Insert (W × H) | Visible window (W × H) | Window offset |
| --- | --- | --- | --- |
| `sg90` | 45.6 × 34.4 mm | 45.2 × 25.9 mm | 0.2 / 1.7 mm |
| `fs0307` | 33.8 × 26.0 mm | 33.4 × 17.5 mm | 0.2 / 1.7 mm |
| `es9051` | 37.2 × 28.4 mm | 36.8 × 19.9 mm | 0.2 / 1.7 mm |
| `hd1370a` | 32.7 × 25.2 mm | 32.3 × 16.7 mm | 0.2 / 1.7 mm |

## Make your own

**[Laptop screen generator](https://jamro.github.io/tiny-engineer/screens/)** — in the browser, nothing to install. Pick your servo preset, then either type a screen (or edit one of the stock designs, colours included) or drop in a screenshot, which is cropped to the visible window. It previews the insert at real proportions and downloads an A4 PDF of copies with the same scale check. Images stay in your browser.

No internet? Cut out the **Blank template** and draw on it or glue a print inside the dashed line, using the sizes above.

## Add or edit a screen

Designs are plain-text files in [`screens/`](screens/), one per insert, drawn in a monospaced font scaled to fill the window. Files are laid out in name order.

```text
# title: Build output
# background: #0d1117
# foreground: #c9d1d9
# color ok: #3fb950
# align: top-left
# outline: none
# color dim: #6e7681
# paper-dim: dim
$ pio run
{ok}[SUCCESS]{/} Took 12.84 seconds
```

| Header | Meaning |
| --- | --- |
| `title` | Label printed under the insert |
| `background` | Fill for the whole insert, `#rrggbb` |
| `foreground` | Default text colour |
| `color <name>` | Named colour for `{name}…{/}` spans in the text |
| `align` | `top-left` (default) or `center` |
| `outline` | `window` draws a dashed line around the visible area (blank template) |
| `paper-dim` | Colour names the design treats as secondary: grey on the No background, black text page (other text is black), and darkened less on the No background, colour text page |

Text lines follow the headers. `{name}` switches to a named colour, `{/}` back to the foreground. Characters must be printable in the PDF base fonts (ASCII and Latin-1 are fine).

Every text colour needs at least **3:1** contrast against `background`, so the Printed background page still reads from a black-only printer. The script refuses a design that falls below it.

Then rebuild the PDF from the repo root (Python 3, standard library only):

```bash
python3 scripts/laptop_screens.py           # rewrite laptop-screens-<preset>.pdf
python3 scripts/laptop_screens.py --check   # CI: fail if the PDF is stale
```

The script re-measures every `parts/{servo_id}/stl/` folder that has a `LaptopScreen.stl`, so a new servo preset gets its own page after the Fusion export — no sizes to edit by hand. It also writes `web/screens/presets.json` and `designs.json`, which the generator page reads; `--check` covers those too. The generator's **Copy .screen source** button gives you a file in this format, ready for a PR.
