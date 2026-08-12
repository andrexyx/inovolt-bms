#include "protocol.h"

#include <algorithm>
#include <cmath>

namespace esphome::inovolt_bms {

static uint16_t read_u16(const uint8_t *data, size_t offset) {
  return static_cast<uint16_t>((static_cast<uint16_t>(data[offset]) << 8U) | data[offset + 1]);
}

static int16_t read_i16(const uint8_t *data, size_t offset) {
  return static_cast<int16_t>(read_u16(data, offset));
}

static std::string read_text(const uint8_t *data) {
  size_t length = 0;
  while (length < 16 && data[length] != 0)
    length++;
  return {reinterpret_cast<const char *>(data), length};
}

std::array<uint8_t, 4> make_request(TpMessage message) {
  return {0x55, 0x04, static_cast<uint8_t>(message), 0xAA};
}

bool decode_frame(const uint8_t *frame, size_t length, BatteryTelemetry &telemetry) {
  if (frame == nullptr || length != TP_FRAME_SIZE || frame[0] != 0x55 || frame[1] != 0x14 || frame[19] != 0xAA)
    return false;

  const auto message = static_cast<TpMessage>(frame[2]);
  switch (message) {
    case TpMessage::SOFTWARE:
      telemetry.software = read_text(frame + 3);
      break;
    case TpMessage::MODEL:
      telemetry.model = read_text(frame + 3);
      break;
    case TpMessage::STATUS:
      telemetry.state_of_charge = read_u16(frame, 3);
      telemetry.voltage = read_u16(frame, 5) * 0.01f;
      telemetry.average_temperature = read_i16(frame, 7) * 0.1f;
      telemetry.ambient_temperature = read_i16(frame, 9) * 0.1f;
      telemetry.mosfet_temperature = read_i16(frame, 11) * 0.1f;
      telemetry.current = read_i16(frame, 13) * 0.01f;
      telemetry.power = telemetry.voltage * telemetry.current;
      telemetry.state_of_health = read_u16(frame, 17);
      break;
    case TpMessage::CAPACITY:
      telemetry.cell_count = std::min<uint8_t>(frame[3], TP_CELL_COUNT);
      telemetry.temperature_count = std::min<uint8_t>(frame[4], TP_TEMPERATURE_COUNT);
      telemetry.nominal_capacity = read_u16(frame, 5) * 0.01f;
      telemetry.remaining_capacity = read_u16(frame, 7) * 0.01f;
      telemetry.cycles = read_u16(frame, 9);
      telemetry.voltage_protection = read_u16(frame, 11);
      telemetry.current_protection = read_u16(frame, 13);
      telemetry.temperature_protection = read_u16(frame, 15);
      telemetry.alarms = read_u16(frame, 17);
      break;
    case TpMessage::SWITCHES:
      telemetry.switch_state = read_u16(frame, 3);
      telemetry.balancing_mask = read_u16(frame, 13);
      break;
    case TpMessage::TEMPERATURES:
      for (size_t index = 0; index < TP_TEMPERATURE_COUNT; index++)
        telemetry.temperatures[index] = read_i16(frame, 3 + index * 2) * 0.1f;
      break;
    case TpMessage::CELLS_1_8:
    case TpMessage::CELLS_9_16:
    case TpMessage::CELLS_17_24: {
      const size_t chunk = static_cast<uint8_t>(message) - static_cast<uint8_t>(TpMessage::CELLS_1_8);
      for (size_t index = 0; index < 8; index++)
        telemetry.cells[chunk * 8 + index] = read_u16(frame, 3 + index * 2) * 0.001f;
      break;
    }
    default:
      return false;
  }
  return true;
}

}  // namespace esphome::inovolt_bms
