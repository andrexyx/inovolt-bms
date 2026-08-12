#include <cassert>
#include <cmath>

#include "../components/inovolt_bms/protocol.h"
#include "../components/inovolt_bms/battery_registry.h"

using esphome::inovolt_bms::BatteryTelemetry;
using esphome::inovolt_bms::BatteryConfig;
using esphome::inovolt_bms::BatteryRegistry;
using esphome::inovolt_bms::TpMessage;
using esphome::inovolt_bms::decode_frame;
using esphome::inovolt_bms::make_request;

int main() {
  const auto request = make_request(TpMessage::STATUS);
  assert((request == std::array<uint8_t, 4>{0x55, 0x04, 0x83, 0xAA}));

  BatteryTelemetry telemetry;
  const uint8_t status[] = {0x55, 0x14, 0x83, 0x00, 0x3C, 0x14, 0x72, 0x01, 0x18, 0x00,
                            0xE6, 0x00, 0xF0, 0x00, 0x00, 0x30, 0x30, 0x00, 0x64, 0xAA};
  assert(decode_frame(status, sizeof(status), telemetry));
  assert(telemetry.state_of_charge == 60.0f);
  assert(std::fabs(telemetry.voltage - 52.34f) < 0.001f);
  assert(std::fabs(telemetry.average_temperature - 28.0f) < 0.001f);
  assert(telemetry.state_of_health == 100.0f);

  const uint8_t cells[] = {0x55, 0x14, 0x88, 0x0C, 0xC1, 0x0C, 0xD5, 0x0C, 0xD2, 0x0C,
                           0xD5, 0x0C, 0xC8, 0x0C, 0xD4, 0x0C, 0xC8, 0x0C, 0xD2, 0xAA};
  assert(decode_frame(cells, sizeof(cells), telemetry));
  assert(std::fabs(telemetry.cells[0] - 3.265f) < 0.001f);
  assert(std::fabs(telemetry.cells[7] - 3.282f) < 0.001f);

  uint8_t invalid[20]{};
  assert(!decode_frame(invalid, sizeof(invalid), telemetry));

  BatteryRegistry registry;
  std::string error;
  assert(registry.configure({
                                {"50:CF:14:00:00:01", "TP_DEMO001", "Battery 1"},
                                {"50:CF:14:00:00:02", "TP_DEMO002", "Battery 2"},
                            },
                            error));
  assert(registry.active_count() == 2);
  assert(registry.active_slot(0)->device_key() == "inovolt_battery_1");
  assert(registry.active_slot(1)->device_key() == "inovolt_battery_2");
  assert(registry.active_slot(2) == nullptr);

  registry.active_slot(0)->telemetry().cell_count = 3;
  registry.active_slot(0)->telemetry().cells[0] = 3.31f;
  registry.active_slot(0)->telemetry().cells[1] = 3.29f;
  registry.active_slot(0)->telemetry().cells[2] = 3.35f;
  const auto summary = registry.active_slot(0)->summary();
  assert(summary.minimum_cell_number == 2);
  assert(summary.maximum_cell_number == 3);
  assert(std::fabs(summary.cell_voltage_delta - 0.06f) < 0.001f);
  return 0;
}
