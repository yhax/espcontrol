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
  for (lv_obj_t *key : ui.keys) {
    if (key) lv_obj_set_style_bg_color(key, lv_color_hex(SECONDARY_GREY), LV_PART_MAIN);
  }
}

// Holds the whole keypad in red for a beat so a wrong PIN is unmistakable,
// then clears the attempt so the panel is ready for another try.
inline void screen_lock_pin_pad_flash_wrong() {
  ScreenLockPinPadUi &ui = screen_lock_pin_pad_ui();
  ui.flashing = true;
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
  if (ui.buffer.size() >= SCREEN_LOCK_PIN_LENGTH) screen_lock_pin_pad_submit();
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
// There is no "Enter Pin" label or progress dots: on a small square panel,
// even a short label eats into the room four rows of keys need, so the
// keypad is the entire screen. Each key highlights in the panel's accent
// colour for as long as it's actually pressed -- LVGL's own pressed-state
// styling -- as the only feedback that a tap registered, instead of a
// separate indicator that has to fit somewhere too.
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

  // The whole screen, minus the outer inset, is the keypad's budget --
  // dividing it by the row/column count is what actually guarantees no
  // overflow, rather than hoping a fixed reference size happens to fit.
  lv_coord_t keypad_h = sh - outer_inset * 2;
  lv_coord_t keypad_w = sw - outer_inset * 2;

  lv_coord_t key_gap = control_modal_scaled_px(14, short_side);
  if (key_gap < 6) key_gap = 6;
  lv_coord_t key_size_w = (keypad_w - key_gap * 2) / 3;
  lv_coord_t key_size_h = (keypad_h - key_gap * 3) / 4;
  lv_coord_t key_size = key_size_w < key_size_h ? key_size_w : key_size_h;
  lv_coord_t max_key_size = control_modal_scaled_px(110, short_side);
  if (max_key_size < 64) max_key_size = 64;
  if (key_size > max_key_size) key_size = max_key_size;
  if (key_size < 36) key_size = 36;

  lv_coord_t total_w = key_size * 3 + key_gap * 2;
  lv_coord_t total_h = key_size * 4 + key_gap * 3;
  lv_coord_t start_x = (sw - total_w) / 2;
  lv_coord_t start_y = outer_inset + (keypad_h - total_h) / 2;
  if (start_y < outer_inset) start_y = outer_inset;

  uint32_t pressed_color = current_button_primary_color();
  for (int i = 0; i < 10; i++) {
    int row = i < 9 ? i / 3 : 3;
    int col = i < 9 ? i % 3 : 1;  // 0 centers under the middle column
    lv_obj_t *key_btn = control_modal_create_round_button(
      ui.overlay, key_size, kScreenLockPinPadDigits[i], nullptr, DARK_BORDER, SECONDARY_GREY);
    control_modal_apply_pressed_fill_color(key_btn, pressed_color);
    ui.keys[i] = key_btn;
    lv_obj_set_pos(key_btn, start_x + col * (key_size + key_gap),
      start_y + row * (key_size + key_gap));
    lv_obj_add_event_cb(key_btn, screen_lock_pin_pad_key_cb, LV_EVENT_CLICKED,
      const_cast<char *>(kScreenLockPinPadDigits[i]));
  }

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
