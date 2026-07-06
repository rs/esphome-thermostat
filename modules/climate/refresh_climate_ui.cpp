|-
  auto upper = [](std::string s) {
    for (auto &c : s) c = ::toupper(c);
    return s;
  };
  auto temp_label = [](float value, int precision) {
    if (std::isnan(value)) return std::string("--.-\xC2\xB0");
    char buf[16];
    snprintf(buf, sizeof(buf), precision == 0 ? "%.0f\xC2\xB0" : "%.1f\xC2\xB0", value);
    return std::string(buf);
  };
  auto set_checked = [](lv_obj_t *obj, bool checked) {
    if (checked) {
      lv_obj_add_state(obj, LV_STATE_CHECKED);
    } else {
      lv_obj_clear_state(obj, LV_STATE_CHECKED);
    }
  };
  auto mode_icon = [](const std::string &s) -> const char * {
    if (s == "off") return "\U000F0425";
    if (s == "heat") return "\U000F0599";
    if (s == "cool") return "\U000F0717";
    if (s == "heat_cool" || s == "auto") return "\U000F1796";
    return "\U000F1796";
  };

  const float current = id(ha_current_temperature).state;
  const float target = id(ha_target_temperature).state;
  const std::string mode = id(ha_climate_state).state;
  const std::string fan = id(ha_fan_mode).state;

  if (!std::isnan(current)) {
    lv_label_set_text(id(lbl_current_temp), temp_label(current, 1).c_str());
  } else {
    lv_label_set_text(id(lbl_current_temp), "--.-\xC2\xB0");
  }

  if (!std::isnan(target)) {
    id(pending_target_temperature) = target;
    lv_label_set_text(id(lbl_target_temp), temp_label(target, 1).c_str());
    lv_arc_set_value(id(arc_thermostat), (int) lroundf(target * 10.0f));
  }

  lv_label_set_text(id(lbl_mode_value), upper(mode).c_str());
  lv_label_set_text(id(lbl_mode_chip), mode_icon(mode));
  lv_label_set_text(id(lbl_fan_value), upper(fan).c_str());
  lv_label_set_text(id(lbl_fan_chip), (fan.empty() || fan == "unknown" ? "--" : upper(fan)).c_str());

  set_checked(id(btn_mode_off), mode == "off");
  set_checked(id(btn_mode_heat), mode == "heat");
  set_checked(id(btn_mode_cool), mode == "cool");
  set_checked(id(btn_mode_auto), mode == "heat_cool" || mode == "auto");

  set_checked(id(btn_fan_1), fan == "${fan_mode_1}");
  set_checked(id(btn_fan_2), fan == "${fan_mode_2}");
  set_checked(id(btn_fan_3), fan == "${fan_mode_3}");
  set_checked(id(btn_fan_4), fan == "${fan_mode_4}");
  set_checked(id(btn_fan_5), fan == "${fan_mode_5}");
