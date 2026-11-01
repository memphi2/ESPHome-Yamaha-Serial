#pragma once
#include "yamaha_serial.h"
#include <cassert>

using namespace esphome;
class Receiver : public yamaha_serial::YamahaSerialComponent {
 public:
  using YamahaSerialComponent::FrameType;
  using YamahaSerialComponent::process_incoming_byte_;
  using YamahaSerialComponent::process_command_queue_;
  using YamahaSerialComponent::queue_probe_ready_;
  using YamahaSerialComponent::queue_poll_commands_;
  using YamahaSerialComponent::apply_profile_;
  using YamahaSerialComponent::receiver_reported_;
  using YamahaSerialComponent::available_;
  using YamahaSerialComponent::parse_errors_;
  using YamahaSerialComponent::responses_received_;
  using YamahaSerialComponent::last_response_ms_;
  using YamahaSerialComponent::last_error_;
  using YamahaSerialComponent::in_flight_;
  using YamahaSerialComponent::queue_;
  using YamahaSerialComponent::timeouts_;
  using YamahaSerialComponent::queue_drops_;
  using YamahaSerialComponent::parse_volume_text_db_;
  using YamahaSerialComponent::active_profile_;
  using YamahaSerialComponent::inputs_;
  using YamahaSerialComponent::queue_named_command_;
  using YamahaSerialComponent::normalize_token_;
  using YamahaSerialComponent::handle_detected_model_;
  using YamahaSerialComponent::queue_simple_command_;
  using YamahaSerialComponent::check_availability_;
  using YamahaSerialComponent::publish_diagnostics_;
  using YamahaSerialComponent::refresh_api_metadata_;
  using YamahaSerialComponent::parse_tuner_station_;
  using YamahaSerialComponent::frame_buffer_;
  using YamahaSerialComponent::command_hold_until_ms_;
  using YamahaSerialComponent::main_power_on_;
  using YamahaSerialComponent::zone2_power_on_;
  using YamahaSerialComponent::set_last_error_;
  using YamahaSerialComponent::record_parse_error_;
  using YamahaSerialComponent::receiver_model_;
  void feed(const std::string &s) { for (uint8_t byte : s) process_incoming_byte_(byte); }
};

inline std::string extended(const std::string &body) {
  unsigned sum = 0;
  for (unsigned char c : body) sum += c;
  char checksum[3];
  snprintf(checksum, sizeof(checksum), "%02X", sum & 255);
  return std::string(1, '\x14') + body + checksum + '\x03';
}

