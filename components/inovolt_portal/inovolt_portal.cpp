#include "inovolt_portal.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <set>

#include "esphome/components/json/json_util.h"
#include "esphome/components/wifi/wifi_component.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include "portal_data.h"

#ifdef USE_ESP32

namespace esphome::inovolt_portal {

static const char *const TAG = "inovolt_portal";
static constexpr uint32_t INOVOLT_PREFERENCE_KEY = 0x494E4F56;

std::string InoVoltPortal::json_escape(const std::string &value) {
  std::string escaped;
  escaped.reserve(value.size());
  static const char JSON_HEX[] = "0123456789abcdef";
  for (const unsigned char character : value) {
    switch (character) {
      case '"':
        escaped += "\\\"";
        break;
      case '\\':
        escaped += "\\\\";
        break;
      case '\b':
        escaped += "\\b";
        break;
      case '\f':
        escaped += "\\f";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        if (character < 0x20) {
          escaped += "\\u00";
          escaped += JSON_HEX[character >> 4];
          escaped += JSON_HEX[character & 0x0F];
        } else {
          escaped += static_cast<char>(character);
        }
    }
  }
  return escaped;
}

void InoVoltPortal::setup() {
  this->preference_ = global_preferences->make_preference<InoVoltStoredConfig>(INOVOLT_PREFERENCE_KEY, true);
  if (!this->preference_.load(&this->stored_) || this->stored_.version != 1 ||
      this->stored_.battery_count > INOVOLT_MAX_BATTERIES) {
    this->stored_ = {};
  }
  this->base_->add_handler_without_auth(this);
}

void InoVoltPortal::dump_config() {
  ESP_LOGCONFIG(TAG, "InoVolt provisioning portal:");
  ESP_LOGCONFIG(TAG, "  Stored batteries: %u", this->stored_.battery_count);
  ESP_LOGCONFIG(TAG, "  Local endpoint: /");
}

bool InoVoltPortal::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  const std::string &name = device.get_name();
  if (name.size() < 3 || std::toupper(static_cast<unsigned char>(name[0])) != 'T' ||
      std::toupper(static_cast<unsigned char>(name[1])) != 'P' || name[2] != '_')
    return false;

  const std::string mac = device.address_str();
  std::scoped_lock lock(this->discovered_mutex_);
  auto found = std::find_if(this->discovered_.begin(), this->discovered_.end(),
                            [&mac](const auto &item) { return item.mac == mac; });
  if (found == this->discovered_.end()) {
    if (this->discovered_.size() >= 24)
      this->discovered_.erase(this->discovered_.begin());
    this->discovered_.push_back({name, mac, device.get_rssi()});
  } else {
    found->name = name;
    found->rssi = device.get_rssi();
  }
  return true;
}

std::string InoVoltPortal::url(AsyncWebServerRequest *request) {
#ifdef USE_ESP32
  char buffer[AsyncWebServerRequest::URL_BUF_SIZE];
  return std::string(request->url_to(buffer));
#else
  return request->url();
#endif
}

bool InoVoltPortal::canHandle(AsyncWebServerRequest *request) const {
  const std::string url = url(request);
  if (request->method() == HTTP_POST)
    return url == "/api/config";
  if (request->method() != HTTP_GET)
    return false;
  if (url == "/" || url == "/index.html" || url == "/styles.css" || url == "/app.js" || url == "/core.mjs" ||
      url == "/api/wifi/scan" || url == "/api/bms/scan")
    return true;
  return wifi::global_wifi_component->is_ap_active();
}

void InoVoltPortal::send_asset_(AsyncWebServerRequest *request, const char *content_type, const char *content) {
  auto *response =
      request->beginResponse(200, content_type, reinterpret_cast<const uint8_t *>(content), strlen(content));
  response->addHeader("Cache-Control", "no-store");
  request->send(response);
}

void InoVoltPortal::send_wifi_scan_(AsyncWebServerRequest *request) {
  wifi::global_wifi_component->start_scanning();
  auto *stream = request->beginResponseStream("application/json");
  stream->print("[");
  bool first = true;
  wifi::ScanResultsLock lock(wifi::global_wifi_component);
  for (const auto &scan : wifi::global_wifi_component->get_scan_result()) {
    if (scan.get_is_hidden() || scan.get_ssid().empty())
      continue;
    if (!first)
      stream->print(",");
    first = false;
    stream->print(R"({"ssid":")");
    stream->print(json_escape(scan.get_ssid().str()));
    stream->printf(R"(","rssi":%d,"secure":%s})", scan.get_rssi(), scan.get_with_auth() ? "true" : "false");
  }
  stream->print("]");
  request->send(stream);
}

void InoVoltPortal::send_bms_scan_(AsyncWebServerRequest *request) {
  if (this->parent_ != nullptr)
    this->parent_->start_scan();
  auto *stream = request->beginResponseStream("application/json");
  stream->print("[");
  std::scoped_lock lock(this->discovered_mutex_);
  for (size_t i = 0; i < this->discovered_.size(); i++) {
    const auto &battery = this->discovered_[i];
    if (i != 0)
      stream->print(",");
    stream->print(R"({"name":")");
    stream->print(json_escape(battery.name));
    stream->print(R"(","mac":")");
    stream->print(battery.mac);
    stream->printf(R"(","rssi":%d})", battery.rssi);
  }
  stream->print("]");
  request->send(stream);
}

bool InoVoltPortal::is_valid_mac(const std::string &mac) {
  if (mac.size() != 17)
    return false;
  for (size_t i = 0; i < mac.size(); i++) {
    if ((i + 1) % 3 == 0) {
      if (mac[i] != ':')
        return false;
    } else if (!std::isxdigit(static_cast<unsigned char>(mac[i]))) {
      return false;
    }
  }
  return true;
}

bool InoVoltPortal::parse_and_store_config_(const std::string &body, std::string &error, std::string &ssid,
                                            std::string &password) {
  InoVoltStoredConfig candidate{};
  bool valid = json::parse_json(body, [&](JsonObject root) -> bool {
    ssid = root["wifi"]["ssid"] | "";
    password = root["wifi"]["password"] | "";
    JsonArray batteries = root["batteries"].as<JsonArray>();
    if (ssid.empty() || ssid.size() > 32 || password.size() > 64 || (!password.empty() && password.size() < 8)) {
      error = "Invalid Wi-Fi settings";
      return false;
    }
    if (batteries.size() < 1 || batteries.size() > INOVOLT_MAX_BATTERIES) {
      error = "Select between one and six batteries";
      return false;
    }

    std::set<std::string> macs;
    size_t index = 0;
    for (JsonObject battery : batteries) {
      std::string mac = battery["mac"] | "";
      std::string advertised_name = battery["advertised_name"] | "";
      std::string friendly_name = battery["friendly_name"] | "";
      if (!is_valid_mac(mac) || !advertised_name.starts_with("TP_") || advertised_name.size() >= 32 ||
          friendly_name.empty() || friendly_name.size() >= 32 || !macs.insert(mac).second) {
        error = "Invalid or duplicate battery";
        return false;
      }
      auto &target = candidate.batteries[index++];
      std::strncpy(target.mac, mac.c_str(), sizeof(target.mac) - 1);
      std::strncpy(target.advertised_name, advertised_name.c_str(), sizeof(target.advertised_name) - 1);
      std::strncpy(target.friendly_name, friendly_name.c_str(), sizeof(target.friendly_name) - 1);
    }
    candidate.battery_count = index;
    return true;
  });

  if (!valid)
    return false;
  this->stored_ = candidate;
  if (!this->preference_.save(&this->stored_)) {
    error = "Could not save battery configuration";
    return false;
  }
  global_preferences->sync();
  return true;
}

void InoVoltPortal::handle_config_save_(AsyncWebServerRequest *request) {
  std::string error;
  std::string ssid;
  std::string password;
  if (!this->parse_and_store_config_(this->request_body_, error, ssid, password)) {
    request->send(422, "application/json", (R"({"ok":false,"error":")" + json_escape(error) + R"("})").c_str());
    this->request_body_.clear();
    return;
  }

  request->send(200, "application/json", R"({"ok":true,"rebooting":true})");
  this->request_body_.clear();
  this->defer([ssid, password]() { wifi::global_wifi_component->save_wifi_sta(ssid, password); });
  this->set_timeout("inovolt-reboot", 1800, []() { App.safe_reboot(); });
}

void InoVoltPortal::handleBody(AsyncWebServerRequest *, uint8_t *data, size_t len, size_t index, size_t total) {
  if (total > 4096)
    return;
  if (index == 0) {
    this->request_body_.clear();
    this->request_body_.reserve(total);
  }
  this->request_body_.append(reinterpret_cast<const char *>(data), len);
}

void InoVoltPortal::handleRequest(AsyncWebServerRequest *request) {
  const std::string url = url(request);
  if (request->method() == HTTP_POST && url == "/api/config") {
    if (request->contentLength() > 4096) {
      request->send(422, "application/json", R"({"ok":false,"error":"Request too large"})");
      return;
    }
    this->handle_config_save_(request);
  } else if (url == "/styles.css") {
    this->send_asset_(request, INOVOLT_PORTAL_CSS_TYPE, INOVOLT_PORTAL_CSS);
  } else if (url == "/app.js") {
    this->send_asset_(request, INOVOLT_PORTAL_APP_TYPE, INOVOLT_PORTAL_APP);
  } else if (url == "/core.mjs") {
    this->send_asset_(request, INOVOLT_PORTAL_CORE_TYPE, INOVOLT_PORTAL_CORE);
  } else if (url == "/api/wifi/scan") {
    this->send_wifi_scan_(request);
  } else if (url == "/api/bms/scan") {
    this->send_bms_scan_(request);
  } else {
    this->send_asset_(request, INOVOLT_PORTAL_INDEX_TYPE, INOVOLT_PORTAL_INDEX);
  }
}

}  // namespace esphome::inovolt_portal

#endif
