# Wiring craft and schematics

Software has stack traces. Hardware has smoke. Most beginner failures aren't mysterious — they're a loose GND, swapped data lines, or a wire that pulled out when you closed the case. This chapter is about making connections you can trust and reading the diagram — Tiny Engineer's drawing is the practice artifact.

---

## Dupont / jumper — the connector this project uses

**Dupont** (jumper) = plastic housing on a pin header. One wire, one pin. That's the whole connector story here.

This build uses Dupont on **servos**, **OLED**, and **ESP32** so those parts unplug and swap if something dies. Servo cables already end in a 3-pin plug (signal / +5 V / GND — often yellow, red, brown; **verify your batch**). That plug seats on Dupont-style headers.

> **If you've written backend code…** Dupont is like a typed port: one pin, one job. Read the silkscreen. Wrong hole is a type error with smoke.

---

## Breadboard vs permanent wiring

**Breadboard** — spring clips, no solder. Great for blinking an LED. Wires pop out. Too bulky for this robot.

**This project needs permanent wiring** — soldered joints that survive being stuffed into a small printed body. Vibration and tight space kill breadboard builds.

**Practical split:**
- **Solder** the boards that stay in the robot: USB-C breakout, PCA9685 servo driver, audio amp, and the power/signal harness between them
- **Dupont** to ESP32, OLED, and servos — replaceable without a soldering iron
- Wire and **verify on the bench** (boot, I2C, one servo, audio) **before** routing into the shell. Debugging inside a closed chest is miserable

The Waveshare **ESP32-C3-Zero** used here ships with **male pins already soldered**. You still solder everything else that doesn't come with headers you can plug into: USB socket/breakout, servo controller, audio board, and the wires that join those modules.

---

## Strain relief and color

Don't tug on solder joints. Zip-tie or clip USB and servo cables so a yank hits the tie, not the pad.

Color is habit, not law: red ≈ positive, black/brown ≈ GND, yellow/orange/green ≈ signal. **Trust labels and a multimeter, not memory.**

---

## Reading schematics and block diagrams

You don't need a full EE schematic. You need the **block diagram** mindset:

- **Boxes** = modules (ESP32, PCA9685, OLED, amp)
- **Lines** = nets (SDA, SCL, 5V, GND)
- **Same net name** = must be connected together

Tiny Engineer's map: [Tiny Engineer.drawio.png](../wiring/Tiny%20Engineer.drawio.png).

> **If you've written backend code…** The PNG is an architecture diagram. [wiring.md](../hardware/wiring.md) is the connection table. [pinout.md](../hardware/pinout.md) is GPIO allocation. Same system, three views.

**How to use the diagram:**
1. Find the ESP32 box — trace I2C to PCA9685 and OLED
2. Trace 5 V from USB breakout to each power consumer
3. Trace 3.3 V from ESP32 to logic VCC pins
4. Confirm GND stars at the USB breakout and fans out

Don't memorize — **trace the path** for whatever you're debugging.

---

## Soldering: enough to not fear it

You will solder. The ESP32 headers are already done; USB, PCA9685, audio, and the interconnects are not.

Basics:
- **Iron** ~350°C for lead-free (follow your solder's label)
- **Tin the pad and the wire**, then join — don't dump a blob on one side
- **Continuity check** after — multimeter beep between pad and wire end

Bad solder looks dull and grainy (cold joint). Good joint is shiny and concave. YouTube "through-hole soldering" beats a paragraph here.

---

## What not to do on first power-up

- Don't power servos before common GND is verified
- Don't connect MAX98357A **SPK−** to GND (bridge-tied load — see [Ch. 04](04-buses-and-protocols.md))
- Don't force a tight 3D-printed pocket onto a servo — ream or reprint instead

Assembly checks before first power: [wiring.md](../hardware/wiring.md) — read that when you're actually wiring, not this chapter.

---

## In Tiny Engineer

Use [Tiny Engineer.drawio.png](../wiring/Tiny%20Engineer.drawio.png) as your map and [wiring.md](../hardware/wiring.md) as your checklist. This guide doesn't duplicate the connection table.

**Bench bring-up order** (wiring mindset):
1. USB power + GND only → ESP32 boots (LED, serial)
2. Add I2C devices → serial shows PCA9685 found
3. Add one servo on one channel → `/test/servo` moves it
4. Add audio, OLED, remaining servos
5. Install into printed parts

**OLED naming trap:** the pad labeled **SCK** on many OLED modules is I2C **SCL**, not SPI clock. Easy to misread if you're used to SPI docs.

---

**Next:** [Buses and protocols](04-buses-and-protocols.md)

**Reference:** [hardware/wiring.md](../hardware/wiring.md) · [wiring/Tiny Engineer.drawio.png](../wiring/Tiny%20Engineer.drawio.png)
