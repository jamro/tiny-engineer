"""Servo model picker. Writes dimensions into Fusion user parameters."""

import json
import os
import traceback

import adsk.core
import adsk.fusion

app = adsk.core.Application.get()
ui = app.userInterface

COMMAND_ID = 'TinyEngineerTools'
COMMAND_NAME = 'TinyEngineer Servo Configurator'
COMMAND_DESCRIPTION = 'Select servo model and apply dimensions'
WORKSPACE_ID = 'FusionSolidEnvironment'
PANEL_ID = 'SolidScriptsAddinsPanel'
SERVO_ID_PARAM = 'servo_id'

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
        control = panel.controls.addCommand(cmd_def)

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


def _load_servos():
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'servos.json')
    with open(path, encoding='utf-8') as file:
        return json.load(file)


def _display_name(parameter_name):
    if parameter_name.startswith('servo_'):
        parameter_name = parameter_name[len('servo_'):]
    return parameter_name.replace('_', ' ').title()


def _selected_servo(inputs, servos):
    dropdown = adsk.core.DropDownCommandInput.cast(
        inputs.itemById('servo_model')
    )
    if not dropdown or not dropdown.selectedItem:
        return None
    return servos[dropdown.selectedItem.name]


def _update_preview(inputs, servos):
    servo = _selected_servo(inputs, servos)
    if not servo:
        return

    for parameter_name, value in servo.items():
        preview = adsk.core.TextBoxCommandInput.cast(
            inputs.itemById(f'preview_{parameter_name}')
        )
        if preview:
            preview.text = value


def _build_dialog(inputs, servos):
    dropdown = inputs.addDropDownCommandInput(
        'servo_model',
        'Servo model',
        adsk.core.DropDownStyles.TextListDropDownStyle,
    )
    for index, servo_name in enumerate(servos):
        dropdown.listItems.add(servo_name, index == 0, '')

    first_servo = next(iter(servos.values()))
    for parameter_name in first_servo:
        inputs.addTextBoxCommandInput(
            f'preview_{parameter_name}',
            _display_name(parameter_name),
            '',
            1,
            True,
        )

    _update_preview(inputs, servos)


def _apply_servo_id(params, value):
    existing = params.itemByName(SERVO_ID_PARAM)
    if existing:
        try:
            existing.textValue = value
            return
        except Exception:
            pass
        try:
            existing.expression = f"'{value}'"
            return
        except Exception:
            existing.deleteMe()

    params.add(
        SERVO_ID_PARAM,
        adsk.core.ValueInput.createByString(f"'{value}'"),
        'Text',
        'Selected servo id',
    )


def _apply_servo(design, servo_data):
    params = design.userParameters
    dimensions = {
        name: value
        for name, value in servo_data.items()
        if name != SERVO_ID_PARAM
    }
    missing = [name for name in dimensions if not params.itemByName(name)]
    if missing:
        ui.messageBox(
            'The following Fusion parameters do not exist:\n\n'
            + '\n'.join(missing)
        )
        return

    servo_id = servo_data.get(SERVO_ID_PARAM)
    if servo_id:
        _apply_servo_id(params, servo_id)

    for name, value in dimensions.items():
        params.itemByName(name).expression = value


def _error(title):
    ui.messageBox(f'{title}:\n\n{traceback.format_exc()}')


class InputChangedHandler(adsk.core.InputChangedEventHandler):
    def __init__(self, servos):
        super().__init__()
        self.servos = servos

    def notify(self, args):
        try:
            if args.input.id == 'servo_model':
                _update_preview(args.firingEvent.sender.commandInputs, self.servos)
        except Exception:
            _error('Input error')


class ExecuteHandler(adsk.core.CommandEventHandler):
    def __init__(self, servos):
        super().__init__()
        self.servos = servos

    def notify(self, args):
        try:
            design = adsk.fusion.Design.cast(app.activeProduct)
            if not design:
                ui.messageBox('Open a Fusion design first.')
                return

            servo_data = _selected_servo(
                args.firingEvent.sender.commandInputs, self.servos
            )
            if not servo_data:
                ui.messageBox('No servo model selected.')
                return

            _apply_servo(design, servo_data)
        except Exception:
            _error('Execute error')


class CommandCreatedHandler(adsk.core.CommandCreatedEventHandler):
    def notify(self, args):
        try:
            servos = _load_servos()
            if not servos:
                ui.messageBox('No servo definitions found in servos.json.')
                return

            command = args.command
            _build_dialog(command.commandInputs, servos)

            input_handler = InputChangedHandler(servos)
            command.inputChanged.add(input_handler)
            _handlers.append(input_handler)

            execute_handler = ExecuteHandler(servos)
            command.execute.add(execute_handler)
            _handlers.append(execute_handler)
        except Exception:
            _error('Command creation error')
