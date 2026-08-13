#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/device.h"
#include "esphome/core/component.h"
#include "../inovolt_portal/inovolt_portal.h"

#include "battery_registry.h"
#include "poll_scheduler.h"

#ifdef USE_ESP32

namespace esphome::inovolt_bms {

class InoVoltSensor : public sensor::Sensor {
 public:
  void set_runtime_name(const std::string &name) {
    this->runtime_name_ = name;
    this->name_ = StringRef(this->runtime_name_);
  }
  void set_runtime_visible(bool visible) { this->flags_.internal = !visible; }
 protected:
  std::string runtime_name_{};
};

template<class Base> class InoVoltNamedEntity : public Base {
 public:
  void set_runtime_name(const std::string &name) { this->runtime_name_ = name; this->name_ = StringRef(this->runtime_name_); }
  void set_runtime_visible(bool visible) { this->flags_.internal = !visible; }
 protected:
  std::string runtime_name_{};
};
using InoVoltBinarySensor = InoVoltNamedEntity<binary_sensor::BinarySensor>;
using InoVoltTextSensor = InoVoltNamedEntity<text_sensor::TextSensor>;

enum class Metric : uint8_t {
  SOC, VOLTAGE, CURRENT, POWER, CHARGE_POWER, DISCHARGE_POWER, HEALTH,
  AVG_TEMPERATURE, AMBIENT_TEMPERATURE, MOSFET_TEMPERATURE,
  NOMINAL_CAPACITY, REMAINING_CAPACITY, CYCLES,
  VOLTAGE_PROTECTION, CURRENT_PROTECTION, TEMPERATURE_PROTECTION, ALARMS,
  SWITCH_STATE, BALANCING_MASK, MIN_CELL_VOLTAGE, MAX_CELL_VOLTAGE,
  AVG_CELL_VOLTAGE, CELL_DELTA, MIN_CELL_NUMBER, MAX_CELL_NUMBER,
  TEMPERATURE_1, TEMPERATURE_2, TEMPERATURE_3, TEMPERATURE_4,
  TEMPERATURE_5, TEMPERATURE_6, TEMPERATURE_7, TEMPERATURE_8,
  CELL_1, CELL_2, CELL_3, CELL_4, CELL_5, CELL_6, CELL_7, CELL_8,
  CELL_9, CELL_10, CELL_11, CELL_12, CELL_13, CELL_14, CELL_15, CELL_16,
  CELL_17, CELL_18, CELL_19, CELL_20, CELL_21, CELL_22, CELL_23, CELL_24,
  COUNT
};

struct DiscoveredBattery {
  uint64_t address{0};
  std::string address_text;
  std::string name;
  int rssi{-127};
  uint32_t last_seen{0};
};

class InoVoltBmsComponent : public Component, public esp32_ble_tracker::ESPBTDeviceListener {
 public:
  InoVoltBmsComponent() = default;

  class SlotNode : public ble_client::BLEClientNode {
   public:
    void configure(InoVoltBmsComponent *owner, size_t slot) { this->owner_ = owner; this->slot_ = slot; }
    void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                             esp_ble_gattc_cb_param_t *param) override;
   protected:
    InoVoltBmsComponent *owner_{nullptr};
    size_t slot_{0};
  };

  void setup() override;
  void loop() override;
  void dump_config() override;
  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;
  // After BLE clients and portal preferences, immediately before the API server.
  float get_setup_priority() const override { return 201.0f; }
  void set_portal(inovolt_portal::InoVoltPortal *portal) { this->portal_ = portal; }
  void set_client(size_t slot, ble_client::BLEClient *client);
  void set_sensor(size_t slot, uint8_t metric, InoVoltSensor *sensor);
  void set_binary_sensor(size_t slot, uint8_t metric, InoVoltBinarySensor *sensor);
  void set_text_sensor(size_t slot, uint8_t metric, InoVoltTextSensor *sensor);
  void set_device(size_t slot, Device *device) { if (slot < INOVOLT_MAX_BATTERIES) this->devices_[slot] = device; }

  const std::array<DiscoveredBattery, 24> &discovered() const { return this->discovered_; }
  size_t discovered_count() const { return this->discovered_count_; }
  BatteryRegistry &registry() { return this->registry_; }
  size_t active_battery_count() const { return this->registry_.active_count(); }
  const BatteryTelemetry *battery_telemetry(size_t index) const {
    const auto *slot = this->registry_.active_slot(index);
    return slot == nullptr ? nullptr : &slot->telemetry();
  }

 protected:
  void connect_slot_(size_t slot);
  void send_request_(size_t slot);
  void handle_gatt_event_(size_t slot, esp_gattc_cb_event_t event, esp_ble_gattc_cb_param_t *param);
  void publish_slot_(size_t slot);
  static bool parse_mac_(const char *text, uint64_t &address);

  BatteryRegistry registry_{};
  static constexpr std::array<TpMessage, 9> MESSAGES_ = {
      TpMessage::SOFTWARE, TpMessage::MODEL, TpMessage::STATUS, TpMessage::CAPACITY,
      TpMessage::SWITCHES, TpMessage::TEMPERATURES, TpMessage::CELLS_1_8,
      TpMessage::CELLS_9_16, TpMessage::CELLS_17_24};
  std::array<size_t, INOVOLT_MAX_BATTERIES> message_index_{};
  std::array<uint32_t, INOVOLT_MAX_BATTERIES> next_action_{};
  std::array<bool, INOVOLT_MAX_BATTERIES> waiting_{};
  std::array<DiscoveredBattery, 24> discovered_{};
  size_t discovered_count_{0};
  inovolt_portal::InoVoltPortal *portal_{nullptr};
  std::array<ble_client::BLEClient *, INOVOLT_MAX_BATTERIES> clients_{};
  std::array<SlotNode, INOVOLT_MAX_BATTERIES> nodes_{};
  std::array<uint16_t, INOVOLT_MAX_BATTERIES> command_handles_{};
  std::array<uint16_t, INOVOLT_MAX_BATTERIES> notify_handles_{};
  std::array<std::array<InoVoltSensor *, static_cast<size_t>(Metric::COUNT)>, INOVOLT_MAX_BATTERIES> sensors_{};
  std::array<Device *, INOVOLT_MAX_BATTERIES> devices_{};
  std::array<std::array<InoVoltBinarySensor *, 21>, INOVOLT_MAX_BATTERIES> binary_sensors_{};
  std::array<std::array<InoVoltTextSensor *, 6>, INOVOLT_MAX_BATTERIES> text_sensors_{};
};

}  // namespace esphome::inovolt_bms

#endif
