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
  auto mode_name = [&](const std::string &s) {
    if (s == "off") return std::string("OFF");
    if (s == "heat") return std::string("HEAT");
    if (s == "cool") return std::string("COOL");
    if (s == "heat_cool" || s == "auto") return std::string("AUTO");
    if (s == "dry") return std::string("DRY");
    if (s == "fan_only") return std::string("FAN");
    return upper(s);
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
  auto is_numeric = [](const std::string &s) {
    if (s.empty()) return false;
    for (const char c : s) {
      if (c < '0' || c > '9') return false;
    }
    return true;
  };
  auto fan_name = [&](const std::string &s) {
    if (s.empty() || s == "unknown" || s == "unavailable") return std::string("--");
    if (is_numeric(s)) return std::string("SPEED ") + s;
    return upper(s);
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

  const float current = id(ha_current_temperature).state;
  const float ha_target = id(ha_target_temperature).state;
  float target = ha_target;
  const std::string hvac_modes = id(ha_hvac_modes).state;
  std::string display_mode = id(ha_climate_state).state;
  std::string fan = id(ha_fan_mode).state;
  const std::string fan_modes = id(ha_fan_modes).state;
  const bool optimistic_target_active =
      (int32_t) (millis() - id(optimistic_target_temperature_until_ms)) < 0;
  const bool optimistic_mode_active =
      !id(optimistic_hvac_mode).empty() &&
      (int32_t) (millis() - id(optimistic_hvac_mode_until_ms)) < 0;
  const bool optimistic_fan_active =
      !id(optimistic_fan_mode).empty() &&
      (int32_t) (millis() - id(optimistic_fan_mode_until_ms)) < 0;

  if (optimistic_target_active) {
    target = id(pending_target_temperature);
  } else if (std::isnan(ha_target)) {
    target = NAN;
  } else {
    id(optimistic_target_temperature_until_ms) = 0;
  }

  if (optimistic_mode_active) {
    display_mode = id(optimistic_hvac_mode);
  } else {
    id(optimistic_hvac_mode).clear();
    id(optimistic_hvac_mode_until_ms) = 0;
  }

  if (optimistic_fan_active) {
    fan = id(optimistic_fan_mode);
  } else {
    id(optimistic_fan_mode).clear();
    id(optimistic_fan_mode_until_ms) = 0;
  }
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

  std::vector<std::string> mode_options;
  int mode_selected = 0;
  bool mode_matched = false;
  auto add_mode_option = [&](const std::string &service, const std::string &label) {
    if (service.empty()) return;
    if (display_mode == service ||
        (label == "AUTO" && (display_mode == "heat_cool" || display_mode == "auto"))) {
      mode_selected = mode_options.size();
      mode_matched = true;
    }
    mode_options.push_back(label);
  };

  if (supports_mode("off")) add_mode_option("off", "OFF");
  if (supports_mode("heat")) add_mode_option("heat", "HEAT");
  if (supports_mode("cool")) add_mode_option("cool", "COOL");
  if (supports_auto) add_mode_option(supports_mode("auto") && !supports_mode("heat_cool") ? "auto" : "heat_cool", "AUTO");
  if (supports_mode("dry")) add_mode_option("dry", "DRY");
  if (supports_mode("fan_only")) add_mode_option("fan_only", "FAN");
  if (!mode_matched && !display_mode.empty() && display_mode != "unknown" && display_mode != "unavailable") {
    mode_selected = mode_options.size();
    mode_options.push_back(mode_name(display_mode));
  }
  if (mode_options.empty()) mode_options.push_back("--");

  id(dd_mode).set_options(mode_options);
  id(dd_mode).set_selected_index(mode_selected, LV_ANIM_OFF);
  lv_dropdown_set_text(id(dd_mode).obj, "");
  lv_label_set_text(id(lbl_mode_dropdown_icon), mode_icon(display_mode));
  lv_label_set_text(id(lbl_mode_dropdown_text), mode_options[mode_selected].c_str());

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
  add_fan_mode(fan);

  id(fan_mode_slot_1) = fan_slots[0];
  id(fan_mode_slot_2) = fan_slots[1];
  id(fan_mode_slot_3) = fan_slots[2];
  id(fan_mode_slot_4) = fan_slots[3];
  id(fan_mode_slot_5) = fan_slots[4];
  id(fan_mode_slot_6) = fan_slots[5];
  id(fan_mode_slot_7) = fan_slots[6];
  id(fan_mode_slot_8) = fan_slots[7];

  std::vector<std::string> fan_options;
  int fan_selected = 0;
  for (int i = 0; i < fan_count; i++) {
    fan_options.push_back(fan_name(fan_slots[i]));
    if (fan_slots[i] == fan) fan_selected = i;
  }
  if (fan_options.empty()) fan_options.push_back("--");

  id(dd_fan).set_options(fan_options);
  id(dd_fan).set_selected_index(fan_selected, LV_ANIM_OFF);
  lv_dropdown_set_text(id(dd_fan).obj, "");
  lv_label_set_text(id(lbl_fan_dropdown_icon), fan_icon(fan));
  lv_label_set_text(id(lbl_fan_dropdown_text), fan_options[fan_selected].c_str());
