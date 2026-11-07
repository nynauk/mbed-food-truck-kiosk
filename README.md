# Food-truck ordering kiosk (Mbed OS, STM32)

An interactive ordering display for a model food truck, built by Group 10 (Ayaan Malik, Karim Huessein, Akram
Halhoul Kaffal, Haci Aras and Isabel Geoff) as a team project in the embedded systems module of my BEng at Royal
Holloway. It runs on an STM32 NUCLEO-F091RC with Mbed OS 6.

## What it does

- A 16x2 I2C LCD scrolls a welcome message. Three push buttons (ice cream, burger, soda) switch the message to
  that item, flash its LED for 10 seconds and play a matching audio track.
- A servo swings to 180 degrees when an item is chosen and returns after 10 seconds.
- An LM35 temperature sensor is read through the ADC (10 mV per °C against a 3.3 V reference, plus a fixed 5 °C
  calibration offset) and shown on the top line of the display.
- After 10 seconds without a button press the kiosk returns to the welcome message and plays a welcome track
  once.
- Audio comes from a DFPlayer Mini over serial at 9,600 baud, driven by its 10-byte command frames
  (start byte `0x7E`, version, length, command, parameters).

Everything runs in one loop using separate Mbed `Timer`s for scrolling (200 ms), inactivity, the servo and LED
flashing, so no part of the kiosk blocks another.

| Part | Pins |
|---|---|
| LCD (I2C, address 0x27) | SDA PB_9, SCL PB_8 |
| Buttons (pull-down) | D5, D6, D7 |
| LEDs | D10, D11, D12 |
| Servo (PWM) | PC_7 |
| LM35 | A0 |
| DFPlayer Mini (serial) | TX D8, RX D2 |

## Build

```bash
mbed deploy                       # fetches mbed-os and the LCD library pinned in the .lib files
mbed compile -m NUCLEO_F091RC -t GCC_ARM
```

The LCD driver is [sstaub/mbedLCDi2c](https://github.com/sstaub/mbedLCDi2c), pinned by `mbedlcdi2c.lib`; Mbed OS
itself is pinned by `mbed-os.lib`. Neither is copied into this repository.
