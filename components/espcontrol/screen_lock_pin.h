#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

#include <cstddef>
#include <string>

// Portable helpers shared by the screen lock PIN store, the on-device keypad,
// and the host test suite. Nothing here touches ESP-IDF, so it can be
// exercised without a device.

constexpr size_t SCREEN_LOCK_PIN_LENGTH = 4;

// The lock screen keypad only shows digits 1-9 arranged in a 3x3 grid, so a
// PIN never contains 0. Keeping that rule in one place lets the keypad, the
// settings page validation, and the stored-credential check all agree on
// what counts as a valid PIN.
inline bool screen_lock_pin_format_valid(const std::string &pin) {
  if (pin.size() != SCREEN_LOCK_PIN_LENGTH) return false;
  for (char c : pin) {
    if (c < '1' || c > '9') return false;
  }
  return true;
}
