// SPDX-License-Identifier: GPL-3.0-only

#include "yamaha_serial.h"

#include "esphome/core/log.h"
#ifdef USE_API
#include "esphome/components/api/api_server.h"
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <cstdio>

namespace esphome::yamaha_serial {

static const char *const TAG = "yamaha_serial";

void YamahaSerialComponent::set_volume_config(float min_db, float max_db, float step_db) {
  if (!std::isfinite(min_db) || !std::isfinite(max_db) || !std::isfinite(step_db) || min_db < -80.0f ||
      max_db > 16.5f || min_db >= max_db || step_db < 0.5f || step_db > 10.0f) {
    this->set_last_error_(YAMAHA_SERIAL_PSTR("Invalid volume configuration"));
    return;
  }
  this->volume_min_db_ = min_db;
  this->volume_max_db_ = max_db;
  if (this->volume_min_db_ > this->volume_max_db_) {
    std::swap(this->volume_min_db_, this->volume_max_db_);
  }
  this->volume_step_db_ = std::max(step_db, 0.5f);
}

void YamahaSerialComponent::set_input_source_select(YamahaSelect *obj) {
  this->input_source_select_ = obj;
  this->configure_input_select_options_();
}

void YamahaSerialComponent::set_zone2_input_source_select(YamahaSelect *obj) {
  this->zone2_input_source_select_ = obj;
  this->configure_input_select_options_();
}

void YamahaSerialComponent::set_program_select(YamahaSelect *obj) {
  this->program_select_ = obj;
  this->configure_program_select_options_();
}

void YamahaSerialComponent::set_scene_select(YamahaSelect *obj) {
  this->scene_select_ = obj;
  this->configure_scene_select_options_();
}

void YamahaSerialComponent::add_input_mapping(const std::string &key, const std::string &label) {
  const std::string key_norm = normalize_token_(key);
  if (key_norm.empty()) {
    return;
  }

  if (!label.empty()) {
    bool updated_override = false;
    for (auto &override : this->input_overrides_) {
      if (override.key != key_norm) {
        continue;
      }
      override.label = label;
      updated_override = true;
      break;
    }
    if (!updated_override) {
      this->input_overrides_.push_back({key_norm, label});
    }
  }

  if (this->inputs_.empty()) {
    this->setup_default_mappings_();
  }

  for (auto &mapping : this->inputs_) {
    if (mapping.key != key_norm) {
      continue;
    }
    if (!label.empty()) {
      mapping.label = label;
    }
    this->configure_input_select_options_();
    return;
  }

  // Keep the override for model detection, but do not expose an unusable input.
  ESP_LOGW(TAG, "Input key '%s' is not in profile %s", key_norm.c_str(), this->active_profile_.c_str());
}

void YamahaSerialComponent::setup() {
  this->frame_buffer_.reserve(BUFFER_MAX_LEN);
  this->setup_default_mappings_();
  this->api_metadata_dirty_ = false;
  if (this->availability_binary_sensor_ != nullptr)
    this->availability_binary_sensor_->publish_state(false);
  this->publish_connection_state_("waiting_for_receiver");
  this->queue_probe_ready_();
}

void YamahaSerialComponent::update() {
  if (!this->receiver_reported_) {
    this->queue_probe_ready_();
    return;
  }
  if (!this->periodic_poll_enabled_) {
    return;
  }
  this->queue_poll_commands_();
}

void YamahaSerialComponent::loop() {
  for (size_t processed = 0; processed < BUFFER_MAX_LEN && this->available(); ++processed) {
    const int next = this->read();
    if (next < 0) {
      break;
    }
    this->process_incoming_byte_(static_cast<uint8_t>(next));
  }

  this->check_frame_timeout_();
  this->publish_diagnostics_();
  this->refresh_api_metadata_();
  this->check_availability_();
  this->process_command_queue_();
}

void YamahaSerialComponent::refresh_api_metadata_() {
  if (!this->api_metadata_dirty_)
    return;
  this->api_metadata_dirty_ = false;
#ifdef USE_API
  if (api::global_api_server == nullptr)
    return;
  // Select lists are discovery metadata, not state. Reconnect once after a
  // changed profile so HA re-discovers them without restarting the ESP.
  for (const auto &client : api::global_api_server->active_clients()) {
    if (!client->is_connection_setup())
      continue;
    if (!client->send_message(api::DisconnectRequest{}))
      client->on_fatal_error();
  }
#endif
}

void YamahaSerialComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Yamaha Serial");
  ESP_LOGCONFIG(TAG, "  Model: %s", this->configured_model_.c_str());
  ESP_LOGCONFIG(TAG, "  Receiver profile: %s (requested: %s)", this->active_profile_.c_str(),
                this->configured_profile_.c_str());
  ESP_LOGCONFIG(TAG, "  UART baud_rate: use 9600, 8N1, hardware handshaking when possible");
  ESP_LOGCONFIG(TAG, "  Periodic polling: %s", this->periodic_poll_enabled_ ? "enabled" : "disabled");
  if (this->periodic_poll_enabled_) {
    ESP_LOGCONFIG(TAG, "  Poll interval: %u ms", this->get_update_interval());
  }
  ESP_LOGCONFIG(TAG, "  Command timeout: %u ms", this->command_timeout_ms_);
  ESP_LOGCONFIG(TAG, "  Max retries: %u", this->max_retries_);
  ESP_LOGCONFIG(TAG, "  Command spacing: %u ms", this->command_spacing_ms_);
  ESP_LOGCONFIG(TAG, "  Power-on delay: %u ms", this->power_on_delay_ms_);
  ESP_LOGCONFIG(TAG, "  Diagnostics interval: %u ms", this->diagnostics_interval_ms_);
  ESP_LOGCONFIG(TAG, "  Raw commands: %s", this->allow_raw_commands_ ? "enabled" : "disabled");
  ESP_LOGCONFIG(TAG, "  Startup probe mode: poll waits until first receiver response");
  ESP_LOGCONFIG(TAG, "  Input mappings: %u", static_cast<unsigned>(this->inputs_.size()));
}
void YamahaSerialComponent::queue_bootstrap_() {
  this->queue_simple_command_(FrameType::DC1, "000", "ready");
  this->queue_simple_command_(FrameType::STX, "20000", "report_enable");
  this->queue_simple_command_(FrameType::STX, "20100", "report_delay_realtime");
}

void YamahaSerialComponent::queue_probe_ready_() {
  this->queue_simple_command_(FrameType::DC1, "000", "probe_ready", false);
}

void YamahaSerialComponent::queue_poll_commands_() {
  if (!this->receiver_reported_) {
    return;
  }
  this->queue_simple_command_(FrameType::STX, "20000", "poll_report_enable");
  this->queue_simple_command_(FrameType::STX, "22001", "poll_main_volume_text");
  this->queue_simple_command_(FrameType::STX, "22003", "poll_input_name_text");
  if (this->profile_caps_.supports_zone2) {
    this->queue_simple_command_(FrameType::STX, "22004", "poll_zone2_input_name_text");
    this->queue_simple_command_(FrameType::STX, "22002", "poll_zone2_volume_text");
  }
  this->queue_extended_poll_commands_();
}

void YamahaSerialComponent::queue_extended_poll_commands_() {
  if (!this->profile_caps_.supports_extended) {
    return;
  }
  this->queue_simple_command_(FrameType::DC4, "20050000F", "poll_model_name");
  this->queue_simple_command_(FrameType::DC4, "2006033000", "poll_bass");
  this->queue_simple_command_(FrameType::DC4, "2006033001", "poll_treble");
  this->queue_simple_command_(FrameType::DC4, "2006041000", "poll_center_distance");
  this->queue_simple_command_(FrameType::DC4, "2006041020", "poll_front_left_distance");
  this->queue_simple_command_(FrameType::DC4, "2006041030", "poll_front_right_distance");
  this->queue_simple_command_(FrameType::DC4, "2006041040", "poll_surround_left_distance");
  this->queue_simple_command_(FrameType::DC4, "2006041050", "poll_surround_right_distance");
  this->queue_simple_command_(FrameType::DC4, "20060410A0", "poll_subwoofer_distance");
  this->queue_simple_command_(FrameType::DC4, "2006050000", "poll_tuner_station");
}
void YamahaSerialComponent::record_parse_error_(const char *message) {
  this->parse_errors_++;
  this->last_parse_error_ = flash_string(message);
  if (this->last_parse_error_.size() > 255) {
    this->last_parse_error_.resize(255);
  }
  ESP_LOGW(TAG, "Parse error: %s", this->last_parse_error_.c_str());
  if (this->last_parse_error_text_sensor_ != nullptr) {
    this->last_parse_error_text_sensor_->publish_state(this->last_parse_error_);
  }
}

void YamahaSerialComponent::record_parse_error_(const char *message, const std::vector<uint8_t> &frame) {
  if (!frame.empty()) {
    // Stay below the logger's line buffer even for maximum-length DC2 frames.
    for (size_t offset = 0; offset < frame.size(); offset += 32) {
      char hex[32 * 3 + 1]{};
      const size_t count = std::min<size_t>(32, frame.size() - offset);
      for (size_t i = 0; i < count; ++i) {
        hex[3 * i] = nibble_to_hex_(frame[offset + i] >> 4);
        hex[3 * i + 1] = nibble_to_hex_(frame[offset + i] & 0x0F);
        hex[3 * i + 2] = ' ';
      }
      ESP_LOGW(TAG, "Rejected frame %u/%u: %s", static_cast<unsigned>(offset), static_cast<unsigned>(frame.size()),
               hex);
    }
  }
  this->record_parse_error_(message);
}

void YamahaSerialComponent::publish_connection_state_(const std::string &state) {
  if (this->connection_state_text_sensor_ != nullptr &&
      (!this->connection_state_text_sensor_->has_state() || this->connection_state_text_sensor_->state != state)) {
    this->connection_state_text_sensor_->publish_state(state);
  }
}

void YamahaSerialComponent::set_last_error_(const char *format, ...) {
  // The entity keeps at most 252 bytes plus "..."; the log gets the whole message. Most
  // messages fit the stack buffer. A longer one, usually echoing user input, is formatted
  // again on the heap, so the log keeps the input's tail.
  static constexpr size_t MAX_KEPT = 252;
  char buffer[MAX_KEPT + 2];
  va_list args;
  va_start(args, format);
  va_list again;
  va_copy(again, args);
  const int written = flash_vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  const char *message = buffer;
  size_t length = written < 0 ? 0 : std::min(static_cast<size_t>(written), sizeof(buffer) - 1);
  std::string overflow;
  if (written >= static_cast<int>(sizeof(buffer))) {
    overflow.assign(static_cast<size_t>(written), '\0');
    flash_vsnprintf(&overflow[0], overflow.size() + 1, format, again);
    message = overflow.c_str();
    length = overflow.size();
  }
  va_end(again);
  // A cut that would split a UTF-8 sequence moves back to the start of that character.
  size_t end = std::min(length, MAX_KEPT);
  while (end > 0 && end < length && (static_cast<uint8_t>(message[end]) & 0xC0) == 0x80) --end;
  this->error_generation_++;
  this->last_error_.assign(message, end);
  if (length > end) {
    this->last_error_ += "...";
  }
  ESP_LOGW(TAG, "%s", message);
  if (this->last_error_text_sensor_ != nullptr) {
    this->last_error_text_sensor_->publish_state(this->last_error_);
  }
}

void YamahaSerialComponent::clear_last_error_() {
  if (this->last_error_.empty()) {
    return;
  }
  this->last_error_.clear();
  if (this->last_error_text_sensor_ != nullptr) {
    this->last_error_text_sensor_->publish_state("");
  }
}

void YamahaSerialComponent::publish_diagnostics_() {
  const uint32_t now = millis();
  if (now - this->last_diagnostics_publish_ms_ < this->diagnostics_interval_ms_) {
    return;
  }
  this->last_diagnostics_publish_ms_ = now;

  if (this->last_response_age_sensor_ != nullptr) {
    const float age = this->has_response_ ? static_cast<float>(now - this->last_response_ms_) : NAN;
    this->last_response_age_sensor_->publish_state(age);
  }
  auto counter = [](sensor::Sensor *sensor, uint32_t value) {
    const float next = static_cast<float>(value);
    if (sensor != nullptr && (!sensor->has_state() || sensor->state != next))
      sensor->publish_state(next);
  };
  counter(this->commands_sent_sensor_, this->commands_sent_);
  counter(this->responses_received_sensor_, this->responses_received_);
  counter(this->parse_errors_sensor_, this->parse_errors_);
  counter(this->timeouts_sensor_, this->timeouts_);
  counter(this->queue_drops_sensor_, this->queue_drops_);
}
std::optional<size_t> YamahaSerialComponent::find_input_index_(const std::string &label_or_key) const {
  const std::string token = normalize_token_(label_or_key);
  if (token.empty()) {
    return std::nullopt;
  }

  for (size_t i = 0; i < this->inputs_.size(); i++) {
    if (this->inputs_[i].key == token || normalize_token_(this->inputs_[i].label) == token) {
      return i;
    }
  }

  // Alternative names for profile input keys, tried last to first.
  struct InputAlias {
    char token[12];
    char key[12];
  };
  static const InputAlias aliases[] YAMAHA_SERIAL_FLASH_TABLE = {
      {"md", "md_tape"},
      {"tape", "md_tape"},
      {"v_aux", "vaux"},
      {"vaux", "vaux"},
      {"vaux", "vaux_dock"},
      {"vaux_dock", "vaux_dock"},
      {"dock", "vaux_dock"},
      {"dtv", "dtv"},
      {"dtv_ld", "dtv_ld"},
      {"dtv_ld", "dtv"},
      {"dtv_ld", "dtv_cbl"},
      {"dtv_cbl", "dtv_cbl"},
      {"cbl", "cbl_sat"},
      {"satellite", "sat"},
      {"dvr", "dvr"},
      {"dvr", "vcr2_dvr"},
      {"vcr", "vcr"},
      {"vcr1", "vcr"},
      {"vcr2_dvr", "dvr_vcr2"},
      {"vcr2_dvr", "dvr"},
      {"dvr_vcr2", "dvr_vcr2"},
      {"dvr_vcr2", "dvr"},
      {"bd", "bd_hd_dvd"},
      {"bd_hd", "bd_hd_dvd"},
      {"bd_hd_dvd", "bd_hd_dvd"},
      {"hd_dvd", "bd_hd_dvd"},
      {"net", "net_usb"},
      {"usb", "net_usb"},
      {"net_usb", "net_usb"},
      {"xm", "xm"},
      {"multich", "multi_ch"},
      {"multi_ch", "multi_ch"},
      {"hdmi1", "dvd"},
      {"hdmi2", "dtv_ld"},
      {"hdmi2", "dtv"},
      {"hdmi2", "dtv_cbl"},
      {"hdmi3", "cbl_sat"},
      {"av1", "cd"},
      {"av2", "cdr"},
      {"av3", "dvd"},
  };
  for (auto it = std::rbegin(aliases); it != std::rend(aliases); ++it) {
    const InputAlias alias = flash_read(*it);
    if (token != alias.token) {
      continue;
    }
    for (size_t i = 0; i < this->inputs_.size(); i++) {
      if (this->inputs_[i].key == alias.key) {
        return i;
      }
    }
  }

  return std::nullopt;
}

std::optional<size_t> YamahaSerialComponent::find_program_index_(const std::string &label) const {
  const std::string token = normalize_token_(label);
  if (token.empty()) {
    return std::nullopt;
  }

  for (size_t i = 0; i < this->programs_.size(); i++) {
    if (normalize_token_(this->programs_[i].label) == token) {
      return i;
    }
  }
  return std::nullopt;
}

bool YamahaSerialComponent::is_frame_start_(uint8_t value) {
  return value == static_cast<uint8_t>(FrameType::STX) || value == static_cast<uint8_t>(FrameType::DC1) ||
         value == static_cast<uint8_t>(FrameType::DC2) || value == static_cast<uint8_t>(FrameType::DC3) ||
         value == static_cast<uint8_t>(FrameType::DC4);
}

int8_t YamahaSerialComponent::hex_nibble_(uint8_t value) {
  if (value >= '0' && value <= '9') {
    return static_cast<int8_t>(value - '0');
  }
  if (value >= 'A' && value <= 'F') {
    return static_cast<int8_t>(value - 'A' + 10);
  }
  if (value >= 'a' && value <= 'f') {
    return static_cast<int8_t>(value - 'a' + 10);
  }
  return -1;
}

uint8_t YamahaSerialComponent::nibble_to_hex_(uint8_t value) {
  value &= 0x0F;
  return value < 10 ? static_cast<uint8_t>('0' + value) : static_cast<uint8_t>('A' + (value - 10));
}

std::string YamahaSerialComponent::normalize_token_(const std::string &in) {
  std::string out{};
  out.reserve(in.size());
  for (char ch : in) {
    const unsigned char c = static_cast<unsigned char>(ch);
    if (std::isalnum(c) != 0) {
      out.push_back(static_cast<char>(std::tolower(c)));
      continue;
    }
    if (ch == '_' || ch == ' ' || ch == '-' || ch == '/' || ch == '.' || ch == ':') {
      if (out.empty() || out.back() == '_') {
        continue;
      }
      out.push_back('_');
    }
  }
  if (!out.empty() && out.back() == '_') {
    out.pop_back();
  }
  return out;
}

std::string YamahaSerialComponent::trim_(const std::string &in) {
  if (in.empty()) {
    return in;
  }
  size_t start = 0;
  while (start < in.size() && std::isspace(static_cast<unsigned char>(in[start])) != 0) {
    start++;
  }
  size_t end = in.size();
  while (end > start && std::isspace(static_cast<unsigned char>(in[end - 1])) != 0) {
    end--;
  }
  return in.substr(start, end - start);
}

std::string YamahaSerialComponent::uppercase_ascii_(const std::string &value) {
  std::string out{};
  out.reserve(value.size());
  for (unsigned char c : value) {
    out.push_back(static_cast<char>(std::toupper(c)));
  }
  return out;
}

bool YamahaSerialComponent::payload_is_printable_ascii_(const std::string &payload) {
  for (unsigned char c : payload) {
    if (c < 0x20 || c > 0x7E) {
      return false;
    }
  }
  return true;
}

std::string YamahaSerialComponent::normalize_model_name_(const std::string &model) {
  std::string value = trim_(model);
  if (value == "R0177")
    return "RX-Vx500";  // Shared by the RX-V1500 and RX-V2500 manuals.
  if (value == "R0210")
    return "RX-V1700";
  if (value == "R0212")
    return "RX-V2700";
  if (value == "R0226")
    return "RX-V1800";
  if (value == "R0227")
    return "HTR-6190";
  if (value == "R0225")
    return "RX-V3800";
  if (value == "R0240")
    return "RX-V1900";
  if (value.size() > 2 && std::isdigit(static_cast<unsigned char>(value[0])) != 0 &&
      std::isdigit(static_cast<unsigned char>(value[1])) != 0 && value[2] == 'R') {
    value = value.substr(2);
  }
  return value;
}

uint8_t YamahaSerialComponent::checksum_8bit_(std::string_view payload) {
  uint32_t sum = 0;
  for (const unsigned char c : payload) {
    sum += c;
  }
  return static_cast<uint8_t>(sum & 0xFFU);
}

std::optional<uint16_t> YamahaSerialComponent::parse_hex_(std::string_view value) {
  if (value.empty() || value.size() > 4)
    return std::nullopt;
  uint16_t result = 0;
  for (const char ch : value) {
    const int8_t nibble = hex_nibble_(ch);
    if (nibble < 0)
      return std::nullopt;
    result = (result << 4) | nibble;
  }
  return result;
}

bool YamahaSerialComponent::valid_extended_length_(std::string_view payload) {
  if (payload.size() < 8 || payload.substr(0, 2) != "20")
    return false;
  const auto encoded = parse_hex_(payload.substr(2, 2));
  return encoded.has_value() && payload.size() == 4U + (*encoded == 0 ? MAX_EXTENDED_DATA_LEN : *encoded);
}

std::optional<float> YamahaSerialComponent::parse_volume_text_db_(const std::string &text) {
  if (text.empty()) {
    return std::nullopt;
  }
  std::string raw = trim_(text);
  std::string lower = raw;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
  if (lower.find("oo") != std::string::npos) {
    return std::nullopt;
  }
  auto pos = lower.find("db");
  if (pos != std::string::npos) {
    raw = raw.substr(0, pos);
  }
  raw = trim_(raw);
  if (raw.empty()) {
    return std::nullopt;
  }
  char *end = nullptr;
  const float value = std::strtof(raw.c_str(), &end);
  if (end == raw.c_str() || *end != '\0' || !std::isfinite(value)) {
    return std::nullopt;
  }
  return value;
}

uint8_t YamahaSerialComponent::volume_db_to_raw_(float db) {
  const float raw = ((db + 80.0f) * 2.0f) + 0x27;
  const auto rounded = static_cast<int>(std::round(raw));
  return static_cast<uint8_t>(clamp(rounded, 0x27, 0xE8));
}

float YamahaSerialComponent::volume_raw_to_db_(uint8_t raw) {
  if (raw == 0x00) {
    return NAN;
  }
  return (static_cast<float>(raw) - static_cast<float>(0x27)) / 2.0f - 80.0f;
}

}  // namespace esphome::yamaha_serial
