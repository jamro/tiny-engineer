# main-control-board-servo-tester

Plug that seats on one servo column of the main control board. A red LED shows 5 V. A yellow LED shows the signal.

- **Status:** draft
- **KiCad:** 10
- **Interfaces:** J1, 1x03 socket. Pin 1 signal (schematic net `+3.3V`) through R1 470 Ω to yellow D1. Pin 2 `+5V` through R2 1 kΩ to red D2. Pin 3 GND. Both LED cathodes go to GND.
- **Assumptions:** Plug matches the main control board column: silk-side pin is signal, middle pin is 5 V, square pad is GND. Only the Default net class. Power comes from the column under test, not from its own USB.
- **Built:** not manufactured
- **Tested:** not tested

![Servo tester](./servo-tester-preview.png)
