#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.
// Requires button_grid_modal.h (control_modal_* helpers) and screen_lock_state.h to already
// be included, so this is pulled into button_grid.h after both.

#include <array>
#include <string>

#include "screen_lock_pin.h"
#include "screen_lock_state.h"

struct ScreenLockPinPadUi {
  lv_obj_t *overlay = nullptr;
  std::array<lv_obj_t *, SCREEN_LOCK_PIN_LENGTH> dots{};
  std::array<lv_obj_t *, 9> keys{};
  std::string buffer;
  bool flashing = false;
  lv_timer_t *flash_timer = nullptr;
};

inline ScreenLockPinPadUi &screen_lock_pin_pad_ui() {
  static ScreenLockPinPadUi ui;
  return ui;
}

inline bool screen_lock_pin_pad_showing() {
  return screen_lock_pin_pad_ui().overlay != nullptr;
}

inline void screen_lock_pin_pad_set_dot_color(lv_obj_t *dot, uint32_t color) {
  if (!dot) return;
  lv_obj_set_style_bg_color(dot, lv_color_hex(color), LV_PART_MAIN);
}

inline void screen_lock_pin_pad_update_dots() {
  ScreenLockPinPadUi &ui = screen_lock_pin_pad_ui();
  for (size_t i = 0; i < ui.dots.size(); i++) {
    bool filled = i < ui.buffer.size();
    screen_lock_pin_pad_set_dot_color(
      ui.dots[i], filled ? current_button_primary_color() : SECONDARY_GREY);
  }
}

inline void screen_lock_pin_pad_hide() {
  ScreenLockPinPadUi &ui = screen_lock_pin_pad_ui();
  if (ui.flash_timer) {
    lv_timer_del(ui.flash_timer);
    ui.flash_timer = nullptr;
  }
  lv_obj_t *overlay = ui.overlay;
  ui = ScreenLockPinPadUi();
  if (overlay) lv_obj_del(overlay);
}

inline void screen_lock_pin_pad_flash_done_cb(lv_timer_t *timer) {
  ScreenLockPinPadUi &ui = screen_lock_pin_pad_ui();
  if (ui.flash_timer == timer) ui.flash_timer = nullptr;
  lv_timer_del(timer);
  if (!screen_lock_pin_pad_showing()) return;
  ui.flashing = false;
  ui.buffer.clear();
  screen_lock_pin_pad_update_dots();
  for (lv_obj_t *key : ui.keys) {
    if (key) lv_obj_set_style_bg_color(key, lv_color_hex(SECONDARY_GREY), LV_PART_MAIN);
  }
}

// Holds the dots and keypad in red for a beat so a wrong PIN is unmistakable,
// then clears the attempt so the panel is ready for another try.
inline void screen_lock_pin_pad_flash_wrong() {
  ScreenLockPinPadUi &ui = screen_lock_pin_pad_ui();
  ui.flashing = true;
  for (lv_obj_t *dot : ui.dots) {
    screen_lock_pin_pad_set_dot_color(dot, ALARM_TRIGGERED_COLOR);
  }
  for (lv_obj_t *key : ui.keys) {
    if (key) lv_obj_set_style_bg_color(key, lv_color_hex(ALARM_TRIGGERED_COLOR), LV_PART_MAIN);
  }
  if (ui.flash_timer) lv_timer_del(ui.flash_timer);
  ui.flash_timer = lv_timer_create(screen_lock_pin_pad_flash_done_cb, 1100, nullptr);
}

inline void screen_lock_pin_pad_submit() {
  ScreenLockPinPadUi &ui = screen_lock_pin_pad_ui();
  if (espcontrol::screen_lock_pin_verify(ui.buffer)) {
    screen_lock_pin_pad_hide();
    screen_lock_set_enabled(false);
    return;
  }
  screen_lock_pin_pad_flash_wrong();
}

inline void screen_lock_pin_pad_key_cb(lv_event_t *e) {
  ScreenLockPinPadUi &ui = screen_lock_pin_pad_ui();
  if (ui.flashing) return;
  const char *digit = static_cast<const char *>(lv_event_get_user_data(e));
  if (!digit) return;
  ui.buffer.push_back(digit[0]);
  screen_lock_pin_pad_update_dots();
  if (ui.buffer.size() >= SCREEN_LOCK_PIN_LENGTH) screen_lock_pin_pad_submit();
}

inline lv_obj_t *screen_lock_pin_pad_create_dot(lv_obj_t *parent, lv_coord_t size) {
  lv_obj_t *dot = lv_obj_create(parent);
  lv_obj_set_size(dot, size, size);
  lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(dot, lv_color_hex(SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);
  lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
  return dot;
}

// A locked screen with a PIN configured shows this instead of the normal
// home screen, on the panel's own top LVGL layer so nothing underneath can
// be reached. It never offers a way to dismiss itself -- the only way out is
// the correct PIN, which is the entire point of Screen Lock's PIN option.
inline void screen_lock_pin_pad_show() {
  if (screen_lock_pin_pad_showing()) return;
  ScreenLockPinPadUi &ui = screen_lock_pin_pad_ui();
  ui.buffer.clear();
  ui.flashing = false;

  ControlModalLayout layout = control_modal_calc_layout(100);
  lv_coord_t short_side = layout.short_side;

  ui.overlay = lv_obj_create(lv_layer_top());
  lv_obj_set_size(ui.overlay, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_color(ui.overlay, lv_color_hex(TERTIARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.overlay, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.overlay, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.overlay, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *column = lv_obj_create(ui.overlay);
  lv_obj_set_size(column, lv_pct(90), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(column, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(column, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(column, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(column, control_modal_scaled_px(20, short_side), LV_PART_MAIN);
  lv_obj_set_layout(column, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(column, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(column, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(column, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_clear_flag(column, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_center(column);

  lv_obj_t *title = lv_label_create(column);
  lv_label_set_display_text(title, espcontrol_i18n("Enter Pin"));
  lv_obj_set_style_text_color(title, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

  lv_obj_t *dots_row = lv_obj_create(column);
  lv_obj_set_size(dots_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(dots_row, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(dots_row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(dots_row, 0, LV_PART_MAIN);
  lv_coord_t dot_gap = control_modal_scaled_px(14, short_side);
  if (dot_gap < 8) dot_gap = 8;
  lv_obj_set_style_pad_column(dots_row, dot_gap, LV_PART_MAIN);
  lv_obj_set_layout(dots_row, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(dots_row, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
  lv_obj_clear_flag(dots_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_coord_t dot_size = control_modal_scaled_px(18, short_side);
  if (dot_size < 12) dot_size = 12;
  if (dot_size > 26) dot_size = 26;
  for (size_t i = 0; i < ui.dots.size(); i++) {
    ui.dots[i] = screen_lock_pin_pad_create_dot(dots_row, dot_size);
  }

  lv_coord_t key_size = control_modal_scaled_px(84, short_side);
  if (key_size < 56) key_size = 56;
  if (key_size > 140) key_size = 140;
  lv_coord_t key_gap = control_modal_scaled_px(16, short_side);
  if (key_gap < 8) key_gap = 8;

  static const char *kKeyDigits[9] = {
    "1", "2", "3",
    "4", "5", "6",
    "7", "8", "9",
  };
  for (int row = 0; row < 3; row++) {
    lv_obj_t *key_row = lv_obj_create(column);
    lv_obj_set_size(key_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(key_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(key_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(key_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(key_row, key_gap, LV_PART_MAIN);
    lv_obj_set_layout(key_row, LV_LAYOUT_FLEX);
    lv_obj_set_style_flex_flow(key_row, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
    lv_obj_clear_flag(key_row, LV_OBJ_FLAG_SCROLLABLE);
    for (int col = 0; col < 3; col++) {
      int index = row * 3 + col;
      lv_obj_t *key_btn = control_modal_create_round_button(
        key_row, key_size, kKeyDigits[index], nullptr, DARK_BORDER, SECONDARY_GREY);
      ui.keys[index] = key_btn;
      lv_obj_add_event_cb(key_btn, screen_lock_pin_pad_key_cb, LV_EVENT_CLICKED,
        const_cast<char *>(kKeyDigits[index]));
    }
  }

  screen_lock_pin_pad_update_dots();
  lv_obj_move_foreground(ui.overlay);
}

// Locking is always allowed with a tap. Unlocking needs the PIN keypad once
// one is configured; without a PIN, a tap keeps Screen Lock's original
// unauthenticated toggle behaviour. Forward-declared in screen_lock_state.h.
inline void screen_lock_toggle() {
  if (!screen_lock_enabled()) {
    screen_lock_set_enabled(true);
    return;
  }
  if (screen_lock_requires_pin()) {
    screen_lock_pin_pad_show();
    return;
  }
  screen_lock_set_enabled(false);
}
