|-
  std::string mode = id(ha_climate_state).state;
  if (!id(optimistic_hvac_mode).empty() &&
      (int32_t) (millis() - id(optimistic_hvac_mode_until_ms)) < 0) {
    mode = id(optimistic_hvac_mode);
  } else {
    id(optimistic_hvac_mode).clear();
    id(optimistic_hvac_mode_until_ms) = 0;
  }
  const float current = id(ha_current_temperature).state;
  const float target = id(pending_target_temperature);

  const bool heat = mode == "heat";
  const bool cool = mode == "cool";
  const bool heat_cool = mode == "heat_cool" || mode == "auto";
  const bool off = mode == "off";

  const uint32_t track_color = cool ? 0x245D91 : heat_cool ? 0x967318 : heat ? 0x984719 : 0x292929;
  const uint32_t delta_color = cool ? 0x2EA3FF : heat_cool ? 0xF5B90A : heat ? 0xFF7626 : 0x606060;
  const uint32_t marker_color = cool ? 0x164978 : heat_cool ? 0x5A4310 : heat ? 0x6E3517 : 0x404040;

  lv_obj_clear_flag(id(arc_track), LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(id(arc_delta), LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(id(obj_current_marker), LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(id(obj_target_marker), LV_OBJ_FLAG_CLICKABLE);

  lv_arc_set_mode(id(arc_thermostat), LV_ARC_MODE_NORMAL);
  lv_arc_set_bg_angles(id(arc_thermostat), 135, 45);
  lv_obj_set_style_arc_color(id(arc_thermostat), lv_color_hex(0x292929), LV_PART_MAIN);
  lv_obj_set_style_arc_opa(id(arc_thermostat), LV_OPA_TRANSP, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(id(arc_thermostat), LV_OPA_TRANSP, LV_PART_KNOB);
  lv_obj_set_style_border_opa(id(arc_thermostat), LV_OPA_TRANSP, LV_PART_KNOB);
  lv_obj_set_style_arc_color(id(arc_track), lv_color_hex(track_color), LV_PART_INDICATOR);
  lv_obj_set_style_arc_opa(id(arc_track), LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(id(arc_track), LV_OPA_TRANSP, LV_PART_KNOB);
  lv_obj_set_style_border_opa(id(arc_track), LV_OPA_TRANSP, LV_PART_KNOB);
  lv_obj_remove_style(id(arc_track), nullptr, LV_PART_KNOB);
  lv_obj_set_style_arc_color(id(arc_delta), lv_color_hex(delta_color), LV_PART_INDICATOR);
  lv_obj_set_style_arc_opa(id(arc_delta), 0, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(id(arc_delta), LV_OPA_TRANSP, LV_PART_KNOB);
  lv_obj_set_style_border_opa(id(arc_delta), LV_OPA_TRANSP, LV_PART_KNOB);
  lv_obj_remove_style(id(arc_delta), nullptr, LV_PART_KNOB);

  lv_obj_set_style_text_color(id(lbl_current_temp), lv_color_hex(off ? 0xA3A3A3 : delta_color), LV_PART_MAIN);

  auto value_angle = [](float value) {
    float tenths = value * 10.0f;
    if (tenths < ${thermostat_min_tenths}) tenths = ${thermostat_min_tenths};
    if (tenths > ${thermostat_max_tenths}) tenths = ${thermostat_max_tenths};
    const float span = (float) (${thermostat_max_tenths} - ${thermostat_min_tenths});
    int angle = (int) lroundf(135.0f + ((tenths - ${thermostat_min_tenths}) / span) * 270.0f);
    angle %= 360;
    if (angle < 0) angle += 360;
    return angle;
  };

  auto place_marker = [](lv_obj_t *obj, int angle, int diameter) {
    const float radians = angle * 0.01745329252f;
    const float radius = 192.0f;
    const float cx = 240.0f;
    const float cy = 236.0f;
    const int x = (int) lroundf(cx + cosf(radians) * radius - diameter / 2.0f);
    const int y = (int) lroundf(cy + sinf(radians) * radius - diameter / 2.0f);
    lv_obj_set_pos(obj, x, y);
  };

  const int target_angle = value_angle(target);
  if (off) {
    lv_obj_add_flag(id(arc_track), LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_arc_set_bg_angles(id(arc_track), cool ? target_angle : 135, (cool || heat_cool) ? 45 : target_angle);
    lv_arc_set_value(id(arc_track), 100);
    lv_obj_clear_flag(id(arc_track), LV_OBJ_FLAG_HIDDEN);
  }

  place_marker(id(obj_target_marker), target_angle, 34);
  lv_obj_set_style_bg_color(id(obj_target_marker), lv_color_hex(0xF8FAFC), LV_PART_MAIN);
  lv_obj_set_style_border_color(id(obj_target_marker), lv_color_hex(heat_cool ? delta_color : 0xF8FAFC), LV_PART_MAIN);
  lv_obj_set_style_border_width(id(obj_target_marker), heat_cool ? 4 : 0, LV_PART_MAIN);
  lv_obj_clear_flag(id(obj_target_marker), LV_OBJ_FLAG_HIDDEN);

  const bool has_current = !std::isnan(current);
  if (has_current) {
    const int current_angle = value_angle(current);
    place_marker(id(obj_current_marker), current_angle, 14);
    lv_obj_set_style_bg_color(id(obj_current_marker), lv_color_hex(marker_color), LV_PART_MAIN);
    lv_obj_clear_flag(id(obj_current_marker), LV_OBJ_FLAG_HIDDEN);

    const bool show_heat_delta = heat && target > current;
    const bool show_cool_delta = cool && target < current;
    if (show_heat_delta || show_cool_delta) {
      lv_arc_set_bg_angles(id(arc_delta),
                           show_heat_delta ? current_angle : target_angle,
                           show_heat_delta ? target_angle : current_angle);
      lv_arc_set_value(id(arc_delta), 100);
      lv_obj_clear_flag(id(arc_delta), LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(id(arc_delta), LV_OBJ_FLAG_HIDDEN);
    }
  } else {
    lv_obj_add_flag(id(obj_current_marker), LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(id(arc_delta), LV_OBJ_FLAG_HIDDEN);
  }
