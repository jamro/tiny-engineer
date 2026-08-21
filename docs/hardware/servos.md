# Servos

## Hardware

| Item | Value |
| --- | --- |
| Model | PowerHD HD-1370A analog micro |
| Count | 5 |
| Driver | Adafruit PCA9685, I2C `0x40` |
| Servo supply | PCA9685 **V+** ← drawing: USB **5V** → PCA9685 **5V** (same servo rail as **V+**; not **VCC**) |
| PWM frequency | **50 Hz** (`pwm.setPWMFreq(50)`) |
| Channels | 0, 1, 2, 3, 4 (`SERVO_CHANNELS`) |

> [!WARNING]
> Never power the servos from the ESP32 3.3 V regulator.

## Electrical PWM range

Manufacturer pulse window used in firmware:

| Constant | Value |
| --- | --- |
| `SERVO_MIN_US` | **800** µs |
| `SERVO_MAX_US` | **2200** µs |
| Neutral (typical) | ~1500 µs (90° in the firmware 0–180 map) |

`angleToPulse()` in [`src/pca9685_servos.cpp`](../../src/pca9685_servos.cpp) maps firmware angle **0–180°** linearly onto 800–2200 µs, then onto PCA9685 12-bit counts assuming a **20 000 µs** period:

`counts = pulse_us * 4096 / 20000`

Datasheet travel over 800–2200 µs is **approximately 130°**, while many sellers list 0–180°. Firmware still uses a 0–180 mathematical scale. That scale is **not** a promise of mechanical 180° in the robot.

## Current firmware test limits

Verified in [`include/pins.h`](../../include/pins.h):

| Constant | Value | Role |
| --- | --- | --- |
| `SERVO_LOW` | **75.0** | Test lower angle |
| `SERVO_CENTER` | **90.0** | Neutral / start / end |
| `SERVO_HIGH` | **105.0** | Test upper angle |
| `SERVO_STEP_MS` | 20 | Interpolation step |

Bring-up motion (`runServoTest`):

1. All channels → 90°
2. Smooth 90° → 105° (1 s)
3. Smooth 105° → 75° (2 s)
4. Smooth 75° → 90° (1 s)

All five channels get the **same** angle. This is a wiring/power test, not a pose library.

## Three ranges (do not collapse them)

| # | Range | Meaning | Current status |
| --- | --- | --- | --- |
| 1 | Electrical PWM | Pulse widths the servo electronics accept (~800–2200 µs @ 50 Hz) | In firmware |
| 2 | Nominal manufacturer angle | Marketing / datasheet travel (0–180° **or** ~130° over full pulse — sources disagree) | Do not trust for installed mechanics |
| 3 | Mechanical safe range | After horns, linkages, and collisions exist | **TBD per servo** |

Firmware must eventually command **range 3**, clipped inside range 1.

Blind 0–180° on the assembled robot can stall gears, tear horns, or brown out the 5 V rail.

## Channel-to-mechanism mapping

**TBD** — not in firmware, not on the wiring sketch (sketch shows one generic servo).

Proposed logical names for when mapping is measured:

```text
SERVO_HEAD_YAW
SERVO_HEAD_PITCH
SERVO_ARM_LEFT
SERVO_ARM_RIGHT
SERVO_LEG_BELL
```

Record channel, direction (invert), and safe angles together. Do not invent the table here.

## Future per-servo config

Store calibrated limits per channel, conceptually:

```cpp
struct ServoConfig {
    uint8_t channel;
    float minAngle;
    float centerAngle;
    float maxAngle;
};
```

Optional later fields (not in repo today): pulse override, invert, name.

Until that exists, keep using the **75 / 90 / 105** test band on the bench, then replace with measured min/center/max after the mechanism is mounted.

Related: [power.md](power.md), [wiring.md](wiring.md), [testing.md](testing.md).
