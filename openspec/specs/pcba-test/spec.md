# pcba-test Specification

## Purpose

Defines the PCBA test session: a golden-module image and a host, either the Python script or the web page, that walk one person through the ESP32 socket, the speaker, and the servo columns together, then record a pass or fail.

## Requirements

### Requirement: PCBA test image is not the product boot
The PCBA test image MUST start the PCBA session on USB connect. It MUST NOT join Wi-Fi, start HTTP, mount the audio filesystem, or speak the product COBS serial protocol.

#### Scenario: Power-up banner
- **WHEN** the golden module receives USB power through a board under test
- **THEN** it prints PCBA-TEST v1 on USB CDC at 115200
- **AND** it does not open the product setup AP or HTTP server

### Requirement: PCA probe gates the session
Before the power chime, the tone step, or any servo step, the image MUST read PCA9685 register 0x00 (MODE1) at I2C address 0x40 twice. The probe succeeds only when both reads complete, return the same byte, and that byte is not 0xFF. An address ACK alone MUST NOT count as success. If the probe fails, the image MUST print a PCA failure and a final FAIL result and MUST NOT play the power chime, the tone, or servo steps. When the probe succeeds, the image MUST play a short power chime, then wait. That chime means the board has power and is ready for the test. It is not the tone step.

#### Scenario: PCA missing
- **WHEN** the session starts and a MODE1 read fails or returns 0xFF
- **THEN** the image prints FAIL pca and RESULT FAIL pca
- **AND** it does not play the power chime or a tone, and it does not offer retry, continue, or restart
- **AND** it does not print READY

#### Scenario: PCA present
- **WHEN** both MODE1 reads return the same byte other than 0xFF
- **THEN** the image plays the power chime and prints READY
- **AND** the tone step has not started until the operator sends Enter

### Requirement: Fixed step order
A board session MUST run three steps in this order: tone, then servos, then oe. The oe step MUST disable the PCA9685 outputs and MUST keep pulsing PWM on channels 0, 1, 2, 3, and 4. The servos step MUST pulse every servo column at the same time.

#### Scenario: Happy path order
- **WHEN** the operator accepts every step
- **THEN** the image announces tone, then servos, then oe
- **AND** then prints a RESULT line

### Requirement: Operator accepts or rejects the current step
Each step MUST hold its outputs until the operator sends Enter or `n`. Enter on the wire MUST be a single line feed and MUST record a pass for that step. `n` MUST record a fail for that step and present the failure menu. Any other key while the step is waiting for Enter or `n` MUST be ignored, except `+` and `-` on the tone step.

#### Scenario: Accept tone
- **WHEN** the tone step is active and the operator sends Enter
- **THEN** the tone step is recorded as passed
- **AND** the session moves to servos

#### Scenario: Reject a column
- **WHEN** a servo step is active and the operator sends `n`
- **THEN** that step is recorded as failed
- **AND** the image presents the failure menu before leaving the step

### Requirement: Failure menu offers retry, continue, or restart
On every failed step the image MUST wait for `r`, `c`, or `a`. Any other key during that wait MUST be ignored. `r` MUST repeat the same step. `c` MUST keep the failure and advance. `a` MUST discard the board's results and return to the connected confirmation without removing USB power. The tone MUST NOT start until Enter.

#### Scenario: Retry the same step
- **WHEN** the operator sends `r` after failing servos
- **THEN** the image announces servos again
- **AND** the previous fail for servos is not kept

#### Scenario: Continue after a failure
- **WHEN** the operator sends `c` after failing a step
- **THEN** the session advances to the next step
- **AND** the failed step remains on the board's fail list

#### Scenario: Restart the board
- **WHEN** the operator sends `a` during a board session
- **THEN** the fail list is cleared
- **AND** the image prints READY
- **AND** the tone has not started until the operator sends Enter

### Requirement: All servo columns pulse together
During the servos step the image MUST pulse PWM duty on channels 0, 1, 2, 3, and 4 at the same time, with outputs enabled. The mapping MUST be head=0, neck=1, arm-l=2, arm-r=3, body=4. Moving the plug from column to column MUST NOT require a key.

#### Scenario: Every column pulses
- **WHEN** the servos step is active
- **THEN** channels 0 through 4 are pulsing
- **AND** the outputs are enabled

### Requirement: Tone is generated on the board
The tone step MUST play an audible tone through the I2S pins to the amplifier and MUST keep playing it until the operator sends Enter or `n`. The tone MUST NOT depend on files in the audio filesystem.

#### Scenario: Tone uses I2S pins
- **WHEN** the tone step becomes active
- **THEN** the image emits a tone on the I2S clock, word-select, and data pins
- **AND** the tone is still emitting while the operator has not sent Enter or `n`

### Requirement: Speaker step changes loudness
During the tone step `+` MUST make the tone louder and `-` MUST make it quieter without stopping the tone. At either end of the range that key MUST leave the level unchanged. Other steps MUST ignore `+` and `-`. The speaker card MUST show `+ = louder` and `- = quieter`.

#### Scenario: Louder on the speaker step
- **WHEN** the tone step is active and the operator sends `+`
- **THEN** the tone keeps playing at a higher level
- **AND** the host shows + = louder

#### Scenario: Quieter on the speaker step
- **WHEN** the tone step is active and the operator sends `-`
- **THEN** the tone keeps playing at a lower level

#### Scenario: Other steps ignore volume keys
- **WHEN** the servos step is active and the operator sends `+`
- **THEN** the servo pulse continues unchanged

### Requirement: One verdict line per board
When the step list finishes, the image MUST print a single RESULT line. RESULT PASS MUST be used only when every completed step was accepted. Otherwise the line MUST be RESULT FAIL followed by the failed step names separated by spaces, in step order. A `?` byte marks that the USB link is up. It is not Enter. When the PCA probe has already failed, `?` MUST repeat `FAIL pca` and `RESULT FAIL pca`. That repeat is the same board, not a new verdict. In every other phase, and after any other verdict, `?` MUST be ignored.

#### Scenario: All steps accepted
- **WHEN** the operator has accepted tone, servos, and oe
- **THEN** the image prints RESULT PASS

#### Scenario: Continue past two failures
- **WHEN** the operator continued past failed tone and failed servos
- **THEN** the image prints RESULT FAIL tone servos

#### Scenario: Link mark repeats a PCA failure
- **WHEN** the image has already printed RESULT FAIL pca and the host sends `?`
- **THEN** the image prints FAIL pca and RESULT FAIL pca again
- **AND** it does not start the tone or servo steps

### Requirement: Next board or stop after the verdict
After RESULT, removing USB power MUST end the session, and the next USB connect MUST start a new session at the banner. `q` MUST print STOP and MUST NOT start another step until power is removed and restored. `n` after RESULT MUST be ignored.

#### Scenario: Cable leaves after the verdict
- **WHEN** the module loses USB power after RESULT
- **THEN** a later USB connect starts a new session at the banner

#### Scenario: Operator stops
- **WHEN** the operator sends `q` after RESULT
- **THEN** the image prints STOP
- **AND** it does not announce another step until power is removed and restored

#### Scenario: Reject key after the verdict
- **WHEN** the operator sends `n` after RESULT
- **THEN** the image does not start another step
- **AND** it does not treat that key as a board failure

### Requirement: Host shows the step and survives the cable move
Each host MUST open the CDC port, show the current step or the RESULT line, and send Enter, `n`, `r`, `c`, `a`, and `q`. A step FAIL MUST show `r`, `c`, and `a`. FAIL pca MUST NOT. When the port returns, the host MUST attach without a relaunch.

#### Scenario: Key forwards to the module
- **WHEN** the operator presses Enter while a step is on screen
- **THEN** the host sends a line feed to the module

#### Scenario: Failure menu is on screen
- **WHEN** the module prints FAIL servos
- **THEN** the host shows `r`, `c`, and `a`

#### Scenario: PCA failure hides the step menu
- **WHEN** the module prints FAIL pca
- **THEN** the host does not show `r`, `c`, or `a`

#### Scenario: Cable moves to the next board
- **WHEN** the serial port drops and a CDC port returns
- **THEN** the host shows the new session's banner or step
- **AND** the operator does not start the program again

### Requirement: Web tester lives at /pcba_test and checks the browser
https://tinyengineer.dev MUST serve the web host at `/pcba_test`. No other page on that site MUST link to it. When Web Serial is unavailable or the page is not a secure context, the page MUST say this browser cannot run the test and MUST name the Python host command, and it MUST NOT offer a port connection. When Web Serial is available in a secure context, the page MUST offer a connection and then run the same session as the script host.

#### Scenario: Incompatible browser
- **WHEN** the page loads and Web Serial is missing or the context is not secure
- **THEN** the page states that this browser cannot run the test
- **AND** it names the Python host command
- **AND** it does not offer a port connection

#### Scenario: Compatible browser
- **WHEN** the page loads in a secure context with Web Serial
- **THEN** the page offers a port connection
- **AND** after the port is open it shows steps and forwards the operator keys

#### Scenario: No link from another site page
- **WHEN** a reader opens any other HTML page on the project site
- **THEN** that page does not link to `/pcba_test`

### Requirement: Procedure card and script name the web tester
The procedure card MUST describe the bench, the step order, the expected lamps, the keys, and both ways to run the session. The card MUST say the golden module stays with the tester. The card and the script host MUST name the web tester as https://tinyengineer.dev/pcba_test. The script MUST show that address as the alternate to itself.

#### Scenario: Card names the public page
- **WHEN** the operator reads the procedure card
- **THEN** it includes https://tinyengineer.dev/pcba_test
- **AND** it includes the Python host command
- **AND** it lists the step order and the keys

#### Scenario: Script offers the web tester
- **WHEN** the script starts
- **THEN** it shows https://tinyengineer.dev/pcba_test as the alternate tester

#### Scenario: Card keeps the golden module
- **WHEN** the operator reads the bench setup
- **THEN** the card says the golden module stays with the tester

### Requirement: Plug position and lamp pattern
The procedure card MUST say signal is on all five servo outputs and the operator plugs the testing board into one column at a time: HEAD, NECK, ARM L, ARM R, and BODY. The card MUST name the silk-side pin as yellow, the middle pin as red, and the square pad as GND. Each column MUST show red on and yellow pulsing. Tone MUST be judged by ear on J1.

#### Scenario: Servo step is one plug sweep
- **WHEN** the operator reads the servos step on the card
- **THEN** the card says signal is on all five servo outputs
- **AND** it says to plug the testing board into one column at a time
- **AND** the columns are HEAD, NECK, ARM L, ARM R, and BODY
- **AND** the expected look is red stays on and yellow pulses

#### Scenario: Plug faces the square pad
- **WHEN** the operator reads how to seat the plug
- **THEN** the card names the square pad as GND
- **AND** it names the silk-side pin as the yellow LED

### Requirement: Module LED shows the test image is running
While the test image has USB power it MUST hold the onboard LED steady white, including during a step, the failure menu, a PCA failure, RESULT, and STOP. The three channels MUST be equal. The LED MUST NOT blink. The procedure card MUST say this steady white means the test image is running, and that board pass or fail is the ear on J1 and the plug lamps.

#### Scenario: Session stays white
- **WHEN** the image is in a step, the failure menu, RESULT, or STOP
- **THEN** the onboard LED is steady white

#### Scenario: PCA failure stays white
- **WHEN** the MODE1 probe fails
- **THEN** the onboard LED is steady white
- **AND** the image still prints RESULT FAIL pca

#### Scenario: Card names the module lamp
- **WHEN** the operator reads the bench setup
- **THEN** the card says the module LED stays steady white while the test image has power

### Requirement: Host explains the current step in English
Each host MUST show English text for the current step: what is tested, what to do, and the expected effect. A waiting step MUST NOT show the STEP token. Steps MUST be numbered 1/3 through 3/3. A waiting step MUST show Enter as good and n as bad. A step FAIL MUST show that step's expected effect and words for r, c, and a. FAIL pca MUST NOT show r, c, or a. Both hosts MUST use the same step sentences.

#### Scenario: Tone step explains the ear
- **WHEN** the module prints STEP tone
- **THEN** the host shows 1/3
- **AND** it shows Speaker on the JST speaker port. Expect a repeating tone.
- **AND** it does not show the token STEP tone
- **AND** it shows Enter = good and n = bad

#### Scenario: Servo step is one sweep
- **WHEN** the module prints STEP servos
- **THEN** the host shows 2/3
- **AND** it shows Signal is on all five servo outputs. Take the testing board. Plug it into one column at a time. Columns are HEAD, NECK, ARM L, ARM R, and BODY. Check the LEDs on every column. Red stays on and yellow pulses.
- **AND** it does not show the token STEP servos

#### Scenario: OE step releases the outputs
- **WHEN** the module prints STEP oe
- **THEN** the host shows 3/3 OE OUTPUT ENABLE
- **AND** it shows The red LED stays on. Only the yellow LED goes out.
- **AND** it shows No need to test each servo column.
- **AND** it does not show the token STEP oe

#### Scenario: Step failure names the keys
- **WHEN** the module prints FAIL servos
- **THEN** the host shows Red stays on and yellow pulses.
- **AND** it shows r = try this step again
- **AND** it shows c = keep the fail, go on
- **AND** it shows a = start this board over

#### Scenario: PCA failure explains the stop
- **WHEN** the module prints FAIL pca
- **THEN** the host shows PCA9685 probe failed at I2C 0x40
- **AND** it shows That chip drives the servos, so this board stops before the speaker test
- **AND** it shows Seat the ESP32 and match the USB mark
- **AND** it shows VCC must be 3.3 V, not 5 V
- **AND** it shows Solder GND, SDA, and SCL
- **AND** it does not show r, c, or a

#### Scenario: PCA verdict keeps the explanation
- **WHEN** the module prints RESULT FAIL pca
- **THEN** the host shows PCA9685 probe failed at I2C 0x40
- **AND** it shows That chip drives the servos, so this board stops before the speaker test
- **AND** it shows unplug USB for the next board, or q to stop
- **AND** it does not show r, c, or a

### Requirement: Banner lamp sentence is page only
When the banner arrives, the page MUST show that the module LED stays steady white and is not the board verdict. The script MUST NOT show that sentence.

#### Scenario: Page names the module lamp
- **WHEN** the page reads PCBA-TEST v1
- **THEN** it shows The module LED stays steady white while the test image has power. That lamp is not the board verdict.

#### Scenario: Script hides the banner sentence
- **WHEN** the script reads PCBA-TEST v1
- **THEN** it does not show that lamp sentence

### Requirement: Hosts name the board revision
The script, the page, and the procedure card MUST say the test is compatible with main control board rev 3.

#### Scenario: Script names the revision
- **WHEN** the script starts
- **THEN** it shows Compatible with main control board rev 3.

#### Scenario: Page names the revision
- **WHEN** the page loads
- **THEN** it shows Compatible with main control board rev 3.

#### Scenario: Card names the revision
- **WHEN** the operator reads the procedure card
- **THEN** the card says the test is compatible with main control board rev 3

### Requirement: Web page controls the open port
After the port opens, the page MUST hide Connect and show Disconnect. Disconnect MUST close the port and reload the page. The page MUST keep the serial grant and reopen it when the cable returns, without a reload. After the link drops, Connect MUST return and that click MUST ask for the port again.

#### Scenario: Port is open
- **WHEN** the operator opens the port
- **THEN** Connect is hidden
- **AND** Disconnect is shown

#### Scenario: Operator disconnects
- **WHEN** the operator presses Disconnect
- **THEN** the page closes the port and reloads

#### Scenario: Link drops
- **WHEN** the open port is lost
- **THEN** Connect returns
- **AND** that click asks for the port again

### Requirement: Page names the script and the procedure
The page MUST name `python3 scripts/pcba_test.py` and MUST link the procedure at `https://github.com/jamro/tiny-engineer/blob/main/docs/hardware/pcba-test.md`.

#### Scenario: Script path on the page
- **WHEN** the page loads in a browser that can run the test
- **THEN** the page names python3 scripts/pcba_test.py
- **AND** it links the procedure doc on GitHub

### Requirement: Script waits on a busy port
When the script cannot open the board port because it is busy, it MUST say the port is busy and MUST keep waiting. It MUST NOT exit with a traceback.

#### Scenario: Another program holds the port
- **WHEN** the board port is busy
- **THEN** the script shows Port is busy. Close the other program using the board. This script will keep waiting.
- **AND** the script stays running

### Requirement: Hosts require the test firmware
Both hosts MUST say the ESP32 must already run the test firmware. The script MUST print, and the page MUST link, the procedure section on how to get one. The procedure MUST say the product firmware cannot run this test and MUST include the pcba-test upload command.

#### Scenario: Script names the firmware
- **WHEN** the script starts
- **THEN** it shows The ESP32 must already run the test firmware.
- **AND** it shows https://github.com/jamro/tiny-engineer/blob/main/docs/hardware/pcba-test.md#get-an-esp32-with-the-test-firmware

#### Scenario: Page names the firmware
- **WHEN** the page loads
- **THEN** it shows The ESP32 must already run the test firmware.
- **AND** it links https://github.com/jamro/tiny-engineer/blob/main/docs/hardware/pcba-test.md#get-an-esp32-with-the-test-firmware

#### Scenario: Card explains how to get one
- **WHEN** the operator reads how to get the test firmware
- **THEN** the card says the product firmware cannot run this test
- **AND** it includes pio run -e pcba-test -t upload
