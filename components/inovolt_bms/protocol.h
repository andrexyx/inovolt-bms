#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace esphome::inovolt_bms {

constexpr size_t TP_FRAME_SIZE = 20;
constexpr size_t TP_CELL_COUNT = 24;
constexpr size_t TP_TEMPERATURE_COUNT = 8;

enum class TpMessage : uint8_t {
  SOFTWARE = 0x81,
  MODEL = 0x82,
  STATUS = 0x83,
  CAPACITY = 0x84,
  SWITCHES = 0x85,
  TEMPERATURES = 0x87,
  CELLS_1_8 = 0x88,
  CELLS_9_16 = 0x89,
  CELLS_17_24 = 0x8A,
};

struct BatteryTelemetry {
  std::string software;
  std::string model;
  float state_of_charge{0.0f};
  float state_of_health{0.0f};
  float voltage{0.0f};
  float current{0.0f};
  float power{0.0f};
  float average_temperature{0.0f};
  float ambient_temperature{0.0f};
  float mosfet_temperature{0.0f};
  float nominal_capacity{0.0f};
  float remaining_capacity{0.0f};
  uint16_t cycles{0};
  uint16_t voltage_protection{0};
  uint16_t current_protection{0};
  uint16_t temperature_protection{0};
  uint16_t alarms{0};
  uint16_t switch_state{0};
  uint16_t balancing_mask{0};
  uint8_t cell_count{0};
  uint8_t temperature_count{0};
  std::array<float, TP_CELL_COUNT> cells{};
  std::array<float, TP_TEMPERATURE_COUNT> temperatures{};
};

std::array<uint8_t, 4> make_request(TpMessage message);
bool decode_frame(const uint8_t *frame, size_t length, BatteryTelemetry &telemetry);

}  // namespace esphome::inovolt_bms
