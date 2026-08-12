#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "battery_registry.h"

namespace esphome::inovolt_bms {

enum class PollAction : uint8_t {
  NONE,
  CONNECT,
  SEND_REQUEST,
  DISCONNECT,
};

struct PollCommand {
  PollAction action{PollAction::NONE};
  size_t slot{0};
  TpMessage message{TpMessage::STATUS};
};

class PollScheduler {
 public:
  explicit PollScheduler(BatteryRegistry &registry) : registry_(registry) {}

  PollCommand start(uint32_t now);
  PollCommand on_connected(size_t slot, uint32_t now);
  PollCommand on_frame(size_t slot, const uint8_t *frame, size_t length, uint32_t now);
  PollCommand on_disconnected(size_t slot, uint32_t now);
  PollCommand tick(uint32_t now);

  size_t active_slot() const { return this->active_slot_; }
  bool running() const { return this->running_; }

 private:
  static constexpr uint32_t RESPONSE_TIMEOUT_MS = 2500;
  static constexpr uint32_t RECONNECT_DELAY_MS = 1200;
  static constexpr std::array<TpMessage, 9> MESSAGES = {
      TpMessage::SOFTWARE,       TpMessage::MODEL,        TpMessage::STATUS,
      TpMessage::CAPACITY,       TpMessage::SWITCHES,     TpMessage::TEMPERATURES,
      TpMessage::CELLS_1_8,      TpMessage::CELLS_9_16,   TpMessage::CELLS_17_24,
  };

  PollCommand connect_current_(uint32_t now);
  PollCommand advance_slot_(uint32_t now);

  BatteryRegistry &registry_;
  size_t active_slot_{0};
  size_t message_index_{0};
  uint32_t deadline_{0};
  bool running_{false};
  bool connected_{false};
  bool waiting_for_response_{false};
};

}  // namespace esphome::inovolt_bms
