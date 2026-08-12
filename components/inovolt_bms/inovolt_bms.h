#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/core/component.h"

#include "battery_registry.h"
#include "poll_scheduler.h"

#ifdef USE_ESP32

namespace esphome::inovolt_bms {

struct DiscoveredBattery {
  uint64_t address{0};
  std::string address_text;
  std::string name;
  int rssi{-127};
  uint32_t last_seen{0};
};

class InoVoltBmsComponent : public Component, public esp32_ble_tracker::ESPBTDeviceListener {
 public:
  InoVoltBmsComponent() : scheduler_(this->registry_) {}

  void setup() override;
  void loop() override;
  void dump_config() override;
  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;

  const std::array<DiscoveredBattery, 24> &discovered() const { return this->discovered_; }
  size_t discovered_count() const { return this->discovered_count_; }
  BatteryRegistry &registry() { return this->registry_; }

 protected:
  void run_command_(const PollCommand &command);

  BatteryRegistry registry_{};
  PollScheduler scheduler_;
  std::array<DiscoveredBattery, 24> discovered_{};
  size_t discovered_count_{0};
};

}  // namespace esphome::inovolt_bms

#endif
