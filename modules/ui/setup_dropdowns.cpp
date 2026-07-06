lv_dropdown_set_symbol(id(dd_mode).obj, nullptr);
lv_dropdown_set_symbol(id(dd_fan).obj, nullptr);
lv_dropdown_set_text(id(dd_mode).obj, "");
lv_dropdown_set_text(id(dd_fan).obj, "");
lv_obj_set_style_text_align(id(dd_mode).obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
lv_obj_set_style_text_align(id(dd_fan).obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
lv_obj_add_state(id(dd_mode).obj, LV_STATE_DISABLED);
lv_obj_add_state(id(dd_fan).obj, LV_STATE_DISABLED);

auto dropdown_open_cb = [](lv_event_t *event) {
  id(dropdown_arc_guard_until_ms) = millis() + 250;
  lv_obj_clear_flag(id(arc_thermostat), LV_OBJ_FLAG_CLICKABLE);
};

auto dropdown_close_cb = [](lv_event_t *event) {
  id(dropdown_arc_guard_until_ms) = millis() + 250;
  lv_async_call([](void *) {
    lv_obj_add_flag(id(arc_thermostat), LV_OBJ_FLAG_CLICKABLE);
  }, nullptr);
};

lv_obj_add_event_cb(id(dd_mode).obj, dropdown_open_cb, LV_EVENT_READY, nullptr);
lv_obj_add_event_cb(id(dd_mode).obj, dropdown_close_cb, LV_EVENT_CANCEL, nullptr);
lv_obj_add_event_cb(id(dd_mode).obj, dropdown_close_cb, LV_EVENT_VALUE_CHANGED, nullptr);
lv_obj_add_event_cb(id(dd_fan).obj, dropdown_open_cb, LV_EVENT_READY, nullptr);
lv_obj_add_event_cb(id(dd_fan).obj, dropdown_close_cb, LV_EVENT_CANCEL, nullptr);
lv_obj_add_event_cb(id(dd_fan).obj, dropdown_close_cb, LV_EVENT_VALUE_CHANGED, nullptr);
