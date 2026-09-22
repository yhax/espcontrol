#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

#include <cstddef>
#include <string>

// Portable helpers shared by the screen lock PIN store, the on-device keypad,
// and the host test suite. Nothing here touches ESP-IDF, so it can be
// exercised without a device.

constexpr size_t SCREEN_LOCK_PIN_LENGTH = 4;

// The lock screen keypad is a phone-style dial pad: 1-9 in a 3x3 grid, with
// 0 centered underneath. Keeping the digit range in one place lets the
// keypad, the settings page validation, and the stored-credential check all
// agree on what counts as a valid PIN.
inline bool screen_lock_pin_format_valid(const std::string &pin) {
  if (pin.size() != SCREEN_LOCK_PIN_LENGTH) return false;
  for (char c : pin) {
    if (c < '0' || c > '9') return false;
  }
  return true;
}
