# Fusion add-in

**TinyEngineer Tools** is a Fusion add-in for [`cad/TinyEngineer.f3d`](../cad/TinyEngineer.f3d). It writes servo dimensions from [`TinyEngineerTools/servos.json`](TinyEngineerTools/servos.json) into Fusion user parameters, and exports each `PRINT_LAYOUT` child as a `.3mf` and binary `.stl` mesh (PRINT_LAYOUT with one child visible, so captured print pose stays).

Fusion must know about the add-in folder. Folder name, `TinyEngineerTools.py`, and `TinyEngineerTools.manifest` must stay the same.

Official Autodesk steps: [How to install an add-in or script](https://www.autodesk.com/support/technical/article/caas/sfdcarticles/sfdcarticles/How-to-install-an-ADD-IN-and-Script-in-Fusion-360.html).

## Point Fusion at this repo

Fusion remembers the path. Code stays in git. Reload after edits (see below).

1. Open Fusion.
2. Open **Utilities → Add-Ins → Scripts and Add-Ins**. Shortcut: **Shift+S**.
3. Open the **Add-Ins** tab.
4. Click the green **+** next to **Script of add-in from device**.
5. Browse to `3d_models/fusion/TinyEngineerTools/` in this repo and select the folder.
6. Select **TinyEngineerTools** in the list and click **Run**.
7. Optional: enable **Run on Startup** so Fusion starts it every launch.

## Use the commands

Open [`cad/TinyEngineer.f3d`](../cad/TinyEngineer.f3d) and stay in the **Design** workspace. Both commands live under **Utilities → Add-ins**.

### TinyEngineer Servo Configurator

Select a servo model and apply dimensions, including `servo_id` (short lowercase id such as `sg90`).

### Tiny Engineer Parts Exporter

Choose an export folder.

The add-in finds the `PRINT_LAYOUT` component and exports each of its direct child components separately. For each child, it temporarily hides the others, keeps that child visible in its saved print orientation, and exports the full PRINT_LAYOUT as:

* `{servo_id}/3mf/{name}.3mf`
* `{servo_id}/stl/{name}.stl` in binary STL format
* `{servo_id}/README.md` with that servo’s parameters from `servos.json`

`servo_id` comes from the Fusion parameter set by Servo Configurator. Apply a servo before exporting.

In that way, each component is exported as a separate file, all keeping their captured print orientation.

After all components are exported, the original visibility settings are restored. A progress dialog stays up during the run so Fusion can paint; Cancel stops after the current part.
