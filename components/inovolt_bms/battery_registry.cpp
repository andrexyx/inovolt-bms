#include "battery_registry.h"

#include <algorithm>
#include <cctype>
#include <set>

namespace esphome::inovolt_bms {

static bool valid_mac(const std::string &mac) {
  if (mac.size() != 17)
    return false;
  for (size_t index = 0; index < mac.size(); index++) {
    if ((index + 1) % 3 == 0) {
      if (mac[index] != ':')
        return false;
    } else if (!std::isxdigit(static_cast<unsigned char>(mac[index]))) {
      return false;
    }
  }
  return true;
}

void BatterySlot::configure(size_t index, const BatteryConfig &config) {
  this->configured_ = true;
  this->number_ = index + 1;
  this->config_ = config;
  this->telemetry_ = {};
}

void BatterySlot::clear() {
  this->configured_ = false;
  this->number_ = 0;
  this->config_ = {};
  this->telemetry_ = {};
}

BatterySummary BatterySlot::summary() const {
  BatterySummary result;
  const size_t count = std::min<size_t>(this->telemetry_.cell_count, TP_CELL_COUNT);
  if (count == 0)
    return result;

  auto begin = this->telemetry_.cells.begin();
  auto end = begin + count;
  auto minimum = std::min_element(begin, end);
  auto maximum = std::max_element(begin, end);
  result.minimum_cell_voltage = *minimum;
  result.maximum_cell_voltage = *maximum;
  result.cell_voltage_delta = *maximum - *minimum;
  result.minimum_cell_number = static_cast<size_t>(std::distance(begin, minimum)) + 1;
  result.maximum_cell_number = static_cast<size_t>(std::distance(begin, maximum)) + 1;
  return result;
}

std::string BatterySlot::device_key() const { return "inovolt_battery_" + std::to_string(this->number_); }

bool BatteryRegistry::configure(const std::vector<BatteryConfig> &configs, std::string &error) {
  if (configs.empty() || configs.size() > INOVOLT_MAX_BATTERIES) {
    error = "Select between one and six batteries";
    return false;
  }

  std::set<std::string> addresses;
  for (const auto &config : configs) {
    if (!valid_mac(config.mac) || !config.advertised_name.starts_with("TP_") || config.friendly_name.empty()) {
      error = "Invalid battery configuration";
      return false;
    }
    if (!addresses.insert(config.mac).second) {
      error = "A battery can only be selected once";
      return false;
    }
  }

  for (auto &slot : this->slots_)
    slot.clear();
  for (size_t index = 0; index < configs.size(); index++)
    this->slots_[index].configure(index, configs[index]);
  this->active_count_ = configs.size();
  return true;
}

BatterySlot *BatteryRegistry::active_slot(size_t index) {
  if (index >= this->active_count_)
    return nullptr;
  return &this->slots_[index];
}

const BatterySlot *BatteryRegistry::active_slot(size_t index) const {
  if (index >= this->active_count_)
    return nullptr;
  return &this->slots_[index];
}

}  // namespace esphome::inovolt_bms
