# Getting started

Canonical build checklist for Tiny Engineer. Each step: open the linked doc, finish the exit criterion, return here. Mechanical joins live in [assembly.md](3d/assembly.md); this page owns overall order.

Robot already on Wi-Fi? Jump to [step 8](#8-agent-hooks).

## Safety

Read this before you power anything. Optional primer, not a gate: [From Code to Circuits](hardware-for-software-engineers/README.md).

- Supply **5 V / ≥2 A** on the board USB-C.
- Speaker: connector **or** solder pads, never both. **SPK− is not ground.**
- Leave the C3-Zero ceramic antenna clear of metal and dense plastic.

## Timeline

Shop → print → connect → flash → assemble (one run) → setup wizard → prove → agent hooks.

The default build uses the **main control board**. Plug in the ESP32, OLED, servos, and speaker. To add modules the board does not carry, follow the harness rules in [wiring.md](hardware/wiring.md) and mount them with [assembly-modular.md](3d/assembly-modular.md).

```mermaid
flowchart LR
  shop[Shop]
  print[Print]
  connect[Connect]
  flash[Flash]
  mech[Assemble]
  wiz[Wizard]
  prove[Prove]
  agent[Agent]

  shop --> print --> connect --> flash --> mech --> wiz --> prove --> agent
```

## Checklist

### 1. Shop

- **Open:** [shopping.md](shopping.md). Default cart is the main control board plus the ESP32, OLED, servos, and speaker.
- **Until:** cart in hand — main control board, ESP32, servos, OLED, speaker, M2 screws/nuts, **5 V / ≥2 A** supply, **data** USB cable, and a print path (filament or a service order). Default servos: **Tower Pro SG90**.
- **Return** here.

### 2. Print or order

- **Open:** [3d_models/README.md](../3d_models/README.md) (home print; testers first) or [3d/order-parts.md](3d/order-parts.md) (no printer).
- **Until:** printed set for your servo model ready.
- **Return** here.

### 3. Connect

- **Open:** [hardware/main-control-board.md](hardware/main-control-board.md).
- **Until:** pre-power checks pass (speaker on **SPK+/SPK−** only, one method; servo plugs match the labels; ESP32 orientation matches the PCB USB mark). Electronics stay on the desk.
- **Advanced:** extra modules the board does not support — wire the harness now ([hardware/wiring.md](hardware/wiring.md)). Mount those boards during assembly ([3d/assembly-modular.md](3d/assembly-modular.md)).
- **Return** here.

### 4. Flash

- **Open:** **[Web flash](https://jamro.github.io/tiny-engineer/flash/)** (Chrome or Edge; USB-C **data** cable). Pick the latest release → Connect & install. No PlatformIO or repo clone needed for stock firmware. More detail / stuck / advanced: [flash.md](flash.md).
- **Until:** stock firmware + audio on the board; **Move all to 90°** (setup AP config page) or one servo moves. Do not finish the Wi-Fi wizard yet.
- **Return** here. Keep electronics on the desk — do not seat them in the chest yet.

### 5. Assemble

- **Open:** [3d/assembly.md](3d/assembly.md) from §1 through §19 (Head/Hat, centering, joins, channels, arms, lamp).
- **Until:** all joins done; servo and OLED headers plugged per the guide.
- **Return** here.

### 6. Setup wizard

- **Open:** [3d/assembly.md §20](3d/assembly.md#20-setup-wizard-and-first-boot-on-wi-fi).
- **Until:** wizard finished, power-cycled, robot on home **2.4 GHz** Wi-Fi.
- **Return** here.

### 7. Prove it

Open `http://tiny-engineer.local/` (or the IP on the OLED) for the web UI.

```bash
curl http://tiny-engineer.local/health
curl -X POST "http://tiny-engineer.local/anim?name=ring"
```

- **Until:** `/health` OK and `ring` runs (motion; sound if LittleFS uploaded). If `.local` is slow, use the OLED IP or `curl -4`.
- **Return** here. Full API: [api.md](api.md).

### 8. Agent hooks

- **Open:** [integration.md](integration.md) — `tiny-engineer setup` (default) or REST for custom agents. Cursor details: [hooks.md](hooks.md).
- **Until:** optional — an agent event triggers an animation.
- Done.

## Stuck?

| Symptom | What to try |
| --- | --- |
| Extra modules the board does not support? | [shopping.md](shopping.md#advanced-extra-modules) · [hardware/wiring.md](hardware/wiring.md) |
| What to buy? | [shopping.md](shopping.md) |
| How to plug in the board? | [hardware/main-control-board.md](hardware/main-control-board.md) |
| What to print? | [3d_models/README.md](../3d_models/README.md) |
| How to assemble printed parts? | [3d/assembly.md](3d/assembly.md) |
| Flash / install stuck | [Web flash](https://jamro.github.io/tiny-engineer/flash/) · [flash.md](flash.md) |
| `.local` slow or fails | OLED IP; `curl -4 http://…` |
| OLED shows join AP / `192.168.4.1` | Wi-Fi not saved or STA failed — finish [assembly §20](3d/assembly.md#20-setup-wizard-and-first-boot-on-wi-fi) |
| Welcome / ring silent (servos move) | Re-install the same release from [Web flash](https://jamro.github.io/tiny-engineer/flash/) (stock image includes audio). Advanced: [flash.md](flash.md) |
| Hooks never move the robot | Node 18+, `npx -y tiny-engineer doctor`, hook `timeout` ≥ 30 for cold `npx` — see [integration.md](integration.md) / [hooks.md](hooks.md) |
| Servos twitch / board resets on motion | Power budget — [hardware/power.md](hardware/power.md) |
| Other boot / I2C / audio failures | [hardware/testing.md](hardware/testing.md) |
