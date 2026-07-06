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
  auto set_hidden = [](lv_obj_t *obj, bool hidden) {
    if (hidden) {
      lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
  };
  auto token_contains = [](const std::string &text, const std::string &needle) {
    size_t pos = 0;
    while (pos < text.size()) {
      while (pos < text.size()) {
        const char c = text[pos];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_') {
          break;
        }
        pos++;
      }
      const size_t start = pos;
      while (pos < text.size()) {
        const char c = text[pos];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_')) {
          break;
        }
        pos++;
      }
      if (pos > start && text.substr(start, pos - start) == needle) return true;
    }
    return false;
  };
  auto mode_icon = [](const std::string &s) -> const char * {
    if (s == "off") return "\U000F0425";
    if (s == "heat") return "\U000F0599";
    if (s == "cool") return "\U000F0717";
    if (s == "heat_cool" || s == "auto") return "\U000F1796";
    if (s == "dry") return "\U000F1A45";
    if (s == "fan_only") return "\U000F0210";
    return "\U000F1796";
  };
  auto mode_name = [&](const std::string &s) {
    if (s == "off") return std::string("OFF");
    if (s == "heat") return std::string("HEAT");
    if (s == "cool") return std::string("COOL");
    if (s == "heat_cool" || s == "auto") return std::string("AUTO");
    if (s == "dry") return std::string("DRY");
    if (s == "fan_only") return std::string("FAN");
    return upper(s);
  };
  auto is_numeric = [](const std::string &s) {
    if (s.empty()) return false;
    for (const char c : s) {
      if (c < '0' || c > '9') return false;
    }
    return true;
  };
  auto fan_icon = [&](const std::string &s) -> const char * {
    if (s == "off") return "\U000F081D";
    if (s == "auto") return "\U000F171D";
    if (s == "quiet") return "\U000F1470";
    if (s == "low" || s == "1") return "\U000F1472";
    if (s == "medium" || s == "med" || s == "2") return "\U000F1473";
    if (s == "high" || s == "3") return "\U000F1474";
    // MDI 7.4.47 has fan-plus, but not fan-speed-plus.
    if (is_numeric(s)) return "\U000F146F";
    return "\U000F0210";
  };
  auto fan_name = [&](const std::string &s) {
    if (s.empty() || s == "unknown" || s == "unavailable") return std::string("--");
    return upper(s);
  };

  const float current = id(ha_current_temperature).state;
  const float target = id(ha_target_temperature).state;
  const std::string mode = id(ha_climate_state).state;
  const std::string hvac_modes = id(ha_hvac_modes).state;
  const std::string fan = id(ha_fan_mode).state;
  const std::string fan_modes = id(ha_fan_modes).state;
  const bool hvac_modes_known = !(hvac_modes.empty() ||
                                  hvac_modes == "unknown" ||
                                  hvac_modes == "unavailable");
  const bool fan_modes_known = !(fan_modes.empty() ||
                                 fan_modes == "unknown" ||
                                 fan_modes == "unavailable");
  auto supports_mode = [&](const std::string &s) {
    if (!hvac_modes_known) {
      return s == "off" || s == "heat" || s == "cool" || s == "heat_cool" || s == "auto";
    }
    return token_contains(hvac_modes, s);
  };
  const bool supports_auto = supports_mode("heat_cool") || supports_mode("auto");

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

  lv_label_set_text(id(lbl_mode_value), mode_name(mode).c_str());
  lv_label_set_text(id(lbl_mode_chip), mode_icon(mode));
  lv_label_set_text(id(lbl_mode_chip_text), mode_name(mode).c_str());
  lv_label_set_text(id(lbl_fan_icon), fan_icon(fan));
  lv_label_set_text(id(lbl_fan_value), fan_name(fan).c_str());
  lv_label_set_text(id(lbl_fan_chip), fan_name(fan).c_str());

  set_checked(id(btn_mode_off), mode == "off");
  set_checked(id(btn_mode_heat), mode == "heat");
  set_checked(id(btn_mode_cool), mode == "cool");
  set_checked(id(btn_mode_auto), mode == "heat_cool" || mode == "auto");
  set_checked(id(btn_mode_dry), mode == "dry");
  set_checked(id(btn_mode_fan_only), mode == "fan_only");

  set_hidden(id(btn_mode_off), !supports_mode("off"));
  set_hidden(id(btn_mode_heat), !supports_mode("heat"));
  set_hidden(id(btn_mode_cool), !supports_mode("cool"));
  set_hidden(id(btn_mode_auto), !supports_auto);
  set_hidden(id(btn_mode_dry), !supports_mode("dry"));
  set_hidden(id(btn_mode_fan_only), !supports_mode("fan_only"));

  std::string fan_slots[8];
  int fan_count = 0;
  auto add_fan_mode = [&](const std::string &value) {
    if (value.empty() || value == "unknown" || value == "unavailable") return;
    for (int i = 0; i < fan_count; i++) {
      if (fan_slots[i] == value) return;
    }
    if (fan_count < 8) {
      fan_slots[fan_count++] = value;
    }
  };
  auto collect_modes = [&](const std::string &text) {
    size_t pos = 0;
    while (pos < text.size()) {
      while (pos < text.size()) {
        const char c = text[pos];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_') {
          break;
        }
        pos++;
      }
      const size_t start = pos;
      while (pos < text.size()) {
        const char c = text[pos];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_')) {
          break;
        }
        pos++;
      }
      if (pos > start) add_fan_mode(text.substr(start, pos - start));
    }
  };
  if (fan_modes_known) {
    collect_modes(fan_modes);
  } else {
    add_fan_mode("${fan_mode_1}");
    add_fan_mode("${fan_mode_2}");
    add_fan_mode("${fan_mode_3}");
    add_fan_mode("${fan_mode_4}");
    add_fan_mode("${fan_mode_5}");
  }
  if (fan_count == 0) add_fan_mode(fan);

  id(fan_mode_slot_1) = fan_slots[0];
  id(fan_mode_slot_2) = fan_slots[1];
  id(fan_mode_slot_3) = fan_slots[2];
  id(fan_mode_slot_4) = fan_slots[3];
  id(fan_mode_slot_5) = fan_slots[4];
  id(fan_mode_slot_6) = fan_slots[5];
  id(fan_mode_slot_7) = fan_slots[6];
  id(fan_mode_slot_8) = fan_slots[7];

  auto update_fan_slot = [&](lv_obj_t *button, lv_obj_t *icon, lv_obj_t *label, const std::string &value) {
    const bool hidden = value.empty();
    set_hidden(button, hidden);
    set_checked(button, !hidden && fan == value);
    if (!hidden) {
      lv_label_set_text(icon, fan_icon(value));
      lv_label_set_text(label, fan_name(value).c_str());
    }
  };
  update_fan_slot(id(btn_fan_1), id(lbl_fan_1_icon), id(lbl_fan_1_value), id(fan_mode_slot_1));
  update_fan_slot(id(btn_fan_2), id(lbl_fan_2_icon), id(lbl_fan_2_value), id(fan_mode_slot_2));
  update_fan_slot(id(btn_fan_3), id(lbl_fan_3_icon), id(lbl_fan_3_value), id(fan_mode_slot_3));
  update_fan_slot(id(btn_fan_4), id(lbl_fan_4_icon), id(lbl_fan_4_value), id(fan_mode_slot_4));
  update_fan_slot(id(btn_fan_5), id(lbl_fan_5_icon), id(lbl_fan_5_value), id(fan_mode_slot_5));
  update_fan_slot(id(btn_fan_6), id(lbl_fan_6_icon), id(lbl_fan_6_value), id(fan_mode_slot_6));
  update_fan_slot(id(btn_fan_7), id(lbl_fan_7_icon), id(lbl_fan_7_value), id(fan_mode_slot_7));
  update_fan_slot(id(btn_fan_8), id(lbl_fan_8_icon), id(lbl_fan_8_value), id(fan_mode_slot_8));
