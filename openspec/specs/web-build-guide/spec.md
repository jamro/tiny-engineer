# web-build-guide Specification

## Purpose

Defines beginner-first voice and stage chrome for the public GitHub Pages build guide from Shop through Setup, so solder-never visitors can follow the flow without treating repo Markdown docs as part of that tone contract.

## Requirements

### Requirement: Build guide audience and voice
The public web build guide stages Shop, Print, Flash, Assembly, and Setup MUST present primary headings, ledes, panel labels, and primary CTAs in plain language aimed at visitors who have never soldered. Primary UI copy MUST lead with the job or outcome before vendor names, chip IDs, or fab jargon. Repo Markdown under `docs/` and other GitHub documentation linked from those stages MUST NOT be required to match this voice.

#### Scenario: Stage chrome uses plain language
- **WHEN** a solder-never visitor opens any of the five build-guide stage pages
- **THEN** the stage title and lede explain what to do in everyday words before any optional technical detail

#### Scenario: Linked Markdown excluded from voice lock
- **WHEN** a visitor follows a link from a stage page into repo Markdown documentation
- **THEN** that documentation MAY use technical contributor voice without violating this capability

### Requirement: Five-stage build timeline
The build guide MUST expose the ordered stages Shop → Print → Flash → Assembly → Setup in shared stage navigation chrome on each of those pages. Connect and Use MUST NOT appear as separate timeline stages; print continues to flash, and prove/agent content lives on Setup after the on-robot wizard.

#### Scenario: Timeline present on each stage
- **WHEN** a visitor opens any build-guide stage page
- **THEN** they see navigation for all five stages in that order with the current stage indicated

#### Scenario: Print continues to flash
- **WHEN** a visitor finishes the print stage continue control
- **THEN** they navigate to `/flash/` (not a Connect stage)

#### Scenario: Setup includes try-it
- **WHEN** a visitor opens `/setup/`
- **THEN** a Try it block appears below the On-robot wizard panel

### Requirement: Light chrome consistency outside shop
Print, Flash, Assembly, and Setup MUST keep beginner-compatible stage chrome (title, lede, primary panel labels, primary CTA wording). This capability MUST NOT require those stages to absorb full shopping-cart or JLCPCB flows that belong on Shop.

#### Scenario: Non-shop stage lede is beginner-readable
- **WHEN** a visitor opens `/print/`, `/flash/`, `/assembly/`, or `/setup/`
- **THEN** the on-page lede and primary CTA use plain language consistent with the build-guide voice

#### Scenario: Deep MD content remains linked
- **WHEN** a thin stage page points to detailed instructions in repo Markdown
- **THEN** the stage MAY keep that link and MUST NOT be required to inline the full Markdown body for voice compliance
