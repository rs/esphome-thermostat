|-
  if (std::isnan(x) || x < 0.0f) return;

  const uint32_t now = millis();
  const float previous = id(ambient_light_previous_lux);
  if (previous >= 0.0f) {
    const float delta = fabsf(x - previous);
    const float scaled = fmaxf(fmaxf(x, previous) * ${light_sensor_presence_delta_ratio}, ${light_sensor_presence_delta_lux});
    if (!id(screen_on) &&
        delta >= scaled &&
        now - id(last_screen_state_change_ms) > ${light_sensor_presence_ignore_after_screen_ms}) {
      id(last_touch_ms) = now;
      id(wake_screen).execute();
    }
  }
  id(ambient_light_previous_lux) = x;

  if (!id(screen_on)) return;

  auto clamp_brightness = [](float value) {
    if (value < 0.05f) return 0.05f;
    if (value > 1.0f) return 1.0f;
    return value;
  };

  const float low_lux = fmaxf(${light_sensor_low_lux}, 0.0f);
  const float high_lux = fmaxf(${light_sensor_high_lux}, low_lux + 1.0f);
  const float min_brightness = clamp_brightness(${light_sensor_min_brightness});
  float max_brightness = clamp_brightness(id(screen_brightness));
  if (max_brightness < min_brightness) max_brightness = min_brightness;

  float ratio = (log10f(x + 1.0f) - log10f(low_lux + 1.0f)) /
                (log10f(high_lux + 1.0f) - log10f(low_lux + 1.0f));
  if (ratio < 0.0f) ratio = 0.0f;
  if (ratio > 1.0f) ratio = 1.0f;

  const float target = min_brightness + ratio * (max_brightness - min_brightness);
  const float smoothed = id(ambient_light_last_brightness) < 0.0f
      ? target
      : id(ambient_light_last_brightness) * 0.75f + target * 0.25f;

  if (fabsf(smoothed - id(screen_active_brightness)) >= 0.015f) {
    id(screen_active_brightness) = smoothed;
    auto call = id(display_backlight).make_call();
    call.set_state(true);
    call.set_brightness(smoothed);
    call.perform();
  }
  id(ambient_light_last_brightness) = smoothed;
