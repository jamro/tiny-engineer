# HD1370A bottom-cover source

See the [part documentation](../../parts/hd1370a/bottom-cover/README.md) for geometry, previews and the unresolved screw-retention limitation.

`BottomCover_HD1370A.f3d` is the editable native cover. `BottomCover_FitCheck_HD1370A.f3d` contains the source desk/chair fixture and four reference screw envelopes. The master `../TinyEngineer.f3d` contains a hidden `OPTIONAL_BOTTOM_COVER_HD1370A` reference outside `PRINT_LAYOUT`; it must only be shown with the HD1370A preset.

The fixed perimeter comes from `underside_profile.py`, serialized as exact lines/arcs in `underside-profile.json`. `source-alignment.json` records source occurrence transforms in the HD1370A frame. The source mating plane is mapped to Z=4.5 mm; the desk print export needs a 180° rotation about X and the chair a −59.3 mm translation in X.

## Rebuild and check

Use a clean checkout. Install the CAD dependencies into an isolated Python environment:

```sh
python3 -m venv /tmp/tiny-engineer-cad-venv
/tmp/tiny-engineer-cad-venv/bin/pip install -r 3d_models/cad/bottom-cover/requirements.txt
/tmp/tiny-engineer-cad-venv/bin/python 3d_models/cad/bottom-cover/build_and_check.py
```

This writes the canonical STEP/STL/3MF exports, the aligned fixture and geometric checks. It first produces a CadQuery mesh; the following Fusion step replaces the 3MF with the native export. Do not confuse the full inspection assembly with the single printable cover.

In Fusion's Text Commands panel, use **Txt** mode and run these scripts in order, substituting the absolute checkout path:

```text
Python.RunScript "/absolute/checkout/3d_models/cad/bottom-cover/create_in_fusion.py"
Python.RunScript "/absolute/checkout/3d_models/cad/bottom-cover/verify_in_fusion.py"
Python.RunScript "/absolute/checkout/3d_models/cad/bottom-cover/present_in_fusion.py"
Python.RunScript "/absolute/checkout/3d_models/cad/bottom-cover/integrate_master_in_fusion.py"
```

Use a fresh Fusion session for a full rebuild: the verification/presentation scripts select the documents created by the first script. `integrate_master_in_fusion.py` replaces the optional reference in the master, checks the original parameters and geometry before saving. It reopens the saved archive, verifies preservation, then checks the HD1370A fit in that temporary inspection copy without saving preset changes.

Then run:

```sh
/tmp/tiny-engineer-cad-venv/bin/python 3d_models/cad/bottom-cover/verify_master_mates.py
/tmp/tiny-engineer-cad-venv/bin/python 3d_models/cad/bottom-cover/inspect_perimeter.py
/tmp/tiny-engineer-cad-venv/bin/python 3d_models/cad/bottom-cover/verify_print.py
/tmp/tiny-engineer-cad-venv/bin/python 3d_models/cad/bottom-cover/verify_head_seats.py
```

The JSON files are CAD/mesh evidence, not physical load or thread-engagement tests. Previews are generated from the imported CAD and actual print-file section. The original HD1370A mating STEP files are unchanged from the baseline revision recorded in `provenance.json`.

CAD sources, scripts and exports in these CAD directories are CERN-OHL-S-2.0; retain [NOTICE](../../NOTICE).
