# Hardware contribution guidelines (PCB)

Baseline for contributing KiCad PCB designs. Goal: boards that are easy to understand, review, reproduce, and maintain — without extra process yet.

Before opening a PR, run the [checklist](#checklist).

Workflow and automated validation are still evolving. A reference board and CI pipeline are in progress and will be used to validate these rules.

KiCad sources live in [`hardware/boards/`](../hardware/README.md). Robot electrical reference (pinout, wiring, BOM) stays in [`docs/hardware/`](hardware/README.md). Firmware drivers stay in `src/hardware/`.

Legal: board sources are CERN-OHL-S-2.0 — [LICENSING.md](../LICENSING.md), [`hardware/LICENSE`](../hardware/LICENSE), [`hardware/NOTICE`](../hardware/NOTICE). New board paths need a matching `[[annotations]]` block in [REUSE.toml](../REUSE.toml).

## Project structure

Each PCB is a separate, self-contained KiCad project:

`hardware/boards/<board-name>/`

Keep the directory name and KiCad project name the same.

Example:

```
hardware/
  boards/
    controller/
      controller.kicad_pro
      controller.kicad_sch
      controller.kicad_pcb
      README.md
      libraries/
        symbols/
        footprints/
        3d/
```

Do not introduce repository-wide shared KiCad libraries at this stage.

If a board needs custom symbols, footprints, or 3D models, keep them inside that board's directory. Duplication between projects is preferred over shared dependencies too early.

Shared libraries may come later if reuse across boards becomes significant.

## KiCad version

Use **KiCad 10** (current project major). Point releases (10.0.x) are fine.

Do not upgrade a board to a newer KiCad major as part of an unrelated change. Major upgrades are a dedicated, intentional change.

Existing in-progress boards may stay on the major they were started with until a dedicated upgrade.

## Libraries and portability

A contributed board must clone and open on another computer without files from the contributor's local environment.

Store custom project assets inside the board directory.

Use project-relative paths where needed, preferably `${KIPRJMOD}`.

Do not commit absolute local paths such as:

- `C:\Users\...`
- `/Users/...`
- `/home/...`

Standard symbols and footprints shipped with KiCad do not need to be copied into the project.

## Files to commit

Commit the source files required to edit and reproduce the design, including where applicable:

- `.kicad_pro`
- `.kicad_sch`
- `.kicad_pcb`
- `.kicad_dru`
- project-specific symbol libraries
- project-specific footprint libraries
- required 3D models
- `sym-lib-table`
- `fp-lib-table`
- board documentation (`README.md`)

Do not commit temporary, local, backup, or generated files such as:

- `.history/`
- `*-backups/`
- `_autosave-*`
- `*.kicad_prl`
- `fp-info-cache`
- `~*.lck`
- generated Gerber files
- generated drill files
- generated PDFs
- generated BOM or pick-and-place files

Manufacturing outputs should be generated from the source design when they are actually needed.

## Schematic and PCB quality

Before submitting or updating a hardware pull request:

- Run Electrical Rules Check (ERC).
- Run Design Rules Check (DRC).
- Review all reported violations.
- Do not suppress ERC or DRC violations only to make the checks pass.
- Any intentional exception should have a clear reason.

The project should not introduce unexplained ERC or DRC errors.

Automated ERC and DRC via GitHub Actions is planned and is being tested on a reference board. It is not required yet.

## Board README

Each board gets a short `README.md`. At minimum:

- what the board does
- current development status
- main components or interfaces
- important design assumptions
- whether this revision has been physically manufactured and tested

Keep it practical. Do not duplicate what is already obvious from the schematic.

Skeleton:

```markdown
# <board-name>

<one-line purpose>

- **Status:** draft / in review / manufactured / tested
- **KiCad:** 10
- **Interfaces:** <connectors, buses, power>
- **Assumptions:** <voltage, stack-up, mechanical fit, …>
- **Built:** not manufactured | manufactured, not tested | manufactured and tested
```

## Pull requests

Keep hardware pull requests focused on one logical change where reasonably possible.

The pull request should state:

- what changed and why
- whether the schematic, PCB layout, and/or symbols or footprints changed
- whether ERC and DRC were reviewed
- whether the board has been physically tested

A design that passed ERC and DRC is not automatically physically validated. Say clearly whether it was only reviewed in KiCad or actually manufactured and tested.

Commit and PR title: `type(pcb): summary` — [CONTRIBUTING.md](../CONTRIBUTING.md). PCB-only `feat(pcb)` / `fix(pcb)` versions the hardware design, not the npm packages.

## Manufacturing files

Gerbers and other manufacturing outputs are not source files. Do not normally commit them.

Manufacturer requirements vary. Contributors are not required to attach production files to every hardware PR.

When manufacturing a board:

1. Generate the required outputs from the KiCad source.
2. Follow the selected manufacturer's requirements.
3. Inspect the generated files before ordering.
4. Verify the result in the manufacturer's Gerber viewer where available.

A more standardized manufacturing and release workflow may come later, after the process has been validated on real boards.

## Checklist

Use this before opening or updating a PCB pull request. Details are in the sections above.

**Structure and version**

- [ ] Board lives in `hardware/boards/<board-name>/`
- [ ] Directory name matches the KiCad project name
- [ ] KiCad **10** (no drive-by major upgrade)
- [ ] Custom symbols, footprints, and 3D models (if any) are inside that board directory

**Portability and files**

- [ ] Project opens after a clean clone (no files from a local KiCad install)
- [ ] Paths are project-relative (`${KIPRJMOD}`); no absolute `C:\Users\…`, `/Users/…`, or `/home/…`
- [ ] Source committed: `.kicad_pro`, `.kicad_sch`, `.kicad_pcb`, plus `.kicad_dru`, lib tables, and custom libs when used
- [ ] No `.history/`, `*-backups/`, `_autosave-*`, `*.kicad_prl`, `fp-info-cache`, or lock files
- [ ] No generated Gerbers, drill files, PDFs, BOM, or pick-and-place outputs

**Quality and docs**

- [ ] ERC run; violations reviewed
- [ ] DRC run; violations reviewed
- [ ] No suppressions used only to make checks pass; any intentional exception has a reason
- [ ] Board `README.md` covers purpose, status, interfaces, assumptions, and whether this revision was manufactured and tested
- [ ] New board path has a matching `[[annotations]]` block in `REUSE.toml` (CERN-OHL-S-2.0)

**Pull request**

- [ ] Title is `type(pcb): summary`
- [ ] PR states what changed and why
- [ ] PR states whether schematic, PCB layout, and/or symbols or footprints changed
- [ ] PR states ERC/DRC result
- [ ] PR states physical status: KiCad review only / manufactured / manufactured and tested

Existing in-progress PRs: align where reasonably possible. Do not redesign solely to satisfy this list.

## Current baseline vs future direction

The requirements above are the current contribution baseline.

Not standardized yet (to be evaluated on the reference board):

- automated KiCad ERC and DRC in GitHub Actions
- exact multi-board CI structure
- manufacturing jobsets
- automated manufacturing artifacts
- release and prerelease manufacturing packages
- shared KiCad libraries
- automated PCB renders or visual diffs
- manufacturer-specific output configurations

Once the reference implementation is validated, this page will be updated. Stable parts of the workflow may become required for new hardware contributions.

Keep the workflow reproducible and safe. Avoid complexity until it solves a real problem.
