# Fusion add-in

**TinyEngineer Tools** writes servo dimensions from [`TinyEngineerTools/servos.json`](TinyEngineerTools/servos.json) into Fusion user parameters. Use it with [`cad/TinyEngineer.f3d`](../cad/TinyEngineer.f3d).

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

## Use the command

1. Open [`cad/TinyEngineer.f3d`](../cad/TinyEngineer.f3d).
2. Stay in the **Design** workspace.
3. Go to **Utilities → Add-ins → TinyEngineer Servo Configurator**.
4. Select a servo model and apply dimensions.
