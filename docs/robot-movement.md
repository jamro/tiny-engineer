# Robot layout and movement

The robot sits in a chair beside a desk. A miniature laptop and a service bell sit on the desk in front of it. Five micro servos drive the pose; limits and channel IDs are in [`include/servos.h`](../include/servos.h).

| Index | Servo | Motion | At `min` | At `max` | Safe range (°) |
|------:|-------|--------|----------|----------|----------------|
| 0 | **Head** | Face up / down (pitch) | down | up | 60 – 130 |
| 1 | **Neck** | Head left / right (yaw) | right | left | 40 – 130 |
| 2 | **Left hand** | Hand up / down | down | up | 50 – 140 |
| 3 | **Right hand** | Hand up / down (inverted scale) | up | down | 40 – 130 |
| 4 | **Body** | Whole torso left / right | right | left | 40 – 130 |

**Head** tilts the face toward or away from the laptop. **Neck** pans the head side to side. **Hands** lift and lower over the keyboard; the forearms have no elbow servo — the arm linkage is fixed, so only the hand joint moves. **Left** and **right** hand servos use opposite scales: on the left, higher angle is up; on the right, higher angle is down (rest pose for typing is left at `max`, right at `min`). **Body** rotates the whole upper body in the chair while the base stays put.

Command a single joint for bench checks:

```bash
curl -X POST "http://tiny-engineer.local/test/servo?index=0&angle=90"
```

Use angles inside the safe range above on the assembled robot.

Related: [servos.md](hardware/servos.md) (PWM and electrical limits), [api.md](api.md) (`/test/servo`, `/anim`).
