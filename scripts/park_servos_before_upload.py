Import("env")

"""Pre-upload: POST /servos/park over serial so joints sit at mid before reset.

Warns and continues on any failure (cold board, wrong port, busy CDC).
"""

import json
import time


# Espressif native USB (ESP32-C3). Docs: flash.md
_ESPRESSIF_VID = 0x303A
_PROBE_TIMEOUT_S = 1.5
_PARK_TIMEOUT_S = 15.0


def _warn(msg):
    print("park_servos_before_upload: %s" % msg)


def _is_junk_port(path):
    lower = path.lower()
    return lower.endswith(
        (
            "bluetooth-incoming-port",
            "wlan-debug",
            "debug-console",
        )
    ) or "bluetooth" in lower


def _resolve_port(env):
    configured = env.subst("$UPLOAD_PORT").strip()
    if configured:
        return configured

    try:
        from platformio.device.finder import SerialPortFinder

        port = SerialPortFinder(
            upload_protocol=env.subst("$UPLOAD_PROTOCOL") or None,
            ensure_ready=True,
            timeout=2,
        ).find(initial_port=None)
        if port and not _is_junk_port(port):
            return port
    except Exception:
        pass

    try:
        from serial.tools.list_ports import comports

        ports = [p for p in comports() if p.device and not _is_junk_port(p.device)]
    except Exception:
        return None

    if not ports:
        return None

    for p in ports:
        if p.vid == _ESPRESSIF_VID:
            return p.device

    for p in ports:
        if "usbmodem" in p.device.lower() or "usbserial" in p.device.lower():
            return p.device

    return ports[0].device


def _read_reply(ser, expect_id, deadline):
    buf = ""
    while time.time() < deadline:
        chunk = ser.read(256)
        if not chunk:
            time.sleep(0.05)
            continue

        buf += chunk.decode("utf-8", errors="replace")
        while "\n" in buf:
            raw, buf = buf.split("\n", 1)
            raw = raw.strip()
            if not raw.startswith("R"):
                continue
            try:
                reply = json.loads(raw[1:])
            except json.JSONDecodeError:
                continue
            if reply.get("id") != expect_id:
                continue
            return reply

    return None


def _serial_rpc(ser, req_id, op, timeout_s):
    line = json.dumps(
        {"v": 1, "id": req_id, "op": op, "q": {}},
        separators=(",", ":"),
    ) + "\n"
    ser.reset_input_buffer()
    ser.write(line.encode("utf-8"))
    ser.flush()
    return _read_reply(ser, req_id, time.time() + timeout_s)


def park_servos(source, target, env):
    try:
        import serial
    except ImportError:
        _warn("pyserial missing; skip park (upload continues)")
        return

    port = _resolve_port(env)
    if not port:
        _warn("no serial port; skip park (upload continues)")
        return

    try:
        baud = int(env.subst("$MONITOR_SPEED") or "115200")
    except Exception:
        baud = 115200

    try:
        ser = serial.Serial(port, baudrate=baud, timeout=0.2)
    except Exception as exc:
        _warn("open %s failed (%s); skip park" % (port, exc))
        return

    try:
        try:
            time.sleep(0.1)
            probe = _serial_rpc(ser, 1, "GET /health", _PROBE_TIMEOUT_S)
            if probe is None:
                _warn(
                    "no serial API on %s (%.1fs); skip park"
                    % (port, _PROBE_TIMEOUT_S)
                )
                return

            if probe.get("status") != 200:
                _warn(
                    "health status=%s on %s; skip park"
                    % (probe.get("status"), port)
                )
                return

            reply = _serial_rpc(ser, 2, "POST /servos/park", _PARK_TIMEOUT_S)
            if reply is None:
                _warn("park timeout on %s; upload continues" % port)
                return

            body = reply.get("body") or {}
            if reply.get("status") == 200 and body.get("ok") is True:
                print("park_servos_before_upload: parked on %s" % port)
                time.sleep(0.3)
            else:
                _warn(
                    "park failed status=%s body=%s; upload continues"
                    % (reply.get("status"), body)
                )
        except Exception as exc:
            _warn("serial I/O on %s failed (%s); skip park" % (port, exc))
    finally:
        try:
            ser.close()
        except Exception:
            pass


env.AddPreAction("upload", park_servos)
