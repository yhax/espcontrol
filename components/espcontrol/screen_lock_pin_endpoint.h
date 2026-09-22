#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

#include <cstddef>
#include <cstdint>

#ifdef USE_WEBSERVER
#include <cstring>
#include <string>

#include <esp_http_server.h>

#include "esphome/components/web_server_idf/web_server_idf.h"
#include "panel_config_http_context.h"
#include "screen_lock_pin.h"
#include "screen_lock_state.h"

namespace espcontrol::configuration {

// Local-only endpoint for the Screen Lock PIN. Unlike the settings the rest
// of the panel's setup page reads and writes, this never goes near the
// panel configuration document or a Home Assistant entity: only whether a
// PIN is currently set is ever readable, and the PIN itself is hashed
// before it reaches flash (see screen_lock_pin_store.h). That keeps a
// browser on the network -- or Home Assistant's own entity history -- from
// ever being able to read the PIN back out.
class ScreenLockPinHandler final : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(
      esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    const bool get = request->method() == HTTP_GET;
    const bool post = request->method() == HTTP_POST;
    if (!get && !post) return false;
    char url_buffer[
        esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    const esphome::StringRef url = request->url_to(url_buffer);
    if (get) return std::strcmp(url.c_str(), "/api/v1/screen_lock_pin/status") == 0;
    return std::strcmp(url.c_str(), "/api/v1/screen_lock_pin/set") == 0 ||
           std::strcmp(url.c_str(), "/api/v1/screen_lock_pin/clear") == 0;
  }

  void handleRequest(
      esphome::web_server_idf::AsyncWebServerRequest *request) override {
    const PanelConfigHttpContext &context = panel_config_http_context();
    httpd_req_t *raw_request = *request;
#ifdef USE_WEBSERVER_AUTH
    if (!request->authenticate(context.username, context.password)) {
      request->requestAuthentication();
      return;
    }
#else
    (void) context;
#endif

    char url_buffer[
        esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    const esphome::StringRef url = request->url_to(url_buffer);

    if (std::strcmp(url.c_str(), "/api/v1/screen_lock_pin/status") == 0) {
      const char *body =
          espcontrol::screen_lock_pin_is_set() ? "{\"is_set\":true}" : "{\"is_set\":false}";
      httpd_resp_set_type(raw_request, "application/json");
      httpd_resp_send(raw_request, body, HTTPD_RESP_USE_STRLEN);
      return;
    }

    if (std::strcmp(url.c_str(), "/api/v1/screen_lock_pin/clear") == 0) {
      espcontrol::screen_lock_pin_clear();
      // Without a PIN, Screen Lock falls back to its original unauthenticated
      // toggle, so there is nothing left worth keeping the panel locked for.
      screen_lock_set_enabled(false);
      httpd_resp_set_status(raw_request, "204 No Content");
      httpd_resp_send(raw_request, nullptr, 0);
      return;
    }

    // "/api/v1/screen_lock_pin/set": the new PIN travels as a query
    // parameter, same as the other small local endpoints in this codebase.
    std::string pin;
    char query[32];
    if (httpd_req_get_url_query_str(raw_request, query, sizeof(query)) == ESP_OK) {
      char value[16];
      if (httpd_query_key_value(query, "pin", value, sizeof(value)) == ESP_OK) {
        pin = value;
      }
    }
    if (!screen_lock_pin_format_valid(pin)) {
      httpd_resp_send_err(raw_request, HTTPD_400_BAD_REQUEST,
                          "PIN must be 4 digits, each between 1 and 9");
      return;
    }
    if (!espcontrol::screen_lock_pin_set(pin)) {
      httpd_resp_send_err(raw_request, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "PIN could not be saved");
      return;
    }
    httpd_resp_set_status(raw_request, "204 No Content");
    httpd_resp_send(raw_request, nullptr, 0);
  }
};

inline void register_screen_lock_pin_endpoint(
    esphome::web_server_idf::AsyncWebServer &server) {
  static bool registered = false;
  if (registered) return;
  server.addHandler(new ScreenLockPinHandler());
  registered = true;
}

}  // namespace espcontrol::configuration
#else
namespace espcontrol::configuration {
inline void register_screen_lock_pin_endpoint(...) {}
}  // namespace espcontrol::configuration
#endif
