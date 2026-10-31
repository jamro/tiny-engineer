# main-control-board-servo-tester

Plug that seats on one servo column of the main control board. A red LED shows 5 V. A yellow LED shows the signal.

- **Status:** draft
- **KiCad:** 10
- **Interfaces:** J1, 1x03 socket. Pin 1 signal (schematic net `+3.3V`) through R1 470 Ω to yellow D1. Pin 2 `+5V` through R2 1 kΩ to red D2. Pin 3 GND. Both LED cathodes go to GND.
- **Assumptions:** Seat J1 so the silk labels match the main-board column: PWM to PWM, +5V to 5 V, GND to GND. Read those labels on both boards before plugging in. The capacitor beside the servo header blocks the other orientation. Pin 1 on this tester is the signal pin; it meets the PWM pin of the column, not pin 1. Only the Default net class. Power comes from the column under test.
- **Built:** not manufactured
- **Tested:** not tested

![Servo tester](./servo-tester-preview.png)
