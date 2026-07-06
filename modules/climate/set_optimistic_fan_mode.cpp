|-
  auto fan_icon = [](const std::string &s) -> const char * {
    if (s == "off") return "\U000F081D";
    if (s == "auto") return "\U000F171D";
    if (s == "quiet") return "\U000F1470";
    if (s == "low" || s == "1") return "\U000F1472";
    if (s == "medium" || s == "med" || s == "2") return "\U000F1473";
    if (s == "high" || s == "3") return "\U000F1474";
    for (const char c : s) {
      if (c < '0' || c > '9') return "\U000F0210";
    }
    return s.empty() ? "\U000F0210" : "\U000F146F";
  };
  auto is_numeric = [](const std::string &s) {
    if (s.empty()) return false;
    for (const char c : s) {
      if (c < '0' || c > '9') return false;
    }
    return true;
  };

  std::string value;
  switch (x) {
    case 0: value = id(fan_mode_slot_1); break;
    case 1: value = id(fan_mode_slot_2); break;
    case 2: value = id(fan_mode_slot_3); break;
    case 3: value = id(fan_mode_slot_4); break;
    case 4: value = id(fan_mode_slot_5); break;
    case 5: value = id(fan_mode_slot_6); break;
    case 6: value = id(fan_mode_slot_7); break;
    case 7: value = id(fan_mode_slot_8); break;
    default: break;
  }

  if (!value.empty() && value != "unknown" && value != "unavailable") {
    id(optimistic_fan_mode) = value;
    id(optimistic_fan_mode_until_ms) = millis() + 8000;
    lv_label_set_text(id(lbl_fan_dropdown_icon), fan_icon(value));
    lv_label_set_text(id(lbl_fan_dropdown_text), is_numeric(value) ? "MANUAL" : text.c_str());
  }
