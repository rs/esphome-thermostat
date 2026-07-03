# ESPHome Thermostat UI (Waveshare ESP32-S3-Touch-LCD-4B)

Project documentation for an **ESPHome-based thermostat UI** built on the
**[Waveshare ESP32-S3-Touch-LCD-4B](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4B)**
platform.

## Project Overview

This project targets a wall-mounted smart thermostat experience powered by ESPHome.

Core goals:
- Touch-first thermostat controls on a 4-inch LCD
- Clean and responsive local UI for temperature, mode, and fan control
- Integration with Home Assistant climate entities
- Voice assistant support for hands-free operation
- Optional temperature sensor attached to the thermostat for remote room temperature sensing

## Hardware Platform

Target hardware:
- **[Waveshare ESP32-S3-Touch-LCD-4B](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4B)**

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

## Using `thermostat.yaml`

`thermostat.yaml` is intended to be used as an ESPHome package. It includes the
hardware, display, touch, LVGL thermostat UI, Home Assistant climate bindings,
wake word, and voice assistant configuration.

It intentionally does **not** include `api`, `wifi`, or `ota`. Keep those in
your own device config so credentials and deployment settings stay local.

Create a parent ESPHome config, for example `my-thermostat.yaml`:

```yaml
substitutions:
  climate_entity: climate.living_room
  name: living-room-thermostat
  friendly_name: Living Room Thermostat

packages:
  thermostat: github://rs/esphome-thermostat/thermostat.yaml@main

api:
  encryption:
    key: !secret api_encryption_key

ota:
  - platform: esphome

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  fast_connect: true
  ap:
    ssid: "${friendly_name} Fallback"
    password: !secret fallback_ap_password

captive_portal:
```

Then validate and build:

```sh
esphome config my-thermostat.yaml
esphome compile my-thermostat.yaml
esphome upload my-thermostat.yaml
```

In Home Assistant, open the ESPHome integration entry for the thermostat and
enable service/action calls for the device. The UI uses `homeassistant.action`
to call `climate.set_temperature`, `climate.set_hvac_mode`, and
`climate.set_fan_mode`; if service calls are disabled, the display can still
read the climate entity but changes made on the screen will not update Home
Assistant.

Required secrets:

```yaml
api_encryption_key: ...
wifi_ssid: ...
wifi_password: ...
fallback_ap_password: ...
```

Useful substitutions:

- `climate_entity`: Home Assistant climate entity to control.
- `thermostat_min_tenths` / `thermostat_max_tenths`: setpoint range in tenths of a degree, defaulting to `180` to `320` for 18-32 °C.
- `thermostat_step_tenths`: setpoint step in tenths of a degree, for example `5` for 0.5 degree steps.
- `fan_mode_1` through `fan_mode_5`: fan mode service values sent to Home Assistant.

## License

This project is licensed under the MIT License.

Copyright (c) 2026 Olivier Poitrey
