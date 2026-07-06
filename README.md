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
- LCD touch UI controls for:
  - Current indoor temperature and target setpoint
  - HVAC mode selection from the main thermostat view
  - Fan mode selection from the main thermostat view
  - Quick actions and status indicators

## Using `thermostat.yaml`

`thermostat.yaml` is intended to be used as an ESPHome package. It includes the
hardware, display, touch, LVGL thermostat UI, Home Assistant climate bindings,
wake word, and voice assistant configuration.

It intentionally does **not** include `api`, `wifi`, or `ota`. Keep those in
your own device config so credentials and deployment settings stay local.

The reusable config is split into focused module directories. Each standard
module uses `module.yaml` as its entrypoint, with any related C++ lambda files
kept beside it.

- `thermostat.yaml`: public entrypoint, substitutions, and package wiring.
- `modules/board/`: Waveshare board platform, buses, display/touch hardware, audio hardware, and physical buttons.
- `modules/base/`: optional sensors from the base PCB.
- `modules/screen/`: screen timeout, brightness state, and wake behavior.
- `modules/climate/`: Home Assistant climate sensors/actions and thermostat UI update lambdas.
- `modules/voice/`: wake word and voice assistant behavior.
- `modules/controls/`: Home Assistant-facing touchscreen, microphone, and volume controls.
- `modules/ui/`: fonts, LVGL root config, page fragments, and voice overlay layer.

The lambda files begin with `|-` because ESPHome parses `!include` targets as
YAML. The rest of each file is the C++ lambda body, which keeps the thermostat
logic readable while still working when the package is imported from GitHub.

Create a parent ESPHome config, for example `my-thermostat.yaml`:

```yaml
substitutions:
  climate_entity: climate.living_room
  name: living-room-thermostat
  friendly_name: Living Room Thermostat
  base_light_sensor: none
  base_thermometer: none

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
- `fan_mode_1` through `fan_mode_5`: fallback fan mode service values used before Home Assistant publishes the climate entity's `fan_modes` capability list. Once available, up to eight reported fan modes are shown automatically.
- `base_light_sensor`: set to `present` when the optional base PCB has the VEML7700 light sensor populated; leave as `none` otherwise.
- `base_thermometer`: set to `present` when the optional base PCB has the SHT45 temperature/humidity sensor populated; leave as `none` otherwise.
- `base_light_sensor_id` / `base_light_sensor_name`: ESPHome ID and Home Assistant name for the VEML7700 ambient light reading.
- `base_light_sensor_internal`: keep the VEML7700 ambient light reading internal by default; set to `false` to expose it to Home Assistant.
- `base_thermometer_id`: ESPHome ID for the SHT45 component.
- `base_temperature_sensor_id` / `base_temperature_sensor_name`: ESPHome ID and Home Assistant name for the SHT45 temperature reading.
- `base_humidity_sensor_id` / `base_humidity_sensor_name`: ESPHome ID and Home Assistant name for the SHT45 humidity reading.

When `base_light_sensor` is `present`, the VEML7700 reading is used locally to
adjust the fully lit screen brightness and to wake the dimmed display when a
fast ambient-light change is detected. The light sensor is kept internal to
ESPHome by default. These substitutions tune that behavior:

- `light_sensor_min_brightness`: minimum full-screen brightness used by auto brightness, default `0.45`.
- `light_sensor_low_lux` / `light_sensor_high_lux`: lux range mapped to the brightness range, default `2.0` to `300.0`.
- `light_sensor_presence_delta_lux`: minimum lux jump that can wake the screen, default `35.0`.
- `light_sensor_presence_delta_ratio`: relative lux jump that can wake the screen, default `0.35`.
- `light_sensor_presence_ignore_after_screen_ms`: debounce after screen state changes, default `3000`.

When `base_thermometer` is `present`, the SHT45 is exposed to Home Assistant
as `Base Temperature` and `Base Humidity` by default. The thermostat UI does not consume
these readings directly; configure your Home Assistant climate entity separately
if you want to use `Base Temperature` as the room temperature source.

## License

This project is licensed under the MIT License.

Copyright (c) 2026 Olivier Poitrey
