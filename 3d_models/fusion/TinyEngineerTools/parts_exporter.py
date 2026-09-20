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
COMMAND_DESCRIPTION = 'Export PRINT_LAYOUT children as 3MF, binary STL, and STEP'
WORKSPACE_ID = 'FusionSolidEnvironment'
PANEL_ID = 'SolidScriptsAddinsPanel'
PRINT_LAYOUT_NAME = 'PRINT_LAYOUT'
_INVALID_FILENAME_CHARS = '<>:"/\\|?*'
PARTS_TABLE_ID = 'parts_table'
SELECT_ALL_ID = 'select_all'
DESELECT_ALL_ID = 'deselect_all'
EXPORT_3MF_ID = 'export_3mf'
EXPORT_STL_ID = 'export_stl'
EXPORT_STEP_ID = 'export_step'
PARTS_VISIBLE_ROWS = 12

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


def _descendant_occurrences(occ):
    result = []
    for child in _occurrence_list(occ.childOccurrences):
        result.append(child)
        result.extend(_descendant_occurrences(child))
    return result


def _set_child_visible(children, visible_occ):
    for occ in children:
        occ.isLightBulbOn = False
    if visible_occ:
        visible_occ.isLightBulbOn = True
        for desc in _descendant_occurrences(visible_occ):
            desc.isLightBulbOn = True


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


def _export_dirs(folder, servo_id, want_stl, want_3mf, want_step):
    servo_dir = os.path.join(folder, servo_id)
    stl_dir = os.path.join(servo_dir, 'stl') if want_stl else None
    c3mf_dir = os.path.join(servo_dir, '3mf') if want_3mf else None
    step_dir = os.path.join(servo_dir, 'step') if want_step else None
    os.makedirs(servo_dir, exist_ok=True)
    if stl_dir:
        os.makedirs(stl_dir, exist_ok=True)
    if c3mf_dir:
        os.makedirs(c3mf_dir, exist_ok=True)
    if step_dir:
        os.makedirs(step_dir, exist_ok=True)
    return servo_dir, stl_dir, c3mf_dir, step_dir


def _export_stl(export_mgr, geometry, stl_dir, base):
    try:
        stl = export_mgr.createSTLExportOptions(
            geometry, os.path.join(stl_dir, f'{base}.stl')
        )
        stl.isBinaryFormat = True
        stl.sendToPrintUtility = False
        export_mgr.execute(stl)
        return []
    except Exception as exc:
        return [f'{base}.stl: {exc}']


def _export_3mf(export_mgr, geometry, c3mf_dir, base):
    try:
        c3mf = export_mgr.createC3MFExportOptions(
            geometry, os.path.join(c3mf_dir, f'{base}.3mf')
        )
        c3mf.sendToPrintUtility = False
        export_mgr.execute(c3mf)
        return []
    except Exception as exc:
        return [f'{base}.3mf: {exc}']


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


def _export_part_formats(
    export_mgr, geometry, base, stl_dir, c3mf_dir, step_dir,
    want_stl, want_3mf, want_step,
):
    errors = []
    if want_stl and stl_dir:
        errors.extend(_export_stl(export_mgr, geometry, stl_dir, base))
    if want_3mf and c3mf_dir:
        errors.extend(_export_3mf(export_mgr, geometry, c3mf_dir, base))
    if want_step and step_dir:
        errors.extend(_export_step(export_mgr, geometry, step_dir, base))
    return errors


def _servo_param_names(servos):
    names = {servo.SERVO_ID_PARAM}
    for data in servos.values():
        names.update(data.keys())
    return names


def _snapshot_params(design, names):
    snapshot = {}
    params = design.userParameters
    for name in names:
        param = params.itemByName(name)
        if not param:
            continue
        snapshot[name] = param.expression
    return snapshot


def _restore_params(design, snapshot):
    if not snapshot:
        return
    params = design.userParameters
    to_set = []
    values = []
    for name, expression in snapshot.items():
        param = params.itemByName(name)
        if not param:
            continue
        to_set.append(param)
        values.append(adsk.core.ValueInput.createByString(expression))
    if to_set:
        design.modifyParameters(to_set, values)
        design.computeAll()


def _bool_input(inputs, input_id):
    return adsk.core.BoolValueCommandInput.cast(inputs.itemById(input_id))


def _build_dialog(inputs, servos, children):
    servo_group = inputs.addGroupCommandInput('servos_group', 'Servos')
    servo_group.isExpanded = True
    servo_children = servo_group.children
    servo_checks = []
    for index, (name, data) in enumerate(servos.items()):
        check_id = f'servo_check_{index}'
        servo_children.addBoolValueInput(check_id, name, True, '', True)
        servo_checks.append((check_id, name, data))

    format_group = inputs.addGroupCommandInput('formats_group', 'Formats')
    format_group.isExpanded = True
    format_children = format_group.children
    format_children.addBoolValueInput(EXPORT_3MF_ID, '3MF', True, '', True)
    format_children.addBoolValueInput(EXPORT_STL_ID, 'STL', True, '', True)
    format_children.addBoolValueInput(EXPORT_STEP_ID, 'STEP', True, '', True)

    table = inputs.addTableCommandInput(PARTS_TABLE_ID, 'Parts', 2, '1:4')
    table.maximumVisibleRows = PARTS_VISIBLE_ROWS
    table.hasGrid = False

    select_all = inputs.addBoolValueInput(SELECT_ALL_ID, 'Select all', False, '', True)
    deselect_all = inputs.addBoolValueInput(
        DESELECT_ALL_ID, 'Deselect all', False, '', True
    )
    table.addToolbarCommandInput(select_all)
    table.addToolbarCommandInput(deselect_all)

    part_checks = []
    used_names = set()
    for index, occ in enumerate(children):
        check_id = f'part_check_{index}'
        name_id = f'part_name_{index}'
        display = _unique_base(occ, used_names)
        check = inputs.addBoolValueInput(check_id, '', True, '', True)
        label = inputs.addStringValueInput(name_id, '', display)
        label.isReadOnly = True
        row = table.rowCount
        table.addCommandInput(check, row, 0)
        table.addCommandInput(label, row, 1)
        part_checks.append((check_id, occ, display))

    return servo_checks, part_checks


def _set_part_checks(inputs, part_checks, value):
    for check_id, _, _ in part_checks:
        check = _bool_input(inputs, check_id)
        if check:
            check.value = value


def _selected_servos(inputs, servo_checks):
    selected = []
    for check_id, name, data in servo_checks:
        check = _bool_input(inputs, check_id)
        if check and check.value:
            selected.append((name, data))
    return selected


def _selected_parts(inputs, part_checks):
    selected = []
    for check_id, occ, display in part_checks:
        check = _bool_input(inputs, check_id)
        if check and check.value:
            selected.append((occ, display))
    return selected


def _selected_formats(inputs):
    want_3mf = _bool_input(inputs, EXPORT_3MF_ID)
    want_stl = _bool_input(inputs, EXPORT_STL_ID)
    want_step = _bool_input(inputs, EXPORT_STEP_ID)
    return (
        bool(want_3mf and want_3mf.value),
        bool(want_stl and want_stl.value),
        bool(want_step and want_step.value),
    )


def _prepare_visibility(geometry, children, layout_component):
    occ_state = [(occ, occ.isLightBulbOn) for occ in children]
    for occ in children:
        for desc in _descendant_occurrences(occ):
            occ_state.append((desc, desc.isLightBulbOn))
    bodies = _body_list(layout_component)
    body_state = [(body, body.isLightBulbOn) for body in bodies]
    folder_on = layout_component.isBodiesFolderLightBulbOn

    layout_occ = adsk.fusion.Occurrence.cast(geometry)
    layout_path = _occurrence_ancestors(layout_occ) if layout_occ else []
    layout_state = [(occ, occ.isLightBulbOn) for occ in layout_path]

    for occ, _ in layout_state:
        occ.isLightBulbOn = True
    layout_component.isBodiesFolderLightBulbOn = False
    for body, _ in body_state:
        body.isLightBulbOn = False
    _set_child_visible(children, None)

    return occ_state, body_state, folder_on, layout_state


def _export_parts(inputs, servos, servo_checks, part_checks, all_children):
    design = adsk.fusion.Design.cast(app.activeProduct)
    if not design:
        ui.messageBox('Open a Fusion design first.')
        return

    selected_servos = _selected_servos(inputs, servo_checks)
    selected_parts = _selected_parts(inputs, part_checks)
    want_3mf, want_stl, want_step = _selected_formats(inputs)

    if not selected_servos:
        ui.messageBox('Select at least one servo.')
        return
    if not selected_parts:
        ui.messageBox('Select at least one part.')
        return
    if not (want_3mf or want_stl or want_step):
        ui.messageBox('Select at least one format.')
        return

    folder = _choose_folder()
    if not folder:
        return

    geometry, _children, layout_component = _find_print_layout(design)
    if geometry is None or layout_component is None:
        ui.messageBox(f'Component "{PRINT_LAYOUT_NAME}" was not found.')
        return

    # Use occurrences from CommandCreated — Fusion re-wraps on a second
    # _find_print_layout, so Python id() matching always fails.
    export_parts = selected_parts

    snapshot = _snapshot_params(design, _servo_param_names(servos))
    export_mgr = design.exportManager
    exported = 0
    errors = []
    cancelled = False
    export_roots = []

    total_steps = len(selected_servos) * len(export_parts)
    progress = ui.createProgressDialog()
    progress.isBackgroundTranslucent = False
    progress.isCancelButtonShown = True

    occ_state = body_state = folder_on = layout_state = None
    visibility_ready = False

    try:
        progress.show(
            'Tiny Engineer Parts Exporter', 'Exporting %v / %m', 0, total_steps, 1
        )
        step_index = 0

        for servo_name, servo_data in selected_servos:
            adsk.doEvents()
            if progress.wasCancelled:
                cancelled = True
                break

            raw_id = (servo_data.get(servo.SERVO_ID_PARAM) or '').strip()
            if not raw_id:
                errors.append(f'{servo_name}: missing servo_id')
                step_index += len(export_parts)
                progress.progressValue = min(step_index, total_steps)
                continue

            if not servo._apply_servo(design, servo_data, show_errors=False):
                errors.append(f'{servo_name}: failed to apply parameters')
                step_index += len(export_parts)
                progress.progressValue = min(step_index, total_steps)
                continue

            servo_id = _safe_filename(raw_id)
            servo_dir, stl_dir, c3mf_dir, step_dir = _export_dirs(
                folder, servo_id, want_stl, want_3mf, want_step
            )
            _write_servo_readme(servo_dir, servo_name, servo_data)
            export_roots.append(servo_dir)

            if not visibility_ready:
                occ_state, body_state, folder_on, layout_state = _prepare_visibility(
                    geometry, all_children, layout_component
                )
                visibility_ready = True

            for occ, display in export_parts:
                adsk.doEvents()
                if progress.wasCancelled:
                    cancelled = True
                    break

                progress.progressValue = step_index
                progress.message = f'{servo_id}: {display} (%v / %m)'
                step_index += 1

                _set_child_visible(all_children, occ)
                part_errors = _export_part_formats(
                    export_mgr,
                    geometry,
                    display,
                    stl_dir,
                    c3mf_dir,
                    step_dir,
                    want_stl,
                    want_3mf,
                    want_step,
                )
                if part_errors:
                    errors.extend(
                        f'{servo_id}/{msg}' for msg in part_errors
                    )
                else:
                    exported += 1
                occ.isLightBulbOn = False
                adsk.doEvents()

            if cancelled:
                break

        if not cancelled:
            progress.progressValue = total_steps
    finally:
        progress.hide()
        if visibility_ready:
            _restore_visibility(
                occ_state + layout_state, body_state, layout_component, folder_on
            )
        _restore_params(design, snapshot)

    if export_roots:
        dest = folder if len(export_roots) > 1 else export_roots[0]
        lines = [f'Exported {exported} part file set(s) to:\n{dest}']
    else:
        lines = ['No parts exported.']
    if cancelled:
        lines.append('Export cancelled.')
    if errors:
        lines.append('Failures:\n' + '\n'.join(errors))
    ui.messageBox('\n\n'.join(lines))


class InputChangedHandler(adsk.core.InputChangedEventHandler):
    def __init__(self, part_checks):
        super().__init__()
        self.part_checks = part_checks

    def notify(self, args):
        try:
            input_id = args.input.id
            inputs = args.firingEvent.sender.commandInputs
            if input_id == SELECT_ALL_ID:
                _set_part_checks(inputs, self.part_checks, True)
                button = _bool_input(inputs, SELECT_ALL_ID)
                if button:
                    button.value = False
            elif input_id == DESELECT_ALL_ID:
                _set_part_checks(inputs, self.part_checks, False)
                button = _bool_input(inputs, DESELECT_ALL_ID)
                if button:
                    button.value = False
        except Exception:
            _error('Input error')


class ExecuteHandler(adsk.core.CommandEventHandler):
    def __init__(self, servos, servo_checks, part_checks, all_children):
        super().__init__()
        self.servos = servos
        self.servo_checks = servo_checks
        self.part_checks = part_checks
        self.all_children = all_children

    def notify(self, args):
        try:
            inputs = args.firingEvent.sender.commandInputs
            _export_parts(
                inputs,
                self.servos,
                self.servo_checks,
                self.part_checks,
                self.all_children,
            )
        except Exception:
            _error('Execute error')


class CommandCreatedHandler(adsk.core.CommandCreatedEventHandler):
    def notify(self, args):
        try:
            design = adsk.fusion.Design.cast(app.activeProduct)
            if not design:
                ui.messageBox('Open a Fusion design first.')
                return

            geometry, children, _layout = _find_print_layout(design)
            if geometry is None:
                ui.messageBox(f'Component "{PRINT_LAYOUT_NAME}" was not found.')
                return
            if not children:
                ui.messageBox(f'"{PRINT_LAYOUT_NAME}" has no children to export.')
                return

            servos = servo._load_servos()
            if not servos:
                ui.messageBox('No servo definitions found in servos.json.')
                return

            command = args.command
            servo_checks, part_checks = _build_dialog(
                command.commandInputs, servos, children
            )

            input_handler = InputChangedHandler(part_checks)
            command.inputChanged.add(input_handler)
            _handlers.append(input_handler)

            execute_handler = ExecuteHandler(
                servos, servo_checks, part_checks, children
            )
            command.execute.add(execute_handler)
            _handlers.append(execute_handler)
        except Exception:
            _error('Command creation error')
