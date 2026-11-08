# ESP32 Embedded Learning

A beginner-friendly embedded development project using **ESP32**, **C++**, and **PlatformIO**.

The goal is to learn embedded programming step by step through practical experiments with GPIO, LEDs, buttons, and an OLED display.

## Hardware

- ESP32-32U (CP2102)
- 0.96" OLED display (SSD1306, 128×64, I2C)
- LEDs
- Tactile push button
- MB-102 breadboard
- Potentiometer (variable resistor)

## Technologies

- C++
- Arduino Framework
- PlatformIO
- Visual Studio Code

## Current Features

- Non-blocking LED blinking using `millis()`
- Button input with `INPUT_PULLUP`
- Button debouncing
- Short-press and long-press detection
- OLED display communication over I2C

## Getting Started

1. Install [Visual Studio Code](https://code.visualstudio.com/).
2. Install the PlatformIO IDE extension.
3. Clone this repository.
4. Open the project in VS Code.
5. Connect the ESP32 via USB.
6. Build and upload:

```bash
pio run
pio run -t upload
pio device monitor
```

## Learning Goals

- Understand GPIO and digital signals.
- Learn non-blocking programming with `millis()`.
- Work with buttons and state machines.
- Understand I2C communication.
- Display sensor data and system information.
- Build practical embedded projects.

---

**Status:** Work in progress 🚀