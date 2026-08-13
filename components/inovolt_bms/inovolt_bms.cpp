#include "inovolt_bms.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cmath>

#include <esp_gattc_api.h>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome::inovolt_bms {

static const char *const TAG = "inovolt_bms";
static constexpr uint16_t TP_SERVICE_UUID = 0xFF00;
static constexpr uint16_t TP_NOTIFY_UUID = 0xFF01;
static constexpr uint16_t TP_COMMAND_UUID = 0xFF02;

static bool is_tp_name(const std::string &name) {
  return name.size() >= 3 && std::toupper(static_cast<unsigned char>(name[0])) == 'T' &&
         std::toupper(static_cast<unsigned char>(name[1])) == 'P' && name[2] == '_';
}

void InoVoltBmsComponent::setup() {
  ESP_LOGI(TAG, "Starting clean InoVolt BMS runtime");
  for (auto &slot_sensors : this->sensors_)
    for (auto *sensor : slot_sensors)
      if (sensor != nullptr)
        sensor->set_runtime_visible(false);
  for (auto &slot_sensors : this->binary_sensors_)
    for (auto *sensor : slot_sensors) if (sensor != nullptr) sensor->set_runtime_visible(false);
  for (auto &slot_sensors : this->text_sensors_)
    for (auto *sensor : slot_sensors) if (sensor != nullptr) sensor->set_runtime_visible(false);
  std::vector<BatteryConfig> configs;
  if (this->portal_ != nullptr) {
    const auto &stored = this->portal_->stored_config();
    for (size_t index = 0; index < stored.battery_count && index < INOVOLT_MAX_BATTERIES; index++) {
      const auto &item = stored.batteries[index];
      uint64_t address = 0;
      if (!parse_mac_(item.mac, address)) {
        ESP_LOGW(TAG, "Ignoring invalid stored MAC in slot %u", static_cast<unsigned>(index + 1));
        continue;
      }
      configs.push_back({item.mac, item.advertised_name, item.friendly_name});
      if (this->devices_[index] != nullptr)
        this->devices_[index]->set_name(item.friendly_name);
      static const char *const LABELS[] = {
          "01 SOC", "02 Tensiune", "03 Curent", "04 Putere", "05 Putere incarcare", "06 Putere descarcare", "07 SOH",
          "08 Temperatura medie", "09 Temperatura ambientala", "10 Temperatura MOSFET", "11 Capacitate nominala",
          "12 Capacitate ramasa", "13 Cicluri", "20 Protectie tensiune", "21 Protectie curent", "22 Protectie temperatura",
          "23 Alarme", "24 Stare MOSFET", "90 Masca balansare", "14 Celula minima", "15 Celula maxima", "16 Medie celule",
          "17 Diferenta celule", "18 Numar celula minima", "19 Numar celula maxima",
          "30 Temperatura 1", "31 Temperatura 2", "32 Temperatura 3", "33 Temperatura 4", "34 Temperatura 5", "35 Temperatura 6",
          "36 Temperatura 7", "37 Temperatura 8", "40 Celula 1", "41 Celula 2", "42 Celula 3", "43 Celula 4", "44 Celula 5",
          "45 Celula 6", "46 Celula 7", "47 Celula 8", "48 Celula 9", "49 Celula 10", "50 Celula 11", "51 Celula 12", "52 Celula 13",
          "53 Celula 14", "54 Celula 15", "55 Celula 16", "56 Celula 17", "57 Celula 18", "58 Celula 19", "59 Celula 20", "60 Celula 21",
          "61 Celula 22", "62 Celula 23", "63 Celula 24"};
      for (size_t metric = 0; metric < static_cast<size_t>(Metric::COUNT); metric++)
        if (this->sensors_[index][metric] != nullptr) {
          this->sensors_[index][metric]->set_runtime_visible(true);
          this->sensors_[index][metric]->set_runtime_name(std::string(item.friendly_name) + " " + LABELS[metric]);
        }
      static const char *const BINARY_LABELS[] = {"00 Online", "25 Incarcare", "26 Descarcare", "27 Limitare curent", "90 Balansare",
          "91 Balansare celula 1", "92 Balansare celula 2", "93 Balansare celula 3", "94 Balansare celula 4", "95 Balansare celula 5",
          "96 Balansare celula 6", "97 Balansare celula 7", "98 Balansare celula 8", "99 Balansare celula 9", "99 Balansare celula 10",
          "99 Balansare celula 11", "99 Balansare celula 12", "99 Balansare celula 13", "99 Balansare celula 14", "99 Balansare celula 15",
          "99 Balansare celula 16"};
      for (size_t metric = 0; metric < 21; metric++) if (this->binary_sensors_[index][metric] != nullptr) {
        this->binary_sensors_[index][metric]->set_runtime_visible(true);
        this->binary_sensors_[index][metric]->set_runtime_name(std::string(item.friendly_name) + " " + BINARY_LABELS[metric]);
      }
      static const char *const TEXT_LABELS[] = {"Versiune software", "Model", "Protectie tensiune", "Protectie curent",
                                                "Protectie temperatura", "Erori"};
      for (size_t metric = 0; metric < 6; metric++) if (this->text_sensors_[index][metric] != nullptr) {
        this->text_sensors_[index][metric]->set_runtime_visible(true);
        this->text_sensors_[index][metric]->set_runtime_name(std::string(item.friendly_name) + " " + TEXT_LABELS[metric]);
      }
      if (this->clients_[index] != nullptr) {
        this->clients_[index]->set_address(address);
        this->clients_[index]->set_auto_connect(false);
        this->clients_[index]->set_enabled(false);
      }
    }
  }
  for (size_t index = configs.size(); index < INOVOLT_MAX_BATTERIES; index++)
    if (this->clients_[index] != nullptr)
      this->clients_[index]->set_enabled(false);
  std::string error;
  if (!configs.empty() && !this->registry_.configure(configs, error))
    ESP_LOGE(TAG, "Stored configuration rejected: %s", error.c_str());
  for (size_t slot = 0; slot < this->registry_.active_count(); slot++) {
    this->next_action_[slot] = millis() + static_cast<uint32_t>(slot * 350);
    this->connect_slot_(slot);
  }
}

void InoVoltBmsComponent::set_client(size_t slot, ble_client::BLEClient *client) {
  if (slot >= INOVOLT_MAX_BATTERIES || client == nullptr)
    return;
  this->clients_[slot] = client;
  this->nodes_[slot].configure(this, slot);
  client->register_ble_node(&this->nodes_[slot]);
}

void InoVoltBmsComponent::set_sensor(size_t slot, uint8_t metric, InoVoltSensor *sensor) {
  if (slot < INOVOLT_MAX_BATTERIES && metric < static_cast<uint8_t>(Metric::COUNT))
    this->sensors_[slot][metric] = sensor;
}

void InoVoltBmsComponent::set_binary_sensor(size_t slot, uint8_t metric, InoVoltBinarySensor *sensor) {
  if (slot < INOVOLT_MAX_BATTERIES && metric < 21) this->binary_sensors_[slot][metric] = sensor;
}

void InoVoltBmsComponent::set_text_sensor(size_t slot, uint8_t metric, InoVoltTextSensor *sensor) {
  if (slot < INOVOLT_MAX_BATTERIES && metric < 6) this->text_sensors_[slot][metric] = sensor;
}

void InoVoltBmsComponent::publish_slot_(size_t slot) {
  const auto *battery = this->registry_.active_slot(slot);
  if (battery == nullptr || !battery->telemetry().has_status)
    return;
  const auto &data = battery->telemetry();
  const auto summary = battery->summary();
  float cell_sum = 0.0f;
  size_t cell_values = 0;
  for (size_t index = 0; index < data.cell_count; index++)
    if (data.cells[index] > 0.0f) { cell_sum += data.cells[index]; cell_values++; }
  const float values[] = {
      data.state_of_charge, data.voltage, data.current, data.power, std::max(0.0f, data.power),
      std::abs(std::min(0.0f, data.power)), data.state_of_health, data.average_temperature,
      data.ambient_temperature, data.mosfet_temperature, data.nominal_capacity, data.remaining_capacity,
      static_cast<float>(data.cycles), static_cast<float>(data.voltage_protection),
      static_cast<float>(data.current_protection), static_cast<float>(data.temperature_protection),
      static_cast<float>(data.alarms), static_cast<float>(data.switch_state), static_cast<float>(data.balancing_mask),
      summary.minimum_cell_voltage, summary.maximum_cell_voltage, cell_values ? cell_sum / cell_values : 0.0f,
      summary.cell_voltage_delta, static_cast<float>(summary.minimum_cell_number),
      static_cast<float>(summary.maximum_cell_number),
      data.temperatures[0], data.temperatures[1], data.temperatures[2], data.temperatures[3],
      data.temperatures[4], data.temperatures[5], data.temperatures[6], data.temperatures[7],
      data.cells[0], data.cells[1], data.cells[2], data.cells[3], data.cells[4], data.cells[5],
      data.cells[6], data.cells[7], data.cells[8], data.cells[9], data.cells[10], data.cells[11],
      data.cells[12], data.cells[13], data.cells[14], data.cells[15], data.cells[16], data.cells[17],
      data.cells[18], data.cells[19], data.cells[20], data.cells[21], data.cells[22], data.cells[23]};
  for (size_t metric = 0; metric < static_cast<size_t>(Metric::COUNT); metric++)
    if (this->sensors_[slot][metric] != nullptr)
      this->sensors_[slot][metric]->publish_state(values[metric]);
  const bool charging = (data.switch_state & 1U) != 0;
  const bool discharging = (data.switch_state & 2U) != 0;
  const bool limiting = (data.switch_state & 0x30U) != 0;
  const bool binary_values[] = {true, charging, discharging, limiting, data.balancing_mask != 0,
      (data.balancing_mask & 0x0001) != 0, (data.balancing_mask & 0x0002) != 0,
      (data.balancing_mask & 0x0004) != 0, (data.balancing_mask & 0x0008) != 0,
      (data.balancing_mask & 0x0010) != 0, (data.balancing_mask & 0x0020) != 0,
      (data.balancing_mask & 0x0040) != 0, (data.balancing_mask & 0x0080) != 0,
      (data.balancing_mask & 0x0100) != 0, (data.balancing_mask & 0x0200) != 0,
      (data.balancing_mask & 0x0400) != 0, (data.balancing_mask & 0x0800) != 0,
      (data.balancing_mask & 0x1000) != 0, (data.balancing_mask & 0x2000) != 0,
      (data.balancing_mask & 0x4000) != 0, (data.balancing_mask & 0x8000) != 0};
  for (size_t metric = 0; metric < 21; metric++) if (this->binary_sensors_[slot][metric] != nullptr)
    this->binary_sensors_[slot][metric]->publish_state(binary_values[metric]);
  const std::string text_values[] = {data.software, data.model, std::to_string(data.voltage_protection),
      std::to_string(data.current_protection), std::to_string(data.temperature_protection), std::to_string(data.alarms)};
  for (size_t metric = 0; metric < 6; metric++) if (this->text_sensors_[slot][metric] != nullptr)
    this->text_sensors_[slot][metric]->publish_state(text_values[metric]);
}

void InoVoltBmsComponent::SlotNode::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t,
                                                        esp_ble_gattc_cb_param_t *param) {
  if (this->owner_ != nullptr)
    this->owner_->handle_gatt_event_(this->slot_, event, param);
}

bool InoVoltBmsComponent::parse_mac_(const char *text, uint64_t &address) {
  unsigned int bytes[6];
  if (text == nullptr || std::sscanf(text, "%02x:%02x:%02x:%02x:%02x:%02x", &bytes[0], &bytes[1], &bytes[2],
                                    &bytes[3], &bytes[4], &bytes[5]) != 6)
    return false;
  address = 0;
  for (unsigned int byte : bytes) {
    if (byte > 0xFF)
      return false;
    address = (address << 8) | byte;
  }
  return address != 0;
}

void InoVoltBmsComponent::loop() {
  const uint32_t now = millis();
  for (size_t slot = 0; slot < this->registry_.active_count(); slot++) {
    auto *client = this->clients_[slot];
    if (client == nullptr) continue;
    if (!client->connected()) {
      if (static_cast<int32_t>(now - this->next_action_[slot]) >= 0) this->connect_slot_(slot);
    } else if (static_cast<int32_t>(now - this->next_action_[slot]) >= 0) {
      if (this->waiting_[slot]) this->waiting_[slot] = false;
      this->message_index_[slot] = 0;
      this->send_request_(slot);
    }
  }
}

void InoVoltBmsComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "InoVolt BMS:");
  ESP_LOGCONFIG(TAG, "  Configured batteries: %u", static_cast<unsigned>(this->registry_.active_count()));
  ESP_LOGCONFIG(TAG, "  Discovered TP devices: %u", static_cast<unsigned>(this->discovered_count_));
  ESP_LOGCONFIG(TAG, "  Transport: concurrent persistent BLE (up to 4 batteries)");
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

void InoVoltBmsComponent::connect_slot_(size_t slot) {
  if (slot >= this->registry_.active_count() || this->clients_[slot] == nullptr) return;
  this->clients_[slot]->set_enabled(true);
  this->clients_[slot]->set_auto_connect(true);
  this->next_action_[slot] = millis() + 10000;
}

void InoVoltBmsComponent::send_request_(size_t slot) {
    auto *client = this->clients_[slot];
    const uint16_t handle = this->command_handles_[slot];
    if (handle == 0 || !client->connected()) return;
    auto frame = make_request(MESSAGES_[this->message_index_[slot]]);
    const auto status = esp_ble_gattc_write_char(client->get_gattc_if(), client->get_conn_id(), handle, frame.size(),
                                                 frame.data(), ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
    if (status != ESP_OK)
      ESP_LOGW(TAG, "Write failed for slot %u: %d", static_cast<unsigned>(slot + 1), status);
    this->waiting_[slot] = true;
    this->next_action_[slot] = millis() + 2500;
}

void InoVoltBmsComponent::handle_gatt_event_(size_t slot, esp_gattc_cb_event_t event,
                                             esp_ble_gattc_cb_param_t *param) {
  if (slot >= INOVOLT_MAX_BATTERIES || this->clients_[slot] == nullptr)
    return;
  auto *client = this->clients_[slot];
  switch (event) {
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      auto *notify = client->get_characteristic(TP_SERVICE_UUID, TP_NOTIFY_UUID);
      auto *command = client->get_characteristic(TP_SERVICE_UUID, TP_COMMAND_UUID);
      if (notify == nullptr || command == nullptr) {
        ESP_LOGW(TAG, "Slot %u is not a compatible Tianpower BMS", static_cast<unsigned>(slot + 1));
        client->set_auto_connect(false);
        client->set_enabled(false);
        return;
      }
      this->notify_handles_[slot] = notify->handle;
      this->command_handles_[slot] = command->handle;
      const auto status = esp_ble_gattc_register_for_notify(client->get_gattc_if(), client->get_remote_bda(), notify->handle);
      if (status != ESP_OK)
        ESP_LOGW(TAG, "Notify registration failed for slot %u: %d", static_cast<unsigned>(slot + 1), status);
      break;
    }
    case ESP_GATTC_REG_FOR_NOTIFY_EVT:
      this->nodes_[slot].node_state = esp32_ble_tracker::ClientState::ESTABLISHED;
      this->message_index_[slot] = 0;
      this->send_request_(slot);
      break;
    case ESP_GATTC_NOTIFY_EVT:
      if (param->notify.handle == this->notify_handles_[slot]) {
        if (this->waiting_[slot] && param->notify.value_len >= 3 &&
            param->notify.value[2] == static_cast<uint8_t>(MESSAGES_[this->message_index_[slot]])) {
          auto *battery = this->registry_.active_slot(slot);
          if (battery != nullptr && decode_frame(param->notify.value, param->notify.value_len, battery->telemetry())) {
            this->publish_slot_(slot);
            this->waiting_[slot] = false;
            this->message_index_[slot]++;
            if (this->message_index_[slot] < MESSAGES_.size()) this->send_request_(slot);
            else { this->message_index_[slot] = 0; this->next_action_[slot] = millis() + 5000; }
          }
        }
      }
      break;
    case ESP_GATTC_DISCONNECT_EVT:
      this->command_handles_[slot] = 0;
      this->notify_handles_[slot] = 0;
      this->waiting_[slot] = false;
      this->nodes_[slot].node_state = esp32_ble_tracker::ClientState::IDLE;
      this->next_action_[slot] = millis() + 1500;
      break;
    default:
      break;
  }
}

}  // namespace esphome::inovolt_bms

#endif
