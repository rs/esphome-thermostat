|-
  std::string mode;
  const char *icon = "\U000F1796";

  if (text == "OFF") {
    mode = "off";
    icon = "\U000F0425";
  } else if (text == "HEAT") {
    mode = "heat";
    icon = "\U000F0599";
  } else if (text == "COOL") {
    mode = "cool";
    icon = "\U000F0717";
  } else if (text == "AUTO") {
    mode = "heat_cool";
    icon = "\U000F1796";
  } else if (text == "DRY") {
    mode = "dry";
    icon = "\U000F1A45";
  } else if (text == "FAN") {
    mode = "fan_only";
    icon = "\U000F0210";
  }

  if (!mode.empty()) {
    id(optimistic_hvac_mode) = mode;
    id(optimistic_hvac_mode_until_ms) = millis() + 8000;
    lv_label_set_text(id(lbl_mode_dropdown_icon), icon);
    lv_label_set_text(id(lbl_mode_dropdown_text), text.c_str());
  }
