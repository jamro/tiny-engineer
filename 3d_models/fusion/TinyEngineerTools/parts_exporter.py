"""Export PRINT_LAYOUT children as 3MF, binary STL, and STEP."""

import os
import traceback

import adsk
import adsk.core
import adsk.fusion

import servo

app = adsk.core.Application.get()
ui = app.userInterface

COMMAND_ID = 'TinyEngineerPartsExporter'
COMMAND_NAME = 'Tiny Engineer Parts Exporter'
COMMAND_DESCRIPTION = 'Export each PRINT_LAYOUT child as 3MF, binary STL, and STEP'
WORKSPACE_ID = 'FusionSolidEnvironment'
PANEL_ID = 'SolidScriptsAddinsPanel'
PRINT_LAYOUT_NAME = 'PRINT_LAYOUT'
_INVALID_FILENAME_CHARS = '<>:"/\\|?*'

_handlers = []


def start():
    cmd_def = ui.commandDefinitions.itemById(COMMAND_ID)
    if not cmd_def:
        cmd_def = ui.commandDefinitions.addButtonDefinition(
            COMMAND_ID, COMMAND_NAME, COMMAND_DESCRIPTION
        )

    created = CommandCreatedHandler()
    cmd_def.commandCreated.add(created)
    _handlers.append(created)

    workspace = ui.workspaces.itemById(WORKSPACE_ID)
    if not workspace:
        ui.messageBox(f'Workspace not found: {WORKSPACE_ID}')
        return

    panel = workspace.toolbarPanels.itemById(PANEL_ID)
    if not panel:
        ui.messageBox(f'Toolbar panel not found: {PANEL_ID}')
        return

    control = panel.controls.itemById(COMMAND_ID)
    if not control:
        control = panel.controls.addCommand(cmd_def, servo.COMMAND_ID, False)

    control.isPromoted = True
    control.isPromotedByDefault = True


def stop():
    workspace = ui.workspaces.itemById(WORKSPACE_ID)
    if workspace:
        panel = workspace.toolbarPanels.itemById(PANEL_ID)
        if panel:
            control = panel.controls.itemById(COMMAND_ID)
            if control:
                control.deleteMe()

    cmd_def = ui.commandDefinitions.itemById(COMMAND_ID)
    if cmd_def:
        cmd_def.deleteMe()

    _handlers.clear()


def _error(title):
    ui.messageBox(f'{title}:\n\n{traceback.format_exc()}')


def _safe_filename(name):
    cleaned = ''.join('_' if ch in _INVALID_FILENAME_CHARS else ch for ch in name)
    cleaned = cleaned.strip().rstrip('.')
    return cleaned or 'part'


def _unique_base(occ, used):
    base = _safe_filename(occ.component.name)
    if base not in used:
        used.add(base)
        return base

    suffixed = _safe_filename(f'{occ.component.name}_{occ.name}')
    candidate = suffixed
    index = 2
    while candidate in used:
        candidate = f'{suffixed}_{index}'
        index += 1
    used.add(candidate)
    return candidate


def _occurrence_list(occurrences):
    if not occurrences:
        return []
    return [occurrences.item(i) for i in range(occurrences.count)]


def _find_print_layout(design):
    root = design.rootComponent
    if root.name == PRINT_LAYOUT_NAME:
        return root, _occurrence_list(root.occurrences), root

    for occ in root.allOccurrences:
        if occ.component.name == PRINT_LAYOUT_NAME:
            return occ, _occurrence_list(occ.childOccurrences), occ.component

    return None, None, None


def _body_list(component):
    bodies = component.bRepBodies
    if not bodies:
        return []
    return [bodies.item(i) for i in range(bodies.count)]


def _set_child_visible(children, visible_occ):
    for occ in children:
        occ.isLightBulbOn = False
    if visible_occ:
        visible_occ.isLightBulbOn = True


def _occurrence_ancestors(occ):
    chain = []
    current = occ
    while current:
        chain.append(current)
        current = current.assemblyContext
    return chain


def _restore_visibility(occ_state, body_state, layout_component, folder_on):
    for occ, was_on in occ_state:
        occ.isLightBulbOn = was_on
    for body, was_on in body_state:
        body.isLightBulbOn = was_on
    layout_component.isBodiesFolderLightBulbOn = folder_on


def _read_servo_id(design):
    param = design.userParameters.itemByName(servo.SERVO_ID_PARAM)
    if not param:
        return None
    try:
        value = param.textValue
    except Exception:
        value = param.expression
    value = (value or '').strip().strip('"').strip("'").strip()
    return value.lower() if value else None


def _find_servo_entry(servo_id):
    for name, data in servo._load_servos().items():
        if data.get(servo.SERVO_ID_PARAM) == servo_id:
            return name, data
    return None, None


def _write_servo_readme(servo_dir, title, data):
    lines = [f'# {title}', '']
    for key, value in data.items():
        lines.append(f'- `{key}`: {value}')
    lines.append('')
    with open(os.path.join(servo_dir, 'README.md'), 'w', encoding='utf-8') as file:
        file.write('\n'.join(lines))


def _choose_folder():
    dialog = ui.createFolderDialog()
    dialog.title = 'Select export folder'
    if dialog.showDialog() != adsk.core.DialogResults.DialogOK:
        return None
    return dialog.folder


def _export_dirs(folder, servo_id):
    servo_dir = os.path.join(folder, servo_id)
    stl_dir = os.path.join(servo_dir, 'stl')
    c3mf_dir = os.path.join(servo_dir, '3mf')
    step_dir = os.path.join(servo_dir, 'step')
    os.makedirs(stl_dir, exist_ok=True)
    os.makedirs(c3mf_dir, exist_ok=True)
    os.makedirs(step_dir, exist_ok=True)
    return servo_dir, stl_dir, c3mf_dir, step_dir


def _export_meshes(export_mgr, geometry, stl_dir, c3mf_dir, base):
    errors = []
    stl_path = os.path.join(stl_dir, f'{base}.stl')
    c3mf_path = os.path.join(c3mf_dir, f'{base}.3mf')

    try:
        stl = export_mgr.createSTLExportOptions(geometry, stl_path)
        stl.isBinaryFormat = True
        stl.sendToPrintUtility = False
        export_mgr.execute(stl)
    except Exception as exc:
        errors.append(f'{base}.stl: {exc}')

    try:
        c3mf = export_mgr.createC3MFExportOptions(geometry, c3mf_path)
        c3mf.sendToPrintUtility = False
        export_mgr.execute(c3mf)
    except Exception as exc:
        errors.append(f'{base}.3mf: {exc}')

    return errors


def _step_component(geometry):
    component = adsk.fusion.Component.cast(geometry)
    if component:
        return component
    occ = adsk.fusion.Occurrence.cast(geometry)
    if occ:
        return occ.component
    return None


def _export_step(export_mgr, geometry, step_dir, base):
    errors = []
    step_path = os.path.join(step_dir, f'{base}.step')

    try:
        # File → Export. createSTEPExportOptions(filename, geometry) rejects
        # Occurrence (Save as Mesh accepts it) and fails silent. Create with
        # filename, set .geometry to PRINT_LAYOUT; fall back to Component.
        step = export_mgr.createSTEPExportOptions(step_path)
        if not step:
            return [f'{base}.step: createSTEPExportOptions failed']
        step.geometry = geometry
        if export_mgr.execute(step):
            return errors

        component = _step_component(geometry)
        if not component:
            return [f'{base}.step: ExportManager.execute failed']
        step = export_mgr.createSTEPExportOptions(step_path, component)
        if not step or not export_mgr.execute(step):
            errors.append(f'{base}.step: ExportManager.execute failed')
    except Exception as exc:
        errors.append(f'{base}.step: {exc}')

    return errors


def _export_parts():
    design = adsk.fusion.Design.cast(app.activeProduct)
    if not design:
        ui.messageBox('Open a Fusion design first.')
        return

    raw_id = _read_servo_id(design)
    if not raw_id:
        ui.messageBox(
            'Apply a servo in TinyEngineer Servo Configurator first '
            '(servo_id is missing).'
        )
        return

    servo_id = _safe_filename(raw_id)
    servo_name, servo_data = _find_servo_entry(raw_id)
    if not servo_data:
        ui.messageBox(
            f'No servo in servos.json matches servo_id "{raw_id}".'
        )
        return

    folder = _choose_folder()
    if not folder:
        return

    geometry, children, layout_component = _find_print_layout(design)
    if geometry is None:
        ui.messageBox(f'Component "{PRINT_LAYOUT_NAME}" was not found.')
        return

    if not children:
        ui.messageBox(f'"{PRINT_LAYOUT_NAME}" has no children to export.')
        return

    occ_state = [(occ, occ.isLightBulbOn) for occ in children]
    bodies = _body_list(layout_component)
    body_state = [(body, body.isLightBulbOn) for body in bodies]
    folder_on = layout_component.isBodiesFolderLightBulbOn

    layout_occ = adsk.fusion.Occurrence.cast(geometry)
    layout_path = _occurrence_ancestors(layout_occ) if layout_occ else []
    layout_state = [(occ, occ.isLightBulbOn) for occ in layout_path]

    export_mgr = design.exportManager
    servo_dir, stl_dir, c3mf_dir, step_dir = _export_dirs(folder, servo_id)
    _write_servo_readme(servo_dir, servo_name, servo_data)
    used_names = set()
    exported = 0
    errors = []
    cancelled = False

    progress = ui.createProgressDialog()
    progress.isBackgroundTranslucent = False
    progress.isCancelButtonShown = True

    try:
        for occ, _ in layout_state:
            occ.isLightBulbOn = True
        layout_component.isBodiesFolderLightBulbOn = False
        for body, _ in body_state:
            body.isLightBulbOn = False
        _set_child_visible(children, None)

        progress.show(
            'Tiny Engineer Parts Exporter', 'Exporting %v / %m', 0, len(children), 1
        )

        for index, occ in enumerate(children):
            adsk.doEvents()
            if progress.wasCancelled:
                cancelled = True
                break

            base = _unique_base(occ, used_names)
            progress.progressValue = index
            progress.message = f'Exporting {base} (%v / %m)'

            _set_child_visible(children, occ)
            part_errors = _export_meshes(
                export_mgr, geometry, stl_dir, c3mf_dir, base
            )
            part_errors.extend(
                _export_step(export_mgr, geometry, step_dir, base)
            )
            if part_errors:
                errors.extend(part_errors)
            else:
                exported += 1
            occ.isLightBulbOn = False
            adsk.doEvents()

        if not cancelled:
            progress.progressValue = len(children)
    finally:
        progress.hide()
        _restore_visibility(
            occ_state + layout_state, body_state, layout_component, folder_on
        )

    lines = [f'Exported {exported} part(s) to:\n{servo_dir}']
    if cancelled:
        lines.append('Export cancelled.')
    if errors:
        lines.append('Failures:\n' + '\n'.join(errors))
    ui.messageBox('\n\n'.join(lines))


class ExecuteHandler(adsk.core.CommandEventHandler):
    def notify(self, args):
        try:
            _export_parts()
        except Exception:
            _error('Execute error')


class CommandCreatedHandler(adsk.core.CommandCreatedEventHandler):
    def notify(self, args):
        try:
            execute_handler = ExecuteHandler()
            args.command.execute.add(execute_handler)
            _handlers.append(execute_handler)
        except Exception:
            _error('Command creation error')
