"""TinyEngineer Tools Add-In."""

import os
import sys
import traceback

import adsk.core

_ADDIN_DIR = os.path.dirname(os.path.abspath(__file__))
if _ADDIN_DIR not in sys.path:
    sys.path.insert(0, _ADDIN_DIR)

import servo

ui = adsk.core.Application.get().userInterface


def run(context):
    try:
        servo.start()
    except Exception:
        ui.messageBox(f'Add-In startup error:\n\n{traceback.format_exc()}')


def stop(context):
    try:
        servo.stop()
    except Exception:
        ui.messageBox(f'Add-In shutdown error:\n\n{traceback.format_exc()}')
