# Mods

Optional and community CAD mods for Tiny Engineer. Not stock robot parts — those live in [`cad/`](../cad/) and [`parts/`](../parts/).

## Layout

```text
mods/<mod_name>/
  cad/       # Fusion source
  parts/     # Exported meshes (mirror 3d_models/parts/)
  README.md  # recommended: fit notes, which servo folders exported
```

## Workflow

Design, timeline, `PRINT_LAYOUT`, export, and checklist: [docs/3d/adding-parts.md](../../docs/3d/adding-parts.md).

## Commits

`type(mods): summary`. Name the mod in the summary (`feat(mods): add desk clamp`). One scope for every mod. `feat(mods)` / `fix(mods)` do not version the stock CAD revision. Moving a mod into stock [`cad/`](../cad/) and [`parts/`](../parts/) is `feat(cad)`. Full rules: [CONTRIBUTING.md](../../CONTRIBUTING.md).

## License

Same as parent [`3d_models/`](../) — [CERN-OHL-S-2.0](../LICENSE). See [NOTICE](../NOTICE).
