# Getting started

Canonical build checklist for Tiny Engineer. Each step: open the linked doc, finish the exit criterion, return here. Mechanical joins live in [assembly.md](3d/assembly.md); this page owns overall order.

Robot already on Wi-Fi? Jump to [step 8](#8-agent-hooks).

## Safety

Read this before you power anything. Optional *why*: [Ch. 01](hardware-for-software-engineers/01-electricity-and-units.md) and [Ch. 07](hardware-for-software-engineers/07-power-budgets-and-safety.md). Full primer is parallel reading, not a gate — [From Code to Circuits](hardware-for-software-engineers/README.md).

- Logic is **3.3 V**. Servos and the amp need **5 V / ≥2 A**. Never power servos from the ESP32 3.3 V pin.
- PCA9685 **VCC** (logic, 3.3 V) must not short to **V+** (servo rail, 5 V).
- Every module shares **GND**.
- Speaker connects to **SPK+** and **SPK−** only. **SPK− is not ground.**
- Leave the C3-Zero ceramic antenna clear of metal and dense plastic.

## Timeline

Shop (pick electronics path) → print → wire → flash → assemble (one run) → setup wizard → prove → agent hooks.

Same safety rules apply whether you build with **modular breakouts** or the **main control board**.

```mermaid
flowchart LR
  shop[Shop]
  print[Print]
  wire[Wire]
  flash[Flash]
  mech[Assemble]
  wiz[Wizard]
  prove[Prove]
  agent[Agent]

  shop --> print --> wire --> flash --> mech --> wiz --> prove --> agent
```

## Checklist

### 0. Safety

- **Open:** the [Safety](#safety) card on this page.
- **Until:** you know 3.3 V vs 5 V, VCC ≠ V+, common GND, SPK− ≠ GND, and 2 A.
- **Return** here.

### 1. Shop

- **Open:** [shopping.md](shopping.md). Pick **modular** or **main control board** — that page has pros/cons and carts.
- **Until:** cart in hand for your path — electronics, M2 screws/nuts, **5 V / ≥2 A** supply, **data** USB cable, and a print path (filament or a service order). Default servos: **Tower Pro SG90**.
- **Return** here.

### 2. Print or order

- **Open:** [3d_models/README.md](../3d_models/README.md) (home print; testers first) or [3d/order-parts.md](3d/order-parts.md) (no printer).
- **Until:** printed set for your servo model ready.
- **Return** here.

### 3. Wire

- **Modular:** [hardware/wiring.md](hardware/wiring.md) and [wiring/Tiny Engineer.drawio.png](wiring/Tiny%20Engineer.drawio.png).
- **Main control board:** [hardware/main-control-board.md](hardware/main-control-board.md).
- **Until:** pre-power checks pass (common GND; PCA9685 **VCC** = 3.3 V vs **V+** = 5 V not shorted; OLED clock on **SCL**; speaker on **SPK+/SPK−** only — PCB path: one speaker method only). Electronics stay on the desk.
- **Return** here.

### 4. Flash

- **Open:** **[Web flash](https://jamro.github.io/tiny-engineer/flash/)** (Chrome or Edge; USB-C **data** cable). Pick the latest release → Connect & install. No PlatformIO or repo clone needed for stock firmware. More detail / stuck / advanced: [flash.md](flash.md).
- **Until:** stock firmware + audio on the board; **Move all to 90°** (setup AP config page) or one servo moves. Do not finish the Wi-Fi wizard yet.
- **Return** here. Keep electronics on the desk — do not seat them in the chest yet.

### 5. Assemble

- **Open:** [3d/assembly.md](3d/assembly.md) from §1 through §19 (Head/Hat, centering, joins, channels, arms, lamp).
- **Until:** all joins done; PCA9685 channels plugged per the guide.
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

- **Open:** [hooks.md](hooks.md) (Cursor) or [integration.md](integration.md) (Antigravity, Claude Code, raw REST).
- **Until:** optional — an agent event triggers an animation.
- Done.

## Stuck?

| Symptom | What to try |
| --- | --- |
| Which electronics path? | [shopping.md](shopping.md#choose-electronics-path) |
| What to buy? | [shopping.md](shopping.md) |
| Which wires / voltages? | Modular: [hardware/wiring.md](hardware/wiring.md). PCB: [hardware/main-control-board.md](hardware/main-control-board.md). Pins: [hardware/pinout.md](hardware/pinout.md) |
| What to print? | [3d_models/README.md](../3d_models/README.md) |
| How to assemble printed parts? | [3d/assembly.md](3d/assembly.md) |
| Flash / install stuck | [Web flash](https://jamro.github.io/tiny-engineer/flash/) · [flash.md](flash.md) |
| `.local` slow or fails | OLED IP; `curl -4 http://…` |
| OLED shows join AP / `192.168.4.1` | Wi-Fi not saved or STA failed — finish [assembly §20](3d/assembly.md#20-setup-wizard-and-first-boot-on-wi-fi) |
| Welcome / ring silent (servos move) | Re-install the same release from [Web flash](https://jamro.github.io/tiny-engineer/flash/) (stock image includes audio). Advanced: [flash.md](flash.md) |
| Hooks never move the robot | Node 18+, hook `timeout` ≥ 30, HTTPS tarball `npx` — see [hooks.md](hooks.md) |
| Servos twitch / board resets on motion | Power budget — [hardware/power.md](hardware/power.md) |
| Other boot / I2C / audio failures | [hardware/testing.md](hardware/testing.md) |
