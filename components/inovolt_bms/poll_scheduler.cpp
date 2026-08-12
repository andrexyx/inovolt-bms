#include "poll_scheduler.h"

namespace esphome::inovolt_bms {

PollCommand PollScheduler::start(uint32_t now) {
  if (this->registry_.active_count() == 0)
    return {};
  this->active_slot_ = 0;
  this->message_index_ = 0;
  this->running_ = true;
  this->connected_ = false;
  this->waiting_for_response_ = false;
  return this->connect_current_(now);
}

PollCommand PollScheduler::connect_current_(uint32_t now) {
  this->deadline_ = now + RESPONSE_TIMEOUT_MS;
  return {PollAction::CONNECT, this->active_slot_, TpMessage::STATUS};
}

PollCommand PollScheduler::on_connected(size_t slot, uint32_t now) {
  if (!this->running_ || slot != this->active_slot_)
    return {};
  this->connected_ = true;
  this->message_index_ = 0;
  this->waiting_for_response_ = true;
  this->deadline_ = now + RESPONSE_TIMEOUT_MS;
  return {PollAction::SEND_REQUEST, slot, MESSAGES[this->message_index_]};
}

PollCommand PollScheduler::on_frame(size_t slot, const uint8_t *frame, size_t length, uint32_t now) {
  if (!this->running_ || !this->connected_ || !this->waiting_for_response_ || slot != this->active_slot_)
    return {};
  if (frame == nullptr || length < 3 || frame[2] != static_cast<uint8_t>(MESSAGES[this->message_index_]))
    return {};
  auto *battery = this->registry_.active_slot(slot);
  if (battery == nullptr || !decode_frame(frame, length, battery->telemetry()))
    return {};

  this->message_index_++;
  if (this->message_index_ >= MESSAGES.size()) {
    this->waiting_for_response_ = false;
    return {PollAction::DISCONNECT, slot, TpMessage::STATUS};
  }
  this->deadline_ = now + RESPONSE_TIMEOUT_MS;
  return {PollAction::SEND_REQUEST, slot, MESSAGES[this->message_index_]};
}

PollCommand PollScheduler::on_disconnected(size_t slot, uint32_t now) {
  if (!this->running_ || slot != this->active_slot_)
    return {};
  this->connected_ = false;
  this->waiting_for_response_ = false;
  return this->advance_slot_(now);
}

PollCommand PollScheduler::advance_slot_(uint32_t now) {
  this->active_slot_ = (this->active_slot_ + 1) % this->registry_.active_count();
  this->message_index_ = 0;
  this->deadline_ = now + RECONNECT_DELAY_MS;
  return {};
}

PollCommand PollScheduler::tick(uint32_t now) {
  if (!this->running_)
    return {};
  if (static_cast<int32_t>(now - this->deadline_) < 0)
    return {};

  if (this->connected_) {
    this->waiting_for_response_ = false;
    return {PollAction::DISCONNECT, this->active_slot_, TpMessage::STATUS};
  }
  return this->connect_current_(now);
}

}  // namespace esphome::inovolt_bms
