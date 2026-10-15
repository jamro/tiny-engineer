# Robot layout and movement

The robot sits in a chair beside a desk. A miniature laptop and a service bell sit on the desk in front of it. Five micro servos drive the pose; limits and channel IDs are in [`include/servos.h`](../include/servos.h).

| Index | Servo | Motion | At `min` | At `max` | Safe range (°) |
|------:|-------|--------|----------|----------|----------------|
| 0 | **Head** | Face up / down (pitch) | down | up | 60 – 130 |
| 1 | **Neck** | Head left / right (yaw) | right | left | 40 – 130 |
| 2 | **Left hand** | Hand up / down | down | up | 45 – 135 |
| 3 | **Right hand** | Hand up / down (inverted scale) | up | down | 35 – 125 |
| 4 | **Body** | Whole torso left / right | right | left | 40 – 130 |

Ranges match stock `SERVO_SPECS` in [`include/servos.h`](../include/servos.h). Those values are **defaults**. Calibration is stored in NVS. Factory reset keeps the saved ranges and centers. Retune limits and centers from the regular Servo control page or the setup wizard.

Animations author poses in **−1..1** relative to each joint’s saved min/center/max (`−1` = min, `0` = calibrated center, `+1` = max). `servoNormToDeg` interpolates separately from min to center and from center to max, so correcting the center never widens the safe band. Without a saved center, the firmware uses `(min + max) / 2`, preserving older calibrations. Right-hand rest is still electrical min (`−1`); left-hand rest is electrical max (`+1`).

**Head** tilts the face toward or away from the laptop. **Neck** pans the head side to side. **Hands** lift and lower over the keyboard; the forearms have no elbow servo — the arm linkage is fixed, so only the hand joint moves. **Left** and **right** hand servos use opposite scales: on the left, higher angle is up; on the right, higher angle is down (rest pose for typing is left at `max`, right at `min`). **Body** rotates the whole upper body in the chair while the base stays put.

## Hand servo assignment

If selecting Left hand moves the right hand, open **Config** and use **Hand servo assignment**. Choose the channel connected to the robot’s left hand (from the robot’s perspective); the other hand automatically uses the remaining channel:

| Left hand channel | Right hand channel | `swap_hands` |
| --- | --- | --- |
| 2 (default) | 3 | `false` |
| 3 (swapped) | 2 | `true` |

Press **Save hand assignment**, then restart the robot before testing either hand or calibrating its range. This selection is also available at the start of the setup wizard. The saved mapping survives power-off and factory reset. Existing limits and centers still belong to the logical left/right joints; verify both hands’ calibration after changing the mapping. All PWM commands, including setup moves and output release, use the chosen channels after reboot.

```bash
curl -X POST "http://tiny-engineer.local/settings" -H "Content-Type: application/json" \
  -d '{"swap_hands":true}'
```

## Movement limits

1. Open **Servo control** and select the joint.
2. Under **Servo limits**, enter **Min (°)** and **Max (°)** as whole degrees from 0 to 180, with min less than max. Use limits you have checked on the assembled robot.
3. Press **Save limits**. The slider and subsequent movements use the saved range immediately, without reboot. Saving does not start a test move.

Only the selected joint's limits change. Its center stays where it is if it fits strictly inside the new range; otherwise the UI shows the new midpoint before saving it as the center. Other joints keep their saved calibration. Limits survive power-off and factory reset.

Changing the fields alone does not move the servo or change its active limits. Save first, then use **Move servo** to test within the new range. To explore the full electrical range during assembly, use the setup wizard.

## Center calibration

1. Open **Servo control** (`http://tiny-engineer.local/servo`) and select Body, Left hand, Right hand, Head, or Neck.
2. Adjust Angle within the saved safe range and press **Move servo** to inspect alignment. For the body, face the torso toward the laptop; for a hand, choose the middle of its intended movement.
3. Press **Save angle as center**. The chosen joint's center is saved to NVS without changing its min/max. Repeat for other joints.
4. **Center** moves to the saved center; **Reset center to midpoint** saves the midpoint of the current limits again.

Centers take effect on subsequent normalized motion commands without reboot. Saving a center does not itself move a joint. Calibration survives power-off and factory reset. Centers must be strictly between min and max; fractional degrees are supported. The setup wizard also offers **Set center** after finding each joint's limits.

Hand rest poses use the safe endpoints, so center calibration changes intermediate hand poses, not where the hands rest. If a hand is misaligned at rest, retune the corresponding min or max under **Servo limits**.

The API accepts all five centers in `[head, neck, left, right, body]` order; keep the other saved values from `GET /settings` when changing one joint:

```bash
curl -X POST "http://tiny-engineer.local/settings" -H "Content-Type: application/json" \
  -d '{"servo_centers":[95,85,92,78,90]}'
```

Command a single joint for bench checks:

```bash
curl -X POST "http://tiny-engineer.local/test/servo" -H "Content-Type: application/json" -d '{"index":0,"angle":90}'
```

Use angles inside the saved safe range on the assembled robot. During setup AP, `POST /setup/servo` can command the full 0–180° electrical range so you can find those limits.

Related: [servos.md](hardware/servos.md) (PWM and electrical limits), [api.md](api.md) (`/test/servo`, `/anim`).
