|-
  int rounded = ((int) lroundf(x / ${thermostat_step_tenths})) * ${thermostat_step_tenths};
  if (rounded < ${thermostat_min_tenths}) rounded = ${thermostat_min_tenths};
  if (rounded > ${thermostat_max_tenths}) rounded = ${thermostat_max_tenths};

  id(pending_target_temperature) = rounded / 10.0f;
  id(optimistic_target_temperature_until_ms) = millis() + 8000;

  char buf[16];
  snprintf(buf, sizeof(buf), "%.1f°", id(pending_target_temperature));
  lv_label_set_text(id(lbl_target_temp), buf);
  lv_arc_set_value(id(arc_thermostat), rounded);
