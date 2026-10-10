# Flash firmware

Flash **after connect, before assembly**. Overview order: [getting-started.md](getting-started.md). Centering (Move all to 90°) happens on the desk with the main control board; do not finish the Wi-Fi wizard until [assembly.md §20](3d/assembly.md#20-setup-wizard-and-first-boot-on-wi-fi).

Physical module is a **Waveshare ESP32-C3-Zero**.

## Web flash (default)

Stock firmware from the browser — no toolchain install, no repo clone.

1. Open **[jamro.github.io/tiny-engineer/flash/](https://jamro.github.io/tiny-engineer/flash/)** in **Chrome or Edge**.
2. Use a USB-C **data** cable (charge-only cables fail).
3. Pick a release (latest is fine) → **Connect & install**.

The stock image includes app firmware and LittleFS (WAV audio). The site ships each release’s images; older tags keep their own partition layout.

### Done when (web path)

- Onboard RGB is **dim green** during init (not a repeating red blink).
- One servo moves: join setup AP `TinyEngineer-XXXX`, open `http://192.168.4.1/config`, press **Move all to 90°**.

Fatal init hangs and blinks the RGB **red**. Count flashes before the long gap:

| Blinks | Meaning |
| --- | --- |
| 1 | PCA9685 not found on I2C `0x40` — wiring / address |
| 2 | MAX98357A / I2S init failed |

Details: [hardware/testing.md — boot-failure blink codes](hardware/testing.md#boot-failure-blink-codes).

**Stop here.** Do not complete the setup wizard (home Wi-Fi, ranges, OLED, LED, speaker) until the robot is assembled — [assembly.md §20](3d/assembly.md#20-setup-wizard-and-first-boot-on-wi-fi). Then return to [getting-started](getting-started.md) and assemble.

---

## Advanced: PlatformIO

Use this path only if you **build from source**, flash an [audio mod](../mods/README.md), use **OTA**, or need a **serial** monitor. Not required for a stock first flash.

Firmware is Arduino on [PlatformIO](https://platformio.org/) ([pioarduino](https://github.com/pioarduino/platform-espressif32) / Arduino-ESP32 3.x). Board and baud live in `platformio.ini`. PlatformIO `board = esp32-c3-devkitm-1` is a **build target name**, not a different board.

### Install and connect

1. Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html) (or the PlatformIO IDE extension).
   Also install [Node.js](https://nodejs.org/) 20.19+ on 20.x, or 22.12+: `pio run` builds the web UI in [`ui/`](../ui/) with npm and embeds it in the firmware.
2. Use a USB-C **data** cable. Charge-only cables fail upload and serial.
3. Plug the data cable into the **main control board USB-C** once the ESP32 is seated on its headers. Advanced breakout build: use the Adafruit 5993, or the C3-Zero onboard USB-C before that breakout is wired — [hardware/wiring.md](hardware/wiring.md).

### Build, upload, serial

From the project root:

```bash
pio run                 # build
pio run -t upload       # flash firmware + LittleFS
pio device monitor      # serial (115200)
```

After firmware upload, a post-script also uploads **LittleFS** ([`scripts/upload_fs_after_upload.py`](../scripts/upload_fs_after_upload.py)) so WAV assets (`welcome`, `bell`, and friends) land on the board. If animations move but stay silent, run `pio run -t uploadfs` once.

Several serial ports:

```bash
pio device list
pio run -t upload --upload-port <PORT>
pio device monitor --port <PORT>
```

`<PORT>` is the name `pio device list` prints for the board, and it is
platform-specific: `COM4` on Windows, `/dev/cu.usbmodemXXXX` or
`/dev/cu.usbserial-XXXX` on macOS, `/dev/ttyACM0` or `/dev/ttyUSB0` on Linux.
Pick the entry whose hardware ID shows Espressif's `VID:PID=303A:1001` (the
ESP32-C3's native USB) or your board's USB-serial bridge — `pio device list`
also lists Bluetooth serial ports, which are not the board.

### Download release binaries

Tagged releases publish stock bins on [GitHub Releases](https://github.com/jamro/tiny-engineer/releases). Offsets are defined per release in that tag’s `partitions.csv` and copied into `manifest.json` by [`.github/workflows/release.yml`](../.github/workflows/release.yml). On current `main`:

| Asset | Flash offset |
| --- | --- |
| `tiny-engineer-<tag>-bootloader.bin` | `0x0` |
| `tiny-engineer-<tag>-partitions.bin` | `0x8000` |
| `tiny-engineer-<tag>-firmware.bin` | `0x10000` (`app0`) |
| `tiny-engineer-<tag>-littlefs.bin` | `0x220000` (`spiffs`) |

All **four** files are required for a clean first flash or web install. App + LittleFS alone is enough only when bootloader and partition table are already on the chip. Each `v*` Release also attaches `manifest.json` for the web flasher. CI attaches the same four bin paths as the `firmware-<sha>` Actions artifact on PR and `main` builds (90-day retention). Pages deploy mirrors those assets for the browser installer (see [Web flash](#web-flash-default)).

### Audio mods

Leave `custom_audio_mod` empty in [`platformio.ini`](../platformio.ini) for the stock clips in [`assets/`](../assets/). Set it to a mod folder name to overlay that mod's WAVs, then flash firmware and the filesystem together:

```ini
custom_audio_mod = halloween
```

```bash
pio run -t upload
```

The pack step copies `assets/*.wav`, then overwrites any matching file in `mods/<name>/assets/`. A clip the mod does not ship stays the stock file. `welcome`, `attention`, `error`, `abort`, and `dead` replacements need a sibling `.cue` (`key=ms` phrase marks). `bell` does not. Details: [`mods/README.md`](../mods/README.md).

The `spiffs` partition starts at `0x220000` and is about 1.81 MB, so a longer replacement set can fit. NVS stays at `0x9000`, so saved settings stay. An image flashed at the old `0x270000` offset will not mount. Use `pio run -t upload` (app + filesystem), not a firmware-only flash, after pulling this layout.

### Over-the-air updates (optional)

The `ota` build environment adds Wi-Fi updates. Compared with the default build it trades:

- the coredump partition, for a second firmware slot ([`partitions_ota.csv`](../partitions_ota.csv))
- LittleFS shrinks from about 1.81 MB to 960 KB, so there is less room for longer mod clips

Switching layouts needs one USB flash of firmware and filesystem together:

```bash
pio run -e ota -t upload
```

After that, with the robot on your home Wi-Fi:

```bash
export TINY_ENGINEER_URL=http://192.168.x.x   # default: tiny-engineer.local
export TINY_ENGINEER_TOKEN=...                # only if access_token is set
pio run -e ota -t ota       # firmware
pio run -e ota -t otafs     # LittleFS (WAV assets)
```

These are the same variables the agent integrations read ([integration.md](integration.md)). OTA listens on UDP/TCP port 3232 and only runs while connected to home Wi-Fi, not in setup AP mode. When `access_token` is set it is also the OTA password; changing it applies to OTA without a reboot.

An update is written to the inactive slot and booted once; it becomes permanent only after it connects to Wi-Fi and starts its OTA listener. Firmware that never gets that far is rolled back to the previous slot on the next reset or power cycle. A firmware that hangs needs that power cycle to recover.

A filesystem update that fails midway reboots the robot; re-run `pio run -e ota -t otafs` or fall back to `pio run -e ota -t uploadfs` over USB.

Partition changes, including a switch back to the default build, cannot be applied over the air. Flash those with `pio run -t upload` over USB.

### Backfill a historical release (e.g. `v0.1.0`)

Tagged releases created before the release workflow can be made flashable later.

1. Build the four bins from **that tag’s commit** (`git switch --detach <tag>`, then `pio run` and `pio run -t buildfs`).
2. Write a `manifest.json` (ESP Web Tools schema, `chipFamily: "ESP32-C3"`) whose `parts` point at  
   `https://github.com/jamro/tiny-engineer/releases/download/<tag>/tiny-engineer-<tag>-{bootloader,partitions,firmware,littlefs}.bin`  
   with offsets from **that tag’s** [`partitions.csv`](../partitions.csv) (plus bootloader `0x0`, partition table `0x8000`).  
   Example for `v0.1.0` (dual-OTA layout on that tag): LittleFS / `spiffs` at **`0x2B0000`** (`2818048`), not the current `main` offset.
3. Upload the four bins and `manifest.json` (exact asset name) to that Release (`gh release upload` or the GitHub UI).
4. Redeploy Pages (**Actions → Pages → Run workflow**, or push a `web/**` change, or edit/publish a Release). The catalog script mirrors the new assets; then reload `/flash/`.

Do **not** reuse current-`main` offsets for an old tag.

### Done when (PlatformIO path)

Same checks as [Done when (web path)](#done-when-web-path). With serial:

- Monitor shows a normal boot (`TINY ENGINEER` … PCA9685 found … no hang)
- Onboard RGB is **dim green** during init
- One servo moves via **Move all to 90°** or `POST /test/servo`
