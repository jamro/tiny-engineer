# Servos and mechanical motion

**Position servos** are everywhere in hobby robotics — small actuators that go to an angle and hold. They're not stepper motors, not DC gearmotors. Understanding them saves you from stripped gears and firmware angles that mean nothing physically. Tiny Engineer has **five** of them; the mechanics chapter applies to any servo-driven joint.

---

## What a hobby servo is

A **position servo** (specifically the **PowerHD HD-1370A** micro servos in this build) contains:

- Motor
- Gear train
- Potentiometer (position feedback)
- Control board that compares commanded pulse width vs actual angle

You command an **angle** (via pulse width). The servo tries to reach it and **holds** there — even against light load.

**Not** continuous rotation — it stops at mechanical limits (~130° usable on these units, marketed sometimes as 180°).

**Three wires:**
| Wire | Role |
| --- | --- |
| Signal (SIG) | PWM in — angle command |
| +5 V (red) | Motor power |
| GND (brown/black) | Common ground |

Power and signal are separate. The signal wire is low current; the power wire carries motor current.

PWM here means a timed HIGH pulse on the signal wire — how long that HIGH lasts is the angle. Full protocol picture (and why a PCA9685 does it): [Ch. 04 — Buses and protocols](04-buses-and-protocols.md#pwm--how-long-high-lasts-is-the-angle).

---

## PWM as angle — recap

Short version (details in [Ch. 04](04-buses-and-protocols.md#pwm--how-long-high-lasts-is-the-angle)): ~50 Hz frame, pulse width sets target:

- ~1500 µs ≈ center / neutral
- Shorter pulse → one direction
- Longer pulse → other direction

Firmware maps 0–180° to **800–2200 µs** on this project. That's the *electrical* range. The *mechanical* range in the assembled robot is narrower — see below.

---

## Horns, splines, and alignment

On this robot (and most 3D-printed hobby builds) the **servo body sits in a printed pocket** — screws or a tight fit, not a metal bracket. The printed part *is* the mount. The moving piece is whatever is on the **output shaft**.

**Spline** — toothed shaft on the servo. **Horn** — the small plastic arm (or a printed piece that mates the same teeth) that locks onto that spline. Teeth only line up in discrete steps — one tooth off and the rest pose is wrong.

**Typical order:**
1. Seat the servo in the printed pocket (don't force — reprint or ream if it's a press-fit from hell)
2. Power to **center** (firmware boot centers slowly on purpose)
3. Press the horn / printed lever onto the spline at the **neutral pose** for that joint (hand down, head looking at the laptop, etc.)
4. Horn screw from the servo bag — snug, not stripped

The printed pocket holds the **case**. The horn (or printed horn-shaped part) transfers **rotation** into the next printed piece. Misalign the spline and the hand hits the desk at "90°" in firmware.

**Off-by-one spline** — one tooth wrong → rest pose crooked → you clamp in software until you hit a printed stop and gears slip.

> **If you've written backend code…** Horn alignment is the zero offset. Wrong offset corrupts everything downstream. Software clamps don't fix a printed stop.

---

## Mechanical limits vs firmware limits

Two layers of limits:

| Layer | What limits | Where defined |
| --- | --- | --- |
| **Electrical** | 800–2200 µs pulse | Firmware constants |
| **Mechanical** | Horn hits shell, linkage binds, servo stalls | Your assembly |
| **Safe software** | Clamped angles per joint | `SERVO_SPECS` in firmware |

Always respect **safe ranges** for the assembled robot — [robot-movement.md](../robot-movement.md). Bench-only wider sweeps (75–105°) exist for testing; don't slam full electrical range into a built shell.

**Tune after assembly:** if a hand hits the desk at 135°, lower `max` in firmware or reorient the horn — don't "just command less" in one script and forget.

---

## Backlash, binding, stall

**Backlash** — gear slack; small commanded moves may not visibly move until slack is taken up. Normal at micro scale.

**Binding** — linkage too tight, screw into moving path, printed hole too small. Servo buzzes, draws stall current, gets hot.

**Stall** — servo commanded past mechanical stop. High current (~260–320 mA per servo at stall). Five servos stalling together browns out the board. Hear buzzing? Stop commanding into the stop.

---

## The Tiny Engineer servo stack

End-to-end path from HTTP to motion:

```mermaid
flowchart LR
  HTTP["POST /test/servo"]
  FW["Firmware angle clamp"]
  I2C["I2C to PCA9685 0x40"]
  PWM["50 Hz PWM ch 0-4"]
  SRV["HD-1370A servos"]
  HTTP --> FW --> I2C --> PWM --> SRV
```

1. You hit `/test/servo` or an animation (see [api.md](../api.md))
2. Firmware clamps angle to `SERVO_SPECS` safe range for that joint
3. I2C write to PCA9685 at address `0x40`
4. Chip outputs 50 Hz PWM on channel 0–4
5. Servo moves

**Channel → joint mapping** (names only — angles in [robot-movement.md](../robot-movement.md)):

| Channel | Joint | Motion |
| --- | --- | --- |
| 0 | Head | Pitch (nod) |
| 1 | Neck | Yaw (turn) |
| 2 | Left hand | Up/down |
| 3 | Right hand | Up/down |
| 4 | Body | Torso twist |

### Left vs right hand — inverted scale

The left and right hand servos mirror each other physically. Firmware uses **opposite angle mapping**: on the left, higher angle = up; on the right, higher angle = down. Rest pose for "typing" uses left at max, right at min.

Symmetric code, asymmetric hardware — normal in robots.

### Boot centering — slow on purpose

On boot, servos move to center at **35°/s** (`SERVO_BOOT_SPEED_DEG_S`), not full speed. Sudden jumps on power-up strip gears and scare you. Live motion can be faster — see [servos.md](../hardware/servos.md).

### PCA9685 is required

No PCA9685 at `0x40` → init fails, red LED, hang. Unlike the OLED. Plan bring-up accordingly.

---

## Bench testing before closing the shell

1. Mount one servo in loose fixture or hand-hold (careful — pinching)
2. Flash firmware, verify serial shows PCA9685 OK
3. Command one joint via web UI or `/test/servo`
4. Sweep through **safe range** slowly — watch for binding
5. Repeat for all five before screwing into final plastic

Mechanical integration last. Software can clamp angles; it can't un-strip a gear.

---

**Next:** [3D printing and mechanical build](06-3d-printing-and-mechanical-build.md)

**Reference:** [robot-movement.md](../robot-movement.md) · [hardware/servos.md](../hardware/servos.md) · [api.md](../api.md)
