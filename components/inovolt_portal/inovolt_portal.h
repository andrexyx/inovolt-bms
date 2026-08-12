#pragma once

#include <array>
#include <mutex>
#include <string>
#include <vector>

#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/web_server_base/web_server_base.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

namespace esphome::inovolt_portal {

static constexpr size_t INOVOLT_MAX_BATTERIES = 6;

struct InoVoltStoredBattery {
  char mac[18]{};
  char advertised_name[32]{};
  char friendly_name[32]{};
};

struct InoVoltStoredConfig {
  uint8_t version{1};
  uint8_t battery_count{0};
  std::array<InoVoltStoredBattery, INOVOLT_MAX_BATTERIES> batteries{};
};

struct InoVoltDiscoveredBattery {
  std::string name;
  std::string mac;
  int rssi;
};

class InoVoltPortal : public Component,
                      public esp32_ble_tracker::ESPBTDeviceListener,
                      public AsyncWebHandler {
 public:
  explicit InoVoltPortal(web_server_base::WebServerBase *base) : base_(base) {}

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::WIFI + 2.0f; }

  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;
  bool canHandle(AsyncWebServerRequest *request) const override;
  void handleRequest(AsyncWebServerRequest *request) override;
  void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) override;
  bool isRequestHandlerTrivial() const override { return false; }

 protected:
  void send_asset_(AsyncWebServerRequest *request, const char *content_type, const char *content);
  void send_wifi_scan_(AsyncWebServerRequest *request);
  void send_bms_scan_(AsyncWebServerRequest *request);
  void handle_config_save_(AsyncWebServerRequest *request);
  bool parse_and_store_config_(const std::string &body, std::string &error, std::string &ssid, std::string &password);
  static bool is_valid_mac_(const std::string &mac);
  static std::string json_escape_(const std::string &value);
  static std::string url_(AsyncWebServerRequest *request);

  web_server_base::WebServerBase *base_;
  ESPPreferenceObject preference_{};
  InoVoltStoredConfig stored_{};
  std::vector<InoVoltDiscoveredBattery> discovered_{};
  std::mutex discovered_mutex_{};
  std::string request_body_{};
};

}  // namespace esphome::inovolt_portal
