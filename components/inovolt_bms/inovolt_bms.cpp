#include "inovolt_bms.h"

#include <algorithm>
#include <cctype>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome::inovolt_bms {

static const char *const TAG = "inovolt_bms";

static bool is_tp_name(const std::string &name) {
  return name.size() >= 3 && std::toupper(static_cast<unsigned char>(name[0])) == 'T' &&
         std::toupper(static_cast<unsigned char>(name[1])) == 'P' && name[2] == '_';
}

void InoVoltBmsComponent::setup() {
  ESP_LOGI(TAG, "Starting clean InoVolt BMS runtime");
  const auto command = this->scheduler_.start(millis());
  this->run_command_(command);
}

void InoVoltBmsComponent::loop() { this->run_command_(this->scheduler_.tick(millis())); }

void InoVoltBmsComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "InoVolt BMS:");
  ESP_LOGCONFIG(TAG, "  Configured batteries: %u", static_cast<unsigned>(this->registry_.active_count()));
  ESP_LOGCONFIG(TAG, "  Discovered TP devices: %u", static_cast<unsigned>(this->discovered_count_));
  ESP_LOGCONFIG(TAG, "  Transport: sequential BLE");
}

bool InoVoltBmsComponent::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  const std::string &name = device.get_name();
  if (!is_tp_name(name))
    return false;

  const uint64_t address = device.address_uint64();
  auto begin = this->discovered_.begin();
  auto end = begin + this->discovered_count_;
  auto found = std::find_if(begin, end, [address](const DiscoveredBattery &item) { return item.address == address; });
  if (found == end) {
    if (this->discovered_count_ < this->discovered_.size()) {
      found = begin + this->discovered_count_;
      this->discovered_count_++;
    } else {
      found = std::min_element(begin, end, [](const DiscoveredBattery &left, const DiscoveredBattery &right) {
        return left.last_seen < right.last_seen;
      });
    }
  }
  found->address = address;
  found->address_text = device.address_str();
  found->name = name;
  found->rssi = device.get_rssi();
  found->last_seen = millis();
  return true;
}

void InoVoltBmsComponent::run_command_(const PollCommand &command) {
  if (command.action == PollAction::NONE)
    return;
  // The ESPHome GATT transport is attached in the next layer. Keeping the
  // scheduler independent makes protocol and six-slot behaviour host-testable.
  ESP_LOGV(TAG, "BLE action=%u slot=%u message=0x%02X", static_cast<unsigned>(command.action),
           static_cast<unsigned>(command.slot + 1), static_cast<unsigned>(command.message));
}

}  // namespace esphome::inovolt_bms

#endif
