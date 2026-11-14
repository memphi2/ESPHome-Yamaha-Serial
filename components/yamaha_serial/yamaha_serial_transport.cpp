// SPDX-License-Identifier: GPL-3.0-only

#include "yamaha_serial.h"

#include "esphome/core/log.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <cstdio>

namespace esphome::yamaha_serial {

static const char *const TAG = "yamaha_serial";

void YamahaSerialComponent::queue_command_(QueuedCommand command) {
  if (is_coalescible_tag_(command.tag)) {
    // A write is a barrier: a read queued before it cannot verify that write.
    bool write_pending = false;
    for (auto it = this->queue_.rbegin(); it != this->queue_.rend(); ++it) {
      if (!is_coalescible_tag_(it->tag)) {
        write_pending = true;
        break;
      }
      if (it->frame_type == command.frame_type && it->payload == command.payload) {
        return;
      }
    }
    if (!write_pending && this->in_flight_.has_value() && this->in_flight_->command.frame_type == command.frame_type &&
        this->in_flight_->command.payload == command.payload)
      return;
  }

  if (this->queue_.size() >= MAX_QUEUE_LEN) {
    auto drop_it = this->queue_.end();
    for (auto it = this->queue_.begin(); it != this->queue_.end(); ++it) {
      if (is_coalescible_tag_(it->tag)) {
        drop_it = it;
        break;
      }
    }
    if (drop_it != this->queue_.end()) {
      this->queue_.erase(drop_it);
    } else {
      this->queue_.pop_front();
    }
    this->queue_drops_++;
    ESP_LOGW(TAG, "Command queue full, dropping oldest queued command");
  }

  this->queue_.push_back(std::move(command));
}

void YamahaSerialComponent::queue_simple_command_(FrameType type, const std::string &payload, const char *tag,
                                                  bool expect_response) {
  if (payload.empty() || payload.size() > BUFFER_MAX_LEN - 4) {
    return;
  }
  QueuedCommand command{};
  command.frame_type = type;
  command.payload = payload;
  command.tag = tag;
  command.expect_response = expect_response;
  command.timeout_ms = this->command_timeout_ms_;
  this->configure_command_policy_(command);
  this->queue_command_(std::move(command));
}

void YamahaSerialComponent::process_command_queue_() {
  const uint32_t now = millis();

  if (this->command_hold_until_ms_ != 0U) {
    if (static_cast<int32_t>(now - this->command_hold_until_ms_) < 0) {
      return;
    }
    this->command_hold_until_ms_ = 0U;
  }

  if (this->in_flight_.has_value()) {
    auto &in_flight = this->in_flight_.value();
    if (!in_flight.command.expect_response) {
      this->in_flight_.reset();
    } else if (now - in_flight.sent_at_ms > in_flight.command.timeout_ms ||
               (in_flight.early_retry_at_ms != 0U &&
                static_cast<int32_t>(now - in_flight.early_retry_at_ms) >= 0)) {
      if (now - this->last_tx_ms_ < this->command_spacing_ms_) {
        return;
      }
      if (in_flight.command.retries_left > 0) {
        in_flight.command.retries_left--;
        in_flight.attempts++;
        if (in_flight.early_retry_at_ms != 0U) {
          ESP_LOGD(TAG, "Receiver woke without replying to '%s', retry %u", in_flight.command.tag,
                   in_flight.attempts);
        } else {
          ESP_LOGW(TAG, "Command timeout for '%s', retry %u", in_flight.command.tag, in_flight.attempts);
        }
        in_flight.early_retry_at_ms = 0U;
        if (this->send_command_(in_flight.command)) {
          in_flight.sent_at_ms = now;
          in_flight.responses_at_send = this->responses_received_;
          this->last_tx_ms_ = now;
        }
        return;
      }
      this->timeouts_++;
      if (in_flight.command.silence_ok) {
        // Expected silence (the receiver ignores this command in its current state): report it,
        // but it is no evidence of a lost connection and no state is published.
        this->set_last_error_(YAMAHA_SERIAL_PSTR("No reply to '%s' (not applicable in the receiver's current state)"),
                              in_flight.command.tag);
        this->in_flight_.reset();
        return;
      }
      this->set_last_error_(YAMAHA_SERIAL_PSTR("Timeout waiting for response to '%s'"), in_flight.command.tag);
      const bool no_activity = in_flight.responses_at_send == this->responses_received_;
      this->in_flight_.reset();
      if (no_activity)
        this->mark_unavailable_();
    } else {
      return;
    }
  }

  if (this->queue_.empty()) {
    return;
  }
  if (now - this->last_tx_ms_ < this->command_spacing_ms_) {
    return;
  }

  QueuedCommand command = std::move(this->queue_.front());
  this->queue_.pop_front();

  if (!this->send_command_(command)) {
    this->set_last_error_(YAMAHA_SERIAL_PSTR("Failed to send command '%s'"), command.tag);
    return;
  }
  this->last_tx_ms_ = now;

  InFlightCommand in_flight{};
  in_flight.command = std::move(command);
  in_flight.sent_at_ms = now;
  in_flight.attempts = 1;
  in_flight.responses_at_send = this->responses_received_;
  this->in_flight_ = std::move(in_flight);
}

bool YamahaSerialComponent::send_command_(const QueuedCommand &command) {
  uint8_t packet[BUFFER_MAX_LEN];
  size_t packet_len = 0;
  const size_t expected_len = command.payload.size() + (command.frame_type == FrameType::DC4 ? 4 : 2);
  if (expected_len > sizeof(packet)) {
    this->set_last_error_(YAMAHA_SERIAL_PSTR("TX payload too long"));
    return false;
  }

  packet[packet_len++] = static_cast<uint8_t>(command.frame_type);
  for (const char ch : command.payload) {
    packet[packet_len++] = static_cast<uint8_t>(ch);
  }
  if (command.frame_type == FrameType::DC4) {
    const uint8_t checksum = checksum_8bit_(command.payload);
    packet[packet_len++] = nibble_to_hex_(checksum >> 4);
    packet[packet_len++] = nibble_to_hex_(checksum & 0x0F);
  }
  packet[packet_len++] = static_cast<uint8_t>(FrameType::ETX);

  this->write_array(packet, packet_len);
  if (command.power_on)
    this->command_hold_until_ms_ = millis() + this->power_on_delay_ms_;
  this->commands_sent_++;
  ESP_LOGD(TAG, "TX [%s]: type=0x%02X payload=%s", command.tag, static_cast<uint8_t>(command.frame_type),
           command.payload.c_str());
  return true;
}

void YamahaSerialComponent::check_frame_timeout_() {
  const uint32_t now = millis();
  if (this->in_frame_ && now - this->frame_started_ms_ > FRAME_READ_TIMEOUT_MS) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("UART frame timeout"), this->frame_buffer_);
    this->in_frame_ = false;
    this->frame_buffer_.clear();
  }
}

void YamahaSerialComponent::handle_wake_byte_(uint32_t now) {
  // Only the first attempt of a retryable power-on, with nothing received since it was sent.
  if (!this->in_flight_.has_value())
    return;
  auto &in_flight = this->in_flight_.value();
  if (!in_flight.command.power_on || in_flight.attempts != 1 || in_flight.command.retries_left == 0 ||
      in_flight.responses_at_send != this->responses_received_ || in_flight.early_retry_at_ms != 0U)
    return;
  in_flight.early_retry_at_ms = now + WAKE_RETRY_MS;
  if (in_flight.early_retry_at_ms == 0U)
    in_flight.early_retry_at_ms = 1U;
  if (this->command_hold_until_ms_ != 0U &&
      static_cast<int32_t>(this->command_hold_until_ms_ - in_flight.early_retry_at_ms) > 0)
    this->command_hold_until_ms_ = in_flight.early_retry_at_ms;
  ESP_LOGD(TAG, "Receiver RS-232 woke; retrying '%s' early", in_flight.command.tag);
}

void YamahaSerialComponent::process_incoming_byte_(uint8_t byte) {
  const uint32_t now = millis();
  this->check_frame_timeout_();

  if (is_frame_start_(byte)) {
    if (this->in_frame_ && !this->frame_buffer_.empty()) {
      this->record_parse_error_(YAMAHA_SERIAL_PSTR("UART frame interrupted by new frame start"), this->frame_buffer_);
    }
    this->in_frame_ = true;
    this->frame_started_ms_ = now;
    this->frame_buffer_.clear();
    this->frame_buffer_.push_back(byte);
    return;
  }

  if (!this->in_frame_) {
    if (byte == 0x00U)
      this->handle_wake_byte_(now);
    return;
  }

  if (this->frame_buffer_.size() >= BUFFER_MAX_LEN) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("UART frame overflow"), this->frame_buffer_);
    this->in_frame_ = false;
    this->frame_buffer_.clear();
    return;
  }
  this->frame_buffer_.push_back(byte);

  if (byte == static_cast<uint8_t>(FrameType::ETX)) {
    this->in_frame_ = false;
    this->parse_frame_(this->frame_buffer_);
    this->frame_buffer_.clear();
  }
}

void YamahaSerialComponent::parse_frame_(const std::vector<uint8_t> &frame) {
  if (frame.size() < 3) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("Frame shorter than minimum length"), frame);
    return;
  }

  const uint32_t parse_errors_before = this->parse_errors_;
  const uint32_t errors_before = this->error_generation_;
  switch (static_cast<FrameType>(frame[0])) {
    case FrameType::STX:
      this->parse_stx_frame_(frame);
      break;
    case FrameType::DC1:
      this->parse_dc1_frame_(frame);
      break;
    case FrameType::DC2:
      this->parse_dc2_frame_(frame);
      break;
    case FrameType::DC4:
      this->parse_dc4_frame_(frame);
      break;
    case FrameType::DC3:
      ESP_LOGW(TAG, "Receiver sent reset frame");
      break;
    default:
      this->record_parse_error_(YAMAHA_SERIAL_PSTR("Unknown frame type"), frame);
      break;
  }

  // A damaged frame is not a response and must not cancel timeout/retry handling.
  if (this->parse_errors_ != parse_errors_before) {
    return;
  }
  this->responses_received_++;
  this->last_response_ms_ = millis();
  this->has_response_ = true;
  this->available_ = true;
  if (this->availability_binary_sensor_ != nullptr) {
    this->availability_binary_sensor_->publish_state(true);
  }
  this->publish_connection_state_(frame[0] == static_cast<uint8_t>(FrameType::DC2) ? "configured" : "online");
  const bool matching = this->in_flight_.has_value() && this->response_matches_(this->in_flight_->command, frame);
  if (matching && this->error_generation_ == errors_before) {
    this->clear_last_error_();
  }
  if (matching) {
    this->in_flight_.reset();
  }
  if (!this->receiver_reported_) {
    this->receiver_reported_ = true;
    this->queue_bootstrap_();
    this->queue_poll_commands_();
    ESP_LOGI(TAG, "Receiver responded; startup sync queued");
  }
}
void YamahaSerialComponent::check_availability_() {
  if (!this->periodic_poll_enabled_ && this->receiver_reported_) {
    return;
  }

  const uint32_t now = millis();
  const uint32_t threshold = std::max<uint32_t>(
      15000U,
      std::max<uint32_t>(this->command_timeout_ms_ * (this->max_retries_ + 2U), this->get_update_interval() * 3U));

  if (!this->receiver_reported_)
    return;
  const bool available = this->has_response_ && (now - this->last_response_ms_ <= threshold);
  if (available == this->available_) {
    return;
  }
  if (!available) {
    this->mark_unavailable_();
  }
}

void YamahaSerialComponent::mark_unavailable_() {
  this->available_ = false;
  this->receiver_reported_ = false;
  if (this->availability_binary_sensor_ != nullptr)
    this->availability_binary_sensor_->publish_state(false);
  if (this->in_flight_.has_value() && is_coalescible_tag_(this->in_flight_->command.tag))
    this->in_flight_.reset();
  this->queue_.erase(std::remove_if(this->queue_.begin(), this->queue_.end(),
                                    [](const QueuedCommand &cmd) { return is_coalescible_tag_(cmd.tag); }),
                     this->queue_.end());
  this->publish_connection_state_("timeout");
  // The existing startup interval probes again; no new polling loop is introduced.
}

bool YamahaSerialComponent::is_coalescible_tag_(const char *tag) {
  return std::strncmp(tag, "poll_", 5) == 0 || std::strcmp(tag, "report_enable") == 0 ||
         std::strcmp(tag, "report_delay_realtime") == 0 || std::strcmp(tag, "ready") == 0 ||
         std::strcmp(tag, "probe_ready") == 0;
}

}  // namespace esphome::yamaha_serial
