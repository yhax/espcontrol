#include <cstdlib>
#include <string>

#include "screen_lock_pin.h"

#define CHECK(condition) do { if (!(condition)) return EXIT_FAILURE; } while (false)

int main() {
  CHECK(screen_lock_pin_format_valid("1234"));
  CHECK(screen_lock_pin_format_valid("9999"));
  CHECK(screen_lock_pin_format_valid("1919"));

  CHECK(!screen_lock_pin_format_valid(""));
  CHECK(!screen_lock_pin_format_valid("123"));
  CHECK(!screen_lock_pin_format_valid("12345"));
  // The keypad is a 3x3 grid of the digits 1-9, so 0 is never enterable.
  CHECK(!screen_lock_pin_format_valid("1230"));
  CHECK(!screen_lock_pin_format_valid("0123"));
  CHECK(!screen_lock_pin_format_valid("12a4"));
  CHECK(!screen_lock_pin_format_valid("12-4"));
  return EXIT_SUCCESS;
}
