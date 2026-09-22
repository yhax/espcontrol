#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

#include <cstdint>
#include <cstring>
#include <string>

#include "screen_lock_pin.h"

#ifdef USE_ESP32
#include <esp_random.h>
#include <mbedtls/sha256.h>
#include <nvs.h>
#endif

namespace espcontrol {

// On-device storage for the Screen Lock PIN. This is a local interaction
// guard for the touchscreen (see screen_lock_state.h) -- the PIN is never a
// Home Assistant entity and never appears in the panel configuration
// document that the settings page reads, so a browser on the network can
// never fetch it back out. Only "is a PIN currently set" is readable.
//
// The salt and hash live in their own NVS namespace, in the same NVS
// partition ESPHome already uses for its own state. A normal OTA firmware
// update only replaces the app partition, so this survives updates the same
// way WiFi credentials and other ESPHome-managed NVS state does.
//
// A 4-digit PIN is a small keyspace (10^4 = 10,000 combinations). Hashing it
// keeps a casual flash read from showing the PIN in the clear, but it is not
// a defense against a determined attacker with physical access to the
// device. Screen Lock remains a local interaction guard, not a replacement
// for Home Assistant's own security.
class ScreenLockPinStore {
 public:
  bool begin() {
#ifdef USE_ESP32
    if (ready_) return true;
    ready_ = nvs_open("espcontrol_lock", NVS_READWRITE, &handle_) == ESP_OK;
    return ready_;
#else
    return false;
#endif
  }

  bool is_set() {
#ifdef USE_ESP32
    if (!begin()) return false;
    uint8_t hash[HASH_SIZE];
    size_t size = sizeof(hash);
    return nvs_get_blob(handle_, kHashKey, hash, &size) == ESP_OK &&
           size == HASH_SIZE;
#else
    return false;
#endif
  }

  bool set_pin(const std::string &pin) {
#ifdef USE_ESP32
    if (!begin() || !screen_lock_pin_format_valid(pin)) return false;
    uint8_t salt[SALT_SIZE];
    esp_fill_random(salt, sizeof(salt));
    uint8_t hash[HASH_SIZE];
    if (!hash_pin(pin, salt, hash)) return false;
    if (nvs_set_blob(handle_, kSaltKey, salt, sizeof(salt)) != ESP_OK) {
      return false;
    }
    if (nvs_set_blob(handle_, kHashKey, hash, sizeof(hash)) != ESP_OK) {
      return false;
    }
    return nvs_commit(handle_) == ESP_OK;
#else
    (void) pin;
    return false;
#endif
  }

  bool verify(const std::string &pin) {
#ifdef USE_ESP32
    if (!begin() || !screen_lock_pin_format_valid(pin)) return false;
    uint8_t salt[SALT_SIZE];
    size_t salt_size = sizeof(salt);
    if (nvs_get_blob(handle_, kSaltKey, salt, &salt_size) != ESP_OK ||
        salt_size != SALT_SIZE) {
      return false;
    }
    uint8_t stored_hash[HASH_SIZE];
    size_t hash_size = sizeof(stored_hash);
    if (nvs_get_blob(handle_, kHashKey, stored_hash, &hash_size) != ESP_OK ||
        hash_size != HASH_SIZE) {
      return false;
    }
    uint8_t computed_hash[HASH_SIZE];
    if (!hash_pin(pin, salt, computed_hash)) return false;
    return std::memcmp(stored_hash, computed_hash, HASH_SIZE) == 0;
#else
    (void) pin;
    return false;
#endif
  }

  bool clear() {
#ifdef USE_ESP32
    if (!begin()) return false;
    nvs_erase_key(handle_, kSaltKey);
    nvs_erase_key(handle_, kHashKey);
    nvs_erase_key(handle_, kLockedKey);
    return nvs_commit(handle_) == ESP_OK;
#else
    return false;
#endif
  }

  // Whether the panel should come back locked after a reboot. Only
  // meaningful while a PIN is set -- without one, Screen Lock keeps its
  // existing unauthenticated toggle behaviour and does not persist.
  bool persisted_locked() {
#ifdef USE_ESP32
    if (!begin()) return false;
    uint8_t value = 0;
    return nvs_get_u8(handle_, kLockedKey, &value) == ESP_OK && value != 0;
#else
    return false;
#endif
  }

  void persist_locked(bool locked) {
#ifdef USE_ESP32
    if (!begin()) return;
    nvs_set_u8(handle_, kLockedKey, locked ? 1 : 0);
    nvs_commit(handle_);
#else
    (void) locked;
#endif
  }

 private:
#ifdef USE_ESP32
  static constexpr size_t SALT_SIZE = 16;
  static constexpr size_t HASH_SIZE = 32;
  static constexpr const char *kSaltKey = "salt";
  static constexpr const char *kHashKey = "hash";
  static constexpr const char *kLockedKey = "locked";

  static bool hash_pin(const std::string &pin, const uint8_t *salt,
                       uint8_t *out_hash) {
    uint8_t input[SALT_SIZE + SCREEN_LOCK_PIN_LENGTH];
    std::memcpy(input, salt, SALT_SIZE);
    std::memcpy(input + SALT_SIZE, pin.data(), pin.size());
    // mbedtls_sha256() is the one-shot form: no context to init/free, and its
    // signature has been stable across the mbedtls versions ESP-IDF vendors.
    return mbedtls_sha256(input, sizeof(input), out_hash, 0) == 0;
  }

  nvs_handle_t handle_{0};
#endif
  bool ready_{false};
};

inline ScreenLockPinStore &screen_lock_pin_store() {
  static ScreenLockPinStore store;
  return store;
}

inline bool screen_lock_pin_is_set() { return screen_lock_pin_store().is_set(); }
inline bool screen_lock_pin_set(const std::string &pin) {
  return screen_lock_pin_store().set_pin(pin);
}
inline bool screen_lock_pin_verify(const std::string &pin) {
  return screen_lock_pin_store().verify(pin);
}
inline bool screen_lock_pin_clear() { return screen_lock_pin_store().clear(); }
inline bool screen_lock_persisted_locked() {
  return screen_lock_pin_store().persisted_locked();
}
inline void screen_lock_persist_locked(bool locked) {
  screen_lock_pin_store().persist_locked(locked);
}

}  // namespace espcontrol
