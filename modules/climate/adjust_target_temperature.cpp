|-
  const int step = 5;
  int target_tenths = (int) lroundf(id(pending_target_temperature) * 10.0f);
  target_tenths = ((int) lroundf(target_tenths / (float) step)) * step;
  target_tenths += delta_tenths > 0 ? step : -step;

  if (target_tenths < ${thermostat_min_tenths}) target_tenths = ${thermostat_min_tenths};
  if (target_tenths > ${thermostat_max_tenths}) target_tenths = ${thermostat_max_tenths};

  id(pending_target_temperature) = target_tenths / 10.0f;
  id(optimistic_target_temperature_until_ms) = millis() + 8000;

  char buf[16];
  snprintf(buf, sizeof(buf), "%.1f°", id(pending_target_temperature));
  lv_label_set_text(id(lbl_target_temp), buf);
  lv_arc_set_value(id(arc_thermostat), target_tenths);
