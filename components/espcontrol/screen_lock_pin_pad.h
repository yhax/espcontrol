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
  std::array<lv_obj_t *, 10> keys{};
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

// A phone-style dial pad: 1-9 in a 3x3 grid, with 0 centered on its own row
// underneath, so a PIN can use any of the 10 digits.
static const char *const kScreenLockPinPadDigits[10] = {
  "1", "2", "3",
  "4", "5", "6",
  "7", "8", "9",
  "0",
};

// A locked screen with a PIN configured shows this instead of the normal
// home screen, on the panel's own top LVGL layer so nothing underneath can
// be reached. It never offers a way to dismiss itself -- the only way out is
// the correct PIN, which is the entire point of Screen Lock's PIN option.
//
// Layout works bottom-up from the smallest supported panel (480x480): the
// title's real rendered height is measured first, and the keypad's key size
// is then derived from whatever vertical space is actually left over, so it
// always fits instead of assuming a fixed size that might not.
inline void screen_lock_pin_pad_show() {
  if (screen_lock_pin_pad_showing()) return;
  ScreenLockPinPadUi &ui = screen_lock_pin_pad_ui();
  ui.buffer.clear();
  ui.flashing = false;

  ControlModalLayout layout = control_modal_calc_layout(100);
  lv_coord_t short_side = layout.short_side;
  lv_coord_t sw = layout.sw;
  lv_coord_t sh = layout.sh;

  ui.overlay = lv_obj_create(lv_layer_top());
  lv_obj_set_size(ui.overlay, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_color(ui.overlay, lv_color_hex(TERTIARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.overlay, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.overlay, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.overlay, LV_OBJ_FLAG_SCROLLABLE);

  lv_coord_t outer_inset = control_modal_scaled_px(20, short_side);
  if (outer_inset < 10) outer_inset = 10;

  lv_obj_t *title = lv_label_create(ui.overlay);
  lv_label_set_display_text(title, espcontrol_i18n("Enter Pin"));
  lv_obj_set_style_text_color(title, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_update_layout(title);
  lv_coord_t title_h = lv_obj_get_height(title);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, outer_inset);

  lv_coord_t section_gap = control_modal_scaled_px(16, short_side);
  if (section_gap < 8) section_gap = 8;
  lv_coord_t dot_gap = control_modal_scaled_px(12, short_side);
  if (dot_gap < 6) dot_gap = 6;
  lv_coord_t dot_size = control_modal_scaled_px(16, short_side);
  if (dot_size < 10) dot_size = 10;
  if (dot_size > 22) dot_size = 22;

  lv_coord_t dots_y = outer_inset + title_h + section_gap;
  lv_coord_t dots_total_w =
    dot_size * static_cast<lv_coord_t>(ui.dots.size()) +
    dot_gap * static_cast<lv_coord_t>(ui.dots.size() - 1);
  lv_coord_t dots_start_x = (sw - dots_total_w) / 2;
  for (size_t i = 0; i < ui.dots.size(); i++) {
    ui.dots[i] = screen_lock_pin_pad_create_dot(ui.overlay, dot_size);
    lv_obj_set_pos(ui.dots[i],
      dots_start_x + static_cast<lv_coord_t>(i) * (dot_size + dot_gap), dots_y);
  }

  // Whatever height remains below the dots (down to the bottom inset) is the
  // entire budget for the 4-row keypad -- dividing it by the row count is
  // what actually guarantees no overflow, rather than hoping a fixed
  // reference size happens to fit.
  lv_coord_t keypad_top = dots_y + dot_size + section_gap;
  lv_coord_t keypad_bottom = sh - outer_inset;
  lv_coord_t keypad_h = keypad_bottom - keypad_top;
  if (keypad_h < 4 * 36) keypad_h = 4 * 36;  // defensive floor; not expected in practice
  lv_coord_t keypad_w = sw - outer_inset * 2;

  lv_coord_t key_gap = control_modal_scaled_px(12, short_side);
  if (key_gap < 6) key_gap = 6;
  lv_coord_t key_size_w = (keypad_w - key_gap * 2) / 3;
  lv_coord_t key_size_h = (keypad_h - key_gap * 3) / 4;
  lv_coord_t key_size = key_size_w < key_size_h ? key_size_w : key_size_h;
  lv_coord_t max_key_size = control_modal_scaled_px(96, short_side);
  if (max_key_size < 56) max_key_size = 56;
  if (key_size > max_key_size) key_size = max_key_size;
  if (key_size < 36) key_size = 36;

  lv_coord_t total_w = key_size * 3 + key_gap * 2;
  lv_coord_t total_h = key_size * 4 + key_gap * 3;
  lv_coord_t start_x = (sw - total_w) / 2;
  lv_coord_t start_y = keypad_top + (keypad_h - total_h) / 2;
  if (start_y < keypad_top) start_y = keypad_top;

  for (int i = 0; i < 10; i++) {
    int row = i < 9 ? i / 3 : 3;
    int col = i < 9 ? i % 3 : 1;  // 0 centers under the middle column
    lv_obj_t *key_btn = control_modal_create_round_button(
      ui.overlay, key_size, kScreenLockPinPadDigits[i], nullptr, DARK_BORDER, SECONDARY_GREY);
    ui.keys[i] = key_btn;
    lv_obj_set_pos(key_btn, start_x + col * (key_size + key_gap),
      start_y + row * (key_size + key_gap));
    lv_obj_add_event_cb(key_btn, screen_lock_pin_pad_key_cb, LV_EVENT_CLICKED,
      const_cast<char *>(kScreenLockPinPadDigits[i]));
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
