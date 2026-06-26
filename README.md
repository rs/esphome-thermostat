# ESPHome Thermostat UI (Waveshare ESP32-S3 Touch LCD 4)

Project documentation for an **ESPHome-based thermostat UI** built on the **Waveshare ESP32-S3-Touch-LCD-4** platform.

## Project Overview

This project targets a wall-mounted smart thermostat experience powered by ESPHome.

Core goals:
- Touch-first thermostat controls on a 4-inch LCD
- Clean and responsive local UI for temperature, mode, and fan control
- Integration with Home Assistant climate entities
- Voice assistant support for hands-free operation
- Optional temperature sensor attached to the thermostat for remote room temperature sensing

## Hardware Platform

Reference board:
- Waveshare ESP32-S3-Touch-LCD-4

Typical peripherals and capabilities:
- 4-inch touch display (UI rendering + user input)
- ESP32-S3 Wi-Fi connectivity
- GPIO/I2C/SPI expansion for sensors and controls

## Software Stack

- **ESPHome** firmware configuration and component orchestration
- **LVGL UI** implemented through ESPHome for touch-first thermostat screens
- **Home Assistant** integration for climate entities and automation
- LCD touch UI screens for:
  - Current indoor temperature and target setpoint
  - HVAC mode selection (Heat/Cool/Auto/Off)
  - Fan mode controls
  - Quick actions and status indicators

## Getting Started

1. Install ESPHome and ensure your board is recognized.
2. Create a base ESPHome config for ESP32-S3.
3. Add display, touch controller, and LVGL configuration for the Waveshare panel.
4. Define climate controls and bind them to your HVAC/Home Assistant entities.
5. Tune UI performance and touch calibration.

## License

This project is licensed under the MIT License.

Copyright (c) 2026 Olivier Poitrey
