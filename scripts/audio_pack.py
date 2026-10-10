"""Merge stock WAVs with an optional mod overlay and halve their rate for the LittleFS image."""

import math
import re
import shutil
import wave
from array import array
from pathlib import Path

SOURCE_RATE = 44100
PACKED_RATE = SOURCE_RATE // 2

STOCK_WAVS = (
    "bell.wav",
    "welcome.wav",
    "attention.wav",
    "error.wav",
    "abort.wav",
    "dead.wav",
)

# Phrase marks the firmware already uses. A replaced clip must ship every key.
# blink_window is (start_key, end_key): the blink must sit inside that span.
CUE_CLIPS = {
    "welcome": {
        "keys": (
            "greeting_end_ms",
            "pause_end_ms",
            "blink_start_ms",
            "blink_end_ms",
            "question_end_ms",
            "end_ms",
        ),
        "increasing": (
            "greeting_end_ms",
            "pause_end_ms",
            "question_end_ms",
            "end_ms",
        ),
        "blink_window": ("greeting_end_ms", "pause_end_ms"),
    },
    "attention": {
        "keys": (
            "pst_end_ms",
            "human_end_ms",
            "blink_start_ms",
            "blink_end_ms",
            "end_ms",
        ),
        "increasing": (
            "pst_end_ms",
            "human_end_ms",
            "end_ms",
        ),
        "blink_window": ("pst_end_ms", "human_end_ms"),
    },
    "error": {
        "keys": (
            "uhoh_end_ms",
            "human_end_ms",
            "problem_end_ms",
            "end_ms",
        ),
        "increasing": (
            "uhoh_end_ms",
            "human_end_ms",
            "problem_end_ms",
            "end_ms",
        ),
    },
    "abort": {
        "keys": (
            "fine_end_ms",
            "didnt_want_end_ms",
            "finish_end_ms",
            "end_ms",
        ),
        "increasing": (
            "fine_end_ms",
            "didnt_want_end_ms",
            "finish_end_ms",
            "end_ms",
        ),
    },
    "dead": {
        "keys": (
            "shutdown_ms",
            "dense_ms",
            "end_ms",
        ),
        "increasing": (
            "dense_ms",
            "shutdown_ms",
            "end_ms",
        ),
    },
}

HEADROOM_BYTES = 64 * 1024
END_TOLERANCE_MS = 200
MOD_NAME_RE = re.compile(r"[a-z0-9][a-z0-9_-]*")


class AudioPackError(Exception):
    pass


def validate_mod_name(name):
    if not isinstance(name, str) or MOD_NAME_RE.fullmatch(name) is None:
        raise AudioPackError(
            f"Audio mod name must be a single folder name, got {name!r}"
        )
    return name


def _require_source_format(wav, path):
    if wav.getnchannels() != 1 or wav.getframerate() != SOURCE_RATE or wav.getsampwidth() != 2:
        raise AudioPackError(
            f"{path} must be {SOURCE_RATE} Hz mono 16-bit PCM "
            f"(got {wav.getframerate()} Hz, "
            f"{wav.getnchannels()} ch, "
            f"{wav.getsampwidth() * 8}-bit)"
        )


def wav_duration_ms(path):
    with wave.open(str(path), "rb") as wav:
        _require_source_format(wav, path)
        frames = wav.getnframes()
    return int(round(frames * 1000 / SOURCE_RATE))


def _halfband_taps(count=31):
    # Blackman-windowed sinc with its cutoff at the output Nyquist. Every
    # even offset from the centre is zero, so only the odd pairs are kept.
    middle = count // 2
    window = [
        0.42 - 0.5 * math.cos(2 * math.pi * n / (count - 1))
        + 0.08 * math.cos(4 * math.pi * n / (count - 1))
        for n in range(count)
    ]
    pairs = [
        (offset, math.sin(math.pi * offset / 2) / (math.pi * offset) * window[middle + offset])
        for offset in range(1, middle + 1, 2)
    ]
    gain = 0.5 + 2 * sum(tap for _, tap in pairs)
    return 0.5 / gain, [(offset, tap / gain) for offset, tap in pairs]


def halve_sample_rate(path):
    """Low-pass and keep every second sample, rewriting the WAV in place."""
    with wave.open(str(path), "rb") as wav:
        _require_source_format(wav, path)
        samples = array("h", wav.readframes(wav.getnframes()))

    centre, pairs = _halfband_taps()
    reach = pairs[-1][0]
    count = len(samples) // 2
    padded = [0] * reach + samples.tolist() + [0] * reach
    acc = [centre * value for value in padded[reach:reach + 2 * count:2]]
    for offset, tap in pairs:
        before = padded[reach - offset::2]
        after = padded[reach + offset::2]
        acc = [total + tap * (left + right) for total, left, right in zip(acc, before, after)]

    out = array("h", (max(-32768, min(32767, round(value))) for value in acc))
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(PACKED_RATE)
        wav.writeframes(out.tobytes())


def parse_cue_text(text, label):
    values = {}
    for lineno, raw in enumerate(text.splitlines(), 1):
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        if "=" not in line:
            raise AudioPackError(f"{label}:{lineno}: expected key=ms")
        key, _, rest = line.partition("=")
        key = key.strip()
        rest = rest.strip()
        if not key or not rest.isdigit():
            raise AudioPackError(f"{label}:{lineno}: expected key=ms")
        if key in values:
            raise AudioPackError(f"{label}:{lineno}: duplicate {key}")
        values[key] = int(rest)
    return values


def validate_cue(clip, values, duration_ms, label):
    schema = CUE_CLIPS[clip]
    expected = set(schema["keys"])
    got = set(values)
    if got != expected:
        missing = ", ".join(sorted(expected - got)) or "(none)"
        extra = ", ".join(sorted(got - expected)) or "(none)"
        raise AudioPackError(f"{label}: missing [{missing}], extra [{extra}]")

    increasing = schema["increasing"]
    previous = 0
    for key in increasing:
        value = values[key]
        if value <= previous:
            raise AudioPackError(
                f"{label}: {key}={value} must be greater than {previous}"
            )
        previous = value

    window = schema.get("blink_window")
    if window is not None:
        start = values["blink_start_ms"]
        end = values["blink_end_ms"]
        lo = values[window[0]]
        hi = values[window[1]]
        if not (lo <= start < end <= hi):
            raise AudioPackError(
                f"{label}: blink {start}-{end} must sit inside {lo}-{hi}"
            )

    end_ms = values["end_ms"]
    if abs(end_ms - duration_ms) > END_TOLERANCE_MS:
        raise AudioPackError(
            f"{label}: end_ms={end_ms} is more than {END_TOLERANCE_MS} ms "
            f"from WAV duration {duration_ms} ms"
        )


def spiffs_size_bytes(partitions_csv):
    path = Path(partitions_csv)
    for raw in path.read_text().splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        parts = [part.strip() for part in line.split(",")]
        if len(parts) < 5:
            continue
        if parts[0] == "spiffs" or parts[2] == "spiffs":
            return int(parts[4], 0)
    raise AudioPackError(f"No spiffs partition in {path}")


def _managed_names():
    names = list(STOCK_WAVS)
    names.extend(f"{clip}.cue" for clip in CUE_CLIPS)
    return names


def pack_audio(assets_dir, dest_dir, partition_bytes, mod_dir=None):
    assets_dir = Path(assets_dir)
    dest_dir = Path(dest_dir)
    dest_dir.mkdir(parents=True, exist_ok=True)

    for name in _managed_names():
        leftover = dest_dir / name
        if leftover.is_file():
            leftover.unlink()

    for name in STOCK_WAVS:
        source = assets_dir / name
        if not source.is_file():
            raise AudioPackError(f"Missing audio asset: {source}")
        dest = dest_dir / name
        shutil.copy2(source, dest)
        print(f"Copied {source.name} -> {dest}")

    if mod_dir is not None:
        _overlay_mod(Path(mod_dir), dest_dir)

    for name in STOCK_WAVS:
        halve_sample_rate(dest_dir / name)
    print(f"Resampled WAVs to {PACKED_RATE} Hz")

    total = sum(
        path.stat().st_size
        for path in dest_dir.iterdir()
        if path.is_file()
    )
    limit = partition_bytes - HEADROOM_BYTES
    if total > limit:
        raise AudioPackError(
            f"Audio image is {total} bytes; "
            f"spiffs allows {limit} after {HEADROOM_BYTES} bytes headroom"
        )
    print(f"Audio image {total} bytes (limit {limit})")


def _overlay_mod(mod_dir, dest_dir):
    if not mod_dir.is_dir():
        raise AudioPackError(f"Audio mod assets not found: {mod_dir}")

    wavs = {}
    cues = {}
    for path in sorted(mod_dir.iterdir()):
        if not path.is_file():
            continue
        suffix = path.suffix.lower()
        if suffix == ".wav":
            if path.name not in STOCK_WAVS:
                raise AudioPackError(f"Unknown audio file: {path.name}")
            wavs[path.stem] = path
        elif suffix == ".cue":
            if path.stem not in CUE_CLIPS:
                raise AudioPackError(f"Unknown cue file: {path.name}")
            cues[path.stem] = path

    for clip, cue_path in cues.items():
        if clip not in wavs:
            raise AudioPackError(f"{cue_path.name} has no matching WAV in {mod_dir}")

    for clip, wav_path in wavs.items():
        duration_ms = wav_duration_ms(wav_path)
        if clip in CUE_CLIPS:
            cue_path = cues.get(clip)
            if cue_path is None:
                raise AudioPackError(f"{wav_path.name} needs {clip}.cue")
            values = parse_cue_text(cue_path.read_text(), cue_path.name)
            validate_cue(clip, values, duration_ms, cue_path.name)
            dest_cue = dest_dir / cue_path.name
            shutil.copy2(cue_path, dest_cue)
            print(f"Copied {cue_path.name} -> {dest_cue}")
        dest_wav = dest_dir / wav_path.name
        shutil.copy2(wav_path, dest_wav)
        print(f"Overlaid {wav_path.name} -> {dest_wav}")
