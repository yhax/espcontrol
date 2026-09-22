#include <cstdlib>
#include <string>

#include "screen_lock_pin.h"

#define CHECK(condition) do { if (!(condition)) return EXIT_FAILURE; } while (false)

int main() {
  CHECK(screen_lock_pin_format_valid("1234"));
  CHECK(screen_lock_pin_format_valid("9999"));
  CHECK(screen_lock_pin_format_valid("1919"));
  // The keypad is a phone-style dial pad: 1-9 plus 0.
  CHECK(screen_lock_pin_format_valid("1230"));
  CHECK(screen_lock_pin_format_valid("0123"));
  CHECK(screen_lock_pin_format_valid("0000"));

  CHECK(!screen_lock_pin_format_valid(""));
  CHECK(!screen_lock_pin_format_valid("123"));
  CHECK(!screen_lock_pin_format_valid("12345"));
  CHECK(!screen_lock_pin_format_valid("12a4"));
  CHECK(!screen_lock_pin_format_valid("12-4"));
  return EXIT_SUCCESS;
}
