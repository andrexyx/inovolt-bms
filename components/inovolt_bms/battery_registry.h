#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "protocol.h"

namespace esphome::inovolt_bms {

constexpr size_t INOVOLT_MAX_BATTERIES = 4;

struct BatteryConfig {
  std::string mac;
  std::string advertised_name;
  std::string friendly_name;
};

struct BatterySummary {
  float minimum_cell_voltage{0.0f};
  float maximum_cell_voltage{0.0f};
  float cell_voltage_delta{0.0f};
  size_t minimum_cell_number{0};
  size_t maximum_cell_number{0};
};

class BatterySlot {
 public:
  void configure(size_t index, const BatteryConfig &config);
  void clear();

  bool configured() const { return this->configured_; }
  size_t number() const { return this->number_; }
  const BatteryConfig &config() const { return this->config_; }
  BatteryTelemetry &telemetry() { return this->telemetry_; }
  const BatteryTelemetry &telemetry() const { return this->telemetry_; }
  BatterySummary summary() const;
  std::string device_key() const;

 private:
  bool configured_{false};
  size_t number_{0};
  BatteryConfig config_{};
  BatteryTelemetry telemetry_{};
};

class BatteryRegistry {
 public:
  bool configure(const std::vector<BatteryConfig> &configs, std::string &error);
  size_t active_count() const { return this->active_count_; }
  BatterySlot *active_slot(size_t index);
  const BatterySlot *active_slot(size_t index) const;

 private:
  std::array<BatterySlot, INOVOLT_MAX_BATTERIES> slots_{};
  size_t active_count_{0};
};

}  // namespace esphome::inovolt_bms
