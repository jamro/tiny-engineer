---
name: tiny-engineer-play
description: Make the Tiny Engineer desk robot say something out loud through its speaker while it gestures, via POST /play. Use whenever the user asks the robot to speak, announce or celebrate a result, greet them, or "tell me through the robot", and when debugging /play errors such as 415 "expected 16-bit mono PCM" or 400 "name must be ...".
---

# Tiny Engineer: speak through the robot

The robot plays a WAV clip on its speaker while it runs an animation, then goes back to what it was doing. Clips must be 16-bit mono PCM at 22050 Hz.

## 1. Make the clip

Use any text-to-speech the machine has, then convert it with ffmpeg:

```bash
espeak-ng -w /tmp/te-raw.wav "Build passed. Ship it!"                                        # Linux
say --file-format=WAVE --data-format=LEI16@22050 -o /tmp/te-raw.wav "Build passed. Ship it!"  # macOS
ffmpeg -y -loglevel error -i /tmp/te-raw.wav -ar 22050 -ac 1 -c:a pcm_s16le /tmp/te-clip.wav
```

Keep clips short (a few seconds up to about 20 s): the robot handles one request at a time, so the hooks that animate it during normal work wait while a clip plays.

## 2. Play it

```bash
<this skill's base directory>/scripts/play /tmp/te-clip.wav            # robot runs "talking"
<this skill's base directory>/scripts/play /tmp/te-clip.wav thinking   # or typing, reading, none
```

It reads `TINY_ENGINEER_URL` (default `http://tiny-engineer.local`) and `TINY_ENGINEER_TOKEN` from the environment, prints the robot's JSON reply when the clip ends, and exits 1 with the reason on any error. It needs `curl`.

## Errors

| Reply | Fix |
| --- | --- |
| `415 expected 16-bit mono PCM at N Hz` | Re-run the `ffmpeg` step with `-ar N` |
| `400 name must be ...` | Use `talking`, `typing`, `reading`, `thinking` or `none` |
| `401 unauthorized` | Set `TINY_ENGINEER_TOKEN` to the robot's `access_token` |
| `robot unreachable` | Check `TINY_ENGINEER_URL`; the robot must be on the same network |
