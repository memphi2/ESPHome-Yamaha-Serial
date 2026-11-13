// SPDX-License-Identifier: GPL-3.0-only

#include "yamaha_serial.h"
#include "yamaha_serial_catalog.h"

#include "esphome/core/log.h"

#ifdef ARDUINO_ARCH_ESP8266
#include <pgmspace.h>
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

#ifdef ARDUINO_ARCH_ESP8266
#define YAMAHA_SERIAL_FLASH_TABLE PROGMEM
#else
#define YAMAHA_SERIAL_FLASH_TABLE
#endif

static const char *const CMD_POWER_ON = "07E7E";
static const char *const CMD_POWER_OFF = "07E7F";
static const char *const CMD_MAIN_POWER_ON = "07E7E";
static const char *const CMD_MAIN_POWER_OFF = "07E7F";
static const char *const CMD_MUTE_ON = "07EA2";
static const char *const CMD_MUTE_OFF = "07EA3";
static const char *const CMD_ZONE2_POWER_ON = "07EBA";
static const char *const CMD_ZONE2_POWER_OFF = "07EBB";
static const char *const CMD_FAN_MODE_ON = "2B201";
static const char *const CMD_FAN_MODE_OFF = "2B200";
static const char *const CMD_PURE_DIRECT_ON = "07E80";
static const char *const CMD_PURE_DIRECT_OFF = "07E82";
static const char *const CMD_ZONE2_MUTE_ON = "07EA0";
static const char *const CMD_ZONE2_MUTE_OFF = "07EA1";
static const char *const CMD_SPEAKER_A_ON = "07EAB";
static const char *const CMD_SPEAKER_A_OFF = "07EAC";
static const char *const CMD_SPEAKER_B_ON = "07EAD";
static const char *const CMD_SPEAKER_B_OFF = "07EAE";
static const char *const CMD_TUNER_INPUT = "07A16";
static const char *const CMD_TUNER_PRESET_1 = "07AE5";
static const char *const CMD_TUNER_PRESET_2 = "07AE6";
static const char *const CMD_TUNER_PRESET_3 = "07AE7";
static const char *const CMD_TUNER_PRESET_4 = "07AE8";
static const char *const CMD_TUNER_PRESET_5 = "07AE9";
static const char *const CMD_TUNER_PRESET_6 = "07AEA";
static const char *const CMD_TUNER_PRESET_7 = "07AEB";
static const char *const CMD_TUNER_PRESET_8 = "07AEC";
static const char *const CMD_TUNER_AUTO_UP = "07EBE";
static const char *const CMD_TUNER_AUTO_DOWN = "07EBF";
struct LabelCommand {
  const char *key;
  const char *command;
};
static const char *lookup_command_(const LabelCommand *entries, size_t count, const std::string &key) {
  for (size_t i = 0; i < count; i++) {
#ifdef ARDUINO_ARCH_ESP8266
    LabelCommand entry;
    memcpy_P(&entry, &entries[i], sizeof(entry));
#else
    const auto &entry = entries[i];
#endif
    if (key == entry.key) {
      return entry.command;
    }
  }
  return nullptr;
}
#define YAMAHA_SERIAL_SELECT_COMMANDS
#include "select_catalog.generated.h"
#undef YAMAHA_SERIAL_SELECT_COMMANDS

void YamahaSerialComponent::command_power(bool on) {
  this->queue_simple_command_(FrameType::STX, on ? CMD_POWER_ON : CMD_POWER_OFF, on ? "power_on" : "power_off");
}

void YamahaSerialComponent::command_main_zone_power(bool on) {
  this->queue_simple_command_(FrameType::STX, on ? CMD_MAIN_POWER_ON : CMD_MAIN_POWER_OFF,
                              on ? "main_power_on" : "main_power_off");
}

void YamahaSerialComponent::command_mute(bool on) {
  this->queue_simple_command_(FrameType::STX, on ? CMD_MUTE_ON : CMD_MUTE_OFF, on ? "mute_on" : "mute_off");
}

void YamahaSerialComponent::command_zone2_power(bool on) {
  if (!this->profile_caps_.supports_zone2) {
    this->set_last_error_("Zone2 power control not supported by active profile");
    return;
  }
  this->queue_simple_command_(FrameType::STX, on ? CMD_ZONE2_POWER_ON : CMD_ZONE2_POWER_OFF,
                              on ? "zone2_power_on" : "zone2_power_off");
}

void YamahaSerialComponent::command_volume_absolute(float db, bool zone2) {
  if (!std::isfinite(db)) {
    this->set_last_error_("Volume must be finite");
    return;
  }
  if (zone2 && !this->profile_caps_.supports_zone2) {
    this->set_last_error_("Zone2 volume control not supported by active profile");
    return;
  }
  db = clamp(db, this->volume_min_db_, this->volume_max_db_);
  const uint8_t raw = volume_db_to_raw_(db);
  char payload[6]{0};
  std::snprintf(payload, sizeof(payload), zone2 ? "231%02X" : "230%02X", raw);
  this->queue_simple_command_(FrameType::STX, payload, zone2 ? "zone2_volume_set" : "volume_set");
}

void YamahaSerialComponent::command_volume_step(bool up, bool zone2) {
  float current = zone2 ? this->zone2_volume_db_ : this->main_volume_db_;
  const std::string_view prefix = zone2 ? "231" : "230";
  auto target = [&](const QueuedCommand &command) {
    if (command.frame_type == FrameType::STX && command.payload.size() == 5 &&
        std::string_view(command.payload).substr(0, 3) == prefix) {
      if (const auto raw = parse_hex_(std::string_view(command.payload).substr(3)); raw.has_value()) {
        current = volume_raw_to_db_(*raw);
      }
    }
  };
  if (this->in_flight_.has_value())
    target(this->in_flight_->command);
  for (const auto &command : this->queue_) target(command);
  if (!std::isfinite(current)) {
    this->set_last_error_("Cannot step volume before a receiver volume report");
    return;
  }
  this->command_volume_absolute(current + (up ? this->volume_step_db_ : -this->volume_step_db_), zone2);
}

void YamahaSerialComponent::command_input(const std::string &label_or_key, bool zone2) {
  if (zone2 && !this->profile_caps_.supports_zone2) {
    this->set_last_error_("Zone2 input control not supported by active profile");
    return;
  }
  const auto idx = this->find_input_index_(label_or_key);
  if (!idx.has_value()) {
    this->set_last_error_("Unknown input source: " + label_or_key);
    return;
  }

  const auto &mapping = this->inputs_[idx.value()];
  const auto &command = zone2 ? mapping.zone2_command : mapping.main_command;
  if (command.empty()) {
    this->set_last_error_("Input mapping has no Yamaha command: " + mapping.key);
    return;
  }
  this->queue_simple_command_(FrameType::STX, command, zone2 ? "zone2_input_select" : "input_select");
}

void YamahaSerialComponent::command_program(const std::string &label) {
  const auto idx = this->find_program_index_(label);
  if (!idx.has_value()) {
    this->set_last_error_("Unknown program: " + label);
    return;
  }
  const auto &program = this->programs_[idx.value()];
  if (program.command.empty()) {
    // Report-only program: decoded from receiver reports, no known select command.
    this->set_last_error_("Program has no select command for this profile: " + program.label);
    return;
  }
  this->queue_simple_command_(FrameType::STX, program.command, "program_select");
}

void YamahaSerialComponent::command_program_step(bool next) {
  if (this->programs_.empty()) {
    this->set_last_error_("No programs configured for active profile");
    return;
  }

  size_t current = 0;
  if (auto idx = this->find_program_index_(this->program_label_); idx.has_value()) {
    current = idx.value();
  } else {
    current = next ? this->programs_.size() - 1 : 0;
  }
  // Skip report-only programs (no select command) when stepping.
  const size_t count = this->programs_.size();
  size_t target = current;
  for (size_t step = 0; step < count; step++) {
    target = next ? (target + 1) % count : (target + count - 1) % count;
    if (!this->programs_[target].command.empty())
      break;
  }
  this->command_program(this->programs_[target].label);
}

void YamahaSerialComponent::command_media_player_shortcut(const std::string &value) {
  if (value.size() > MAX_RAW_INPUT_LEN) {
    this->set_last_error_("Media player command too long");
    return;
  }
  const size_t start = value.find_first_not_of(" \t\r\n");
  std::string shortcut = start == std::string::npos ? "" : value.substr(start);
  if (shortcut.empty()) {
    this->set_last_error_("Empty media player shortcut");
    return;
  }

  auto strip_prefix = [&shortcut](const std::string &prefix) -> bool {
    const std::string key = YamahaSerialComponent::normalize_token_(shortcut);
    const std::string prefix_key = YamahaSerialComponent::normalize_token_(prefix);
    if (key == prefix_key) {
      shortcut.clear();
      return true;
    }
    if (key.rfind(prefix_key + "_", 0) != 0) {
      return false;
    }
    shortcut = shortcut.substr(prefix.size());
    while (!shortcut.empty() &&
           (shortcut.front() == ':' || shortcut.front() == '/' || shortcut.front() == ' ' || shortcut.front() == '=')) {
      shortcut.erase(shortcut.begin());
    }
    return true;
  };

  if (strip_prefix("yamaha://input") || strip_prefix("yamaha:input") || strip_prefix("media-source://yamaha/input") ||
      strip_prefix("media_source://yamaha/input") || strip_prefix("yamaha://source") || strip_prefix("yamaha:source") ||
      strip_prefix("media-source://yamaha/source") || strip_prefix("media_source://yamaha/source") ||
      strip_prefix("media_player.select_source") || strip_prefix("select_source") || strip_prefix("input") ||
      strip_prefix("source")) {
    this->command_input(shortcut, false);
    return;
  }
  if (strip_prefix("yamaha://program") || strip_prefix("yamaha:program") ||
      strip_prefix("media-source://yamaha/program") || strip_prefix("media_source://yamaha/program") ||
      strip_prefix("yamaha://dsp") || strip_prefix("yamaha:dsp") || strip_prefix("media-source://yamaha/dsp") ||
      strip_prefix("media_source://yamaha/dsp") || strip_prefix("yamaha://sound_mode") ||
      strip_prefix("yamaha:sound_mode") || strip_prefix("media-source://yamaha/sound_mode") ||
      strip_prefix("media_source://yamaha/sound_mode") || strip_prefix("media_player.select_sound_mode") ||
      strip_prefix("select_sound_mode") || strip_prefix("sound_mode") || strip_prefix("sound mode") ||
      strip_prefix("program") || strip_prefix("dsp")) {
    this->command_program(shortcut);
    return;
  }
  if (strip_prefix("yamaha://scene") || strip_prefix("yamaha:scene") || strip_prefix("media-source://yamaha/scene") ||
      strip_prefix("media_source://yamaha/scene") || strip_prefix("yamaha://system_memory") ||
      strip_prefix("yamaha:system_memory") || strip_prefix("media-source://yamaha/system_memory") ||
      strip_prefix("media_source://yamaha/system_memory") || strip_prefix("scene") || strip_prefix("system_memory") ||
      strip_prefix("system memory")) {
    this->command_scene(shortcut);
    return;
  }
  if (strip_prefix("yamaha://raw") || strip_prefix("yamaha:raw") || strip_prefix("media-source://yamaha/raw") ||
      strip_prefix("media_source://yamaha/raw") || strip_prefix("raw")) {
    this->command_raw_text(shortcut);
    return;
  }

  if (this->find_input_index_(shortcut).has_value()) {
    this->command_input(shortcut, false);
    return;
  }
  if (this->find_program_index_(shortcut).has_value()) {
    this->command_program(shortcut);
    return;
  }

  this->set_last_error_("Unknown media player shortcut: " + value);
}

void YamahaSerialComponent::command_scene(const std::string &label) {
  if (!this->profile_caps_.supports_scene) {
    this->set_last_error_("Scene control not supported by active profile");
    return;
  }
  const std::string key = normalize_token_(label);
  for (const auto &scene : this->scenes_) {
    if (normalize_token_(scene.label) != key) {
      continue;
    }
    this->queue_simple_command_(FrameType::STX, scene.command, "scene_select");
    this->scene_label_ = trim_(label);
    if (this->scene_select_ != nullptr && this->scene_select_->has_option(this->scene_label_)) {
      this->scene_select_->publish_state(this->scene_label_);
    }
    return;
  }
  this->set_last_error_("Unknown scene: " + label);
}

void YamahaSerialComponent::command_audio_select(const std::string &label) {
  if (!this->profile_caps_.supports_audio_select) {
    this->set_last_error_("Audio select control not supported by active profile");
    return;
  }
  const std::string key = normalize_token_(label);
  const char *command = lookup_command_(AUDIO_SELECT_COMMANDS, std::size(AUDIO_SELECT_COMMANDS), key);
  if (command == nullptr) {
    this->set_last_error_("Unknown audio select mode: " + label);
    return;
  }
  this->queue_simple_command_(FrameType::STX, command, "audio_select_set");
}

void YamahaSerialComponent::command_night_mode(const std::string &label) {
  if (!this->profile_caps_.supports_night_mode) {
    this->set_last_error_("Night mode control not supported by active profile");
    return;
  }
  const std::string key = normalize_token_(label);
  const char *command = lookup_command_(NIGHT_MODE_COMMANDS, std::size(NIGHT_MODE_COMMANDS), key);
  if (command == nullptr) {
    this->set_last_error_("Unknown night mode: " + label);
    return;
  }
  this->queue_simple_command_(FrameType::STX, command, "night_mode_set");
}

void YamahaSerialComponent::command_fan_mode(bool on) {
  if (!this->profile_caps_.supports_fan_mode) {
    this->set_last_error_("Fan mode control not supported by active profile");
    return;
  }
  this->queue_simple_command_(FrameType::STX, on ? CMD_FAN_MODE_ON : CMD_FAN_MODE_OFF,
                              on ? "fan_mode_on" : "fan_mode_off");
}

void YamahaSerialComponent::command_pure_direct(bool on) {
  if (!this->profile_caps_.supports_pure_direct) {
    this->set_last_error_("Pure direct control not supported by active profile");
    return;
  }
  const std::string configured_model = normalize_token_(this->configured_model_);
  const std::string reported_model = normalize_token_(this->receiver_model_);
  const bool rx_v2500 =
      configured_model.find("v2500") != std::string::npos || reported_model.find("v2500") != std::string::npos;
  const char *command = rx_v2500 ? (on ? "07E82" : "07E83") : (on ? CMD_PURE_DIRECT_ON : CMD_PURE_DIRECT_OFF);
  this->queue_simple_command_(FrameType::STX, command, on ? "pure_direct_on" : "pure_direct_off");
}

void YamahaSerialComponent::command_zone2_mute(bool on) {
  if (!this->profile_caps_.supports_zone2) {
    this->set_last_error_("Zone2 mute control not supported by active profile");
    return;
  }
  this->queue_simple_command_(FrameType::STX, on ? CMD_ZONE2_MUTE_ON : CMD_ZONE2_MUTE_OFF,
                              on ? "zone2_mute_on" : "zone2_mute_off");
}

void YamahaSerialComponent::command_speaker_relay(char relay, bool on) {
  if (!this->profile_caps_.supports_speaker_relays) {
    this->set_last_error_("Speaker relay control not supported by active profile");
    return;
  }
  if (relay == 'A') {
    this->queue_simple_command_(FrameType::STX, on ? CMD_SPEAKER_A_ON : CMD_SPEAKER_A_OFF,
                                on ? "speaker_a_on" : "speaker_a_off");
    return;
  }
  if (relay == 'B') {
    this->queue_simple_command_(FrameType::STX, on ? CMD_SPEAKER_B_ON : CMD_SPEAKER_B_OFF,
                                on ? "speaker_b_on" : "speaker_b_off");
    return;
  }
  this->set_last_error_("Unsupported speaker relay");
}

void YamahaSerialComponent::command_tuner_preset_page(const std::string &label) {
  const std::string key = normalize_token_(label);
  const char *command = lookup_command_(TUNER_PRESET_PAGE_COMMANDS, std::size(TUNER_PRESET_PAGE_COMMANDS), key);
  if (command == nullptr) {
    this->set_last_error_("Unknown tuner preset page: " + label);
    return;
  }
  this->queue_simple_command_(FrameType::STX, CMD_TUNER_INPUT, "tuner_input");
  this->queue_simple_command_(FrameType::STX, command, "tuner_preset_page_set");
}

void YamahaSerialComponent::command_tuner_band(const std::string &label) {
  const std::string key = normalize_token_(label);
  const char *command = lookup_command_(TUNER_BAND_COMMANDS, std::size(TUNER_BAND_COMMANDS), key);
  if (command == nullptr) {
    this->set_last_error_("Unknown tuner band: " + label);
    return;
  }
  this->queue_simple_command_(FrameType::STX, CMD_TUNER_INPUT, "tuner_input");
  this->queue_simple_command_(FrameType::STX, command, "tuner_band_set");
}

void YamahaSerialComponent::command_sleep_timer(const std::string &label) {
  if (!this->profile_caps_.sleep_control) {
    this->set_last_error_("Sleep control disabled for active profile");
    return;
  }
  const std::string key = normalize_token_(label);
  const char *command = lookup_command_(SLEEP_TIMER_COMMANDS, std::size(SLEEP_TIMER_COMMANDS), key);
  if (command == nullptr) {
    this->set_last_error_("Unknown sleep timer value: " + label);
    return;
  }
  this->queue_simple_command_(FrameType::STX, command, "sleep_timer_set");
}

void YamahaSerialComponent::command_decoder_mode(const std::string &label) {
  if (!this->profile_caps_.supports_decoder_mode) {
    this->set_last_error_("Decoder mode not supported by active profile");
    return;
  }
  const std::string key = normalize_token_(label);
  const char *command = lookup_command_(DECODER_MODE_COMMANDS, std::size(DECODER_MODE_COMMANDS), key);
  if (command == nullptr) {
    this->set_last_error_("Unknown decoder mode: " + label);
    return;
  }
  this->queue_simple_command_(FrameType::STX, command, "decoder_mode_set");
}

void YamahaSerialComponent::command_extended_surround(const std::string &label) {
  const std::string key = normalize_token_(label);
  const char *command = lookup_command_(EXTENDED_SURROUND_COMMANDS, std::size(EXTENDED_SURROUND_COMMANDS), key);
  if (command == nullptr) {
    this->set_last_error_("Unknown extended surround mode: " + label);
    return;
  }
  this->queue_simple_command_(FrameType::STX, command, "extended_surround_set");
}

void YamahaSerialComponent::command_speaker_b_assignment(const std::string &label) {
  const std::string key = normalize_token_(label);
  const char *command = lookup_command_(SPEAKER_B_ASSIGNMENT_COMMANDS, std::size(SPEAKER_B_ASSIGNMENT_COMMANDS), key);
  if (command == nullptr) {
    this->set_last_error_("Unknown speaker B assignment: " + label);
    return;
  }
  this->queue_simple_command_(FrameType::STX, command, "speaker_b_assignment_set");
}

void YamahaSerialComponent::command_zone2_amp(const std::string &label) {
  if (!this->profile_caps_.supports_zone2) {
    this->set_last_error_("Zone2 amp control not supported by active profile");
    return;
  }
  const std::string key = normalize_token_(label);
  const char *command = lookup_command_(ZONE2_AMP_COMMANDS, std::size(ZONE2_AMP_COMMANDS), key);
  if (command == nullptr) {
    this->set_last_error_("Unknown Zone2 amp mode: " + label);
    return;
  }
  this->queue_simple_command_(FrameType::STX, command, "zone2_amp_set");
}

void YamahaSerialComponent::command_surround_decoder(const std::string &label) {
  if (!this->profile_caps_.surround_decoder) {
    this->set_last_error_("Surround decoder not supported by active profile");
    return;
  }
  const std::string key = normalize_token_(label);
  const char *command = lookup_command_(SURROUND_DECODER_COMMANDS, std::size(SURROUND_DECODER_COMMANDS), key);
  if (command == nullptr) {
    this->set_last_error_("Surround decoder has no select command: " + label);
    return;
  }
  // The receiver ignores decoder commands unless Surround Decode (report 0x2C) is active.
  // Always select it first: the last reported program may be stale or about to change by
  // a queued command, and re-selecting Surround Decode keeps the current decoder.
  for (const auto &program : this->programs_) {
    if (program.report_code == 0x2C && !program.command.empty()) {
      this->queue_simple_command_(FrameType::STX, program.command, "program_select");
      break;
    }
  }
  this->queue_simple_command_(FrameType::STX, command, "surround_decoder_set");
}

void YamahaSerialComponent::command_tuner_auto_seek(bool up) {
  this->queue_simple_command_(FrameType::STX, CMD_TUNER_INPUT, "tuner_input");
  this->queue_simple_command_(FrameType::STX, up ? CMD_TUNER_AUTO_UP : CMD_TUNER_AUTO_DOWN,
                              up ? "tuner_auto_up" : "tuner_auto_down");
}

void YamahaSerialComponent::command_dimmer_percent(float percent) {
  if (!std::isfinite(percent)) {
    this->set_last_error_("Invalid dimmer value");
    return;
  }
  if (!this->profile_caps_.supports_extended) {
    this->set_last_error_("Dimmer control requires extended profile support");
    return;
  }
  const auto clamped = static_cast<int>(std::lround(clamp(percent, 0.0f, 100.0f)));
  const uint8_t level = static_cast<uint8_t>(clamp(clamped / 25, 0, 4));
  char payload[6]{0};
  std::snprintf(payload, sizeof(payload), "2610%c", nibble_to_hex_(level));
  this->queue_simple_command_(FrameType::STX, payload, "dimmer_set");
}

void YamahaSerialComponent::command_tuner_preset(uint8_t preset) {
  if (preset < 1 || preset > 8) {
    this->set_last_error_("Unknown tuner preset");
    return;
  }
  preset = static_cast<uint8_t>(clamp(static_cast<int>(preset), 1, 8));
  static const std::array<const char *, 8> preset_commands{
      CMD_TUNER_PRESET_1, CMD_TUNER_PRESET_2, CMD_TUNER_PRESET_3, CMD_TUNER_PRESET_4,
      CMD_TUNER_PRESET_5, CMD_TUNER_PRESET_6, CMD_TUNER_PRESET_7, CMD_TUNER_PRESET_8,
  };
  this->queue_simple_command_(FrameType::STX, CMD_TUNER_INPUT, "tuner_input");
  this->queue_simple_command_(FrameType::STX, preset_commands[preset - 1], "tuner_preset_set");
}

void YamahaSerialComponent::command_tuner_frequency(float frequency, bool fm) {
  if (!std::isfinite(frequency)) {
    this->set_last_error_("Invalid tuner frequency");
    return;
  }
  if (!this->profile_caps_.supports_extended) {
    this->set_last_error_("Tuner frequency control requires extended profile support");
    return;
  }

  char freq_chars[7]{0};
  if (fm) {
    const float clamped = clamp(frequency, 76.0f, 108.0f);
    std::snprintf(freq_chars, sizeof(freq_chars), "%06.2f", static_cast<double>(clamped));
  } else {
    const auto clamped = static_cast<unsigned>(std::lround(clamp(frequency, 500.0f, 1800.0f)));
    std::snprintf(freq_chars, sizeof(freq_chars), "%06u", clamped);
  }

  char write_cmd[18]{0};
  // Command 050 writes tuner station. PAGE=0 and NUMBER=0 mean current station.
  std::snprintf(write_cmd, sizeof(write_cmd), "200D050100%c%s", fm ? '1' : '0', freq_chars);
  this->queue_simple_command_(FrameType::STX, CMD_TUNER_INPUT, "tuner_input");
  this->queue_simple_command_(FrameType::DC4, write_cmd, fm ? "tuner_fm_frequency_set" : "tuner_am_frequency_set");
  this->queue_simple_command_(FrameType::DC4, "2006050000", "poll_tuner_station");
}

void YamahaSerialComponent::command_bass_percent(float percent) {
  if (!std::isfinite(percent)) {
    this->set_last_error_("Invalid bass value");
    return;
  }
  if (!this->profile_caps_.supports_extended) {
    this->set_last_error_("Bass control requires extended profile support");
    return;
  }
  const auto clamped = static_cast<int>(std::lround(clamp(percent, 0.0f, 100.0f)));
  const uint8_t raw =
      static_cast<uint8_t>(clamp(static_cast<int>(std::lround(static_cast<float>(clamped) / 4.1666667f)), 0, 0x18));
  char write_cmd[14]{0};
  std::snprintf(write_cmd, sizeof(write_cmd), "20090331001%02X", raw);
  this->queue_simple_command_(FrameType::DC4, write_cmd, "bass_set");
  this->queue_simple_command_(FrameType::DC4, "2006033000", "poll_bass");
}

void YamahaSerialComponent::command_treble_percent(float percent) {
  if (!std::isfinite(percent)) {
    this->set_last_error_("Invalid treble value");
    return;
  }
  if (!this->profile_caps_.supports_extended) {
    this->set_last_error_("Treble control requires extended profile support");
    return;
  }
  const auto clamped = static_cast<int>(std::lround(clamp(percent, 0.0f, 100.0f)));
  const uint8_t raw =
      static_cast<uint8_t>(clamp(static_cast<int>(std::lround(static_cast<float>(clamped) / 4.1666667f)), 0, 0x18));
  char write_cmd[14]{0};
  std::snprintf(write_cmd, sizeof(write_cmd), "20090331011%02X", raw);
  this->queue_simple_command_(FrameType::DC4, write_cmd, "treble_set");
  this->queue_simple_command_(FrameType::DC4, "2006033001", "poll_treble");
}

void YamahaSerialComponent::command_speaker_distance(uint8_t channel_id, uint16_t distance_value) {
  if (!this->profile_caps_.supports_extended) {
    this->set_last_error_("Speaker distance control requires extended profile support");
    return;
  }
  static const std::array<std::pair<uint8_t, const char *>, 6> write_prefixes{
      std::pair<uint8_t, const char *>{0x0, "2009041100"}, std::pair<uint8_t, const char *>{0x2, "2009041120"},
      std::pair<uint8_t, const char *>{0x3, "2009041130"}, std::pair<uint8_t, const char *>{0x4, "2009041140"},
      std::pair<uint8_t, const char *>{0x5, "2009041150"}, std::pair<uint8_t, const char *>{0xA, "20090411A0"},
  };
  static const std::array<std::pair<uint8_t, const char *>, 6> read_prefixes{
      std::pair<uint8_t, const char *>{0x0, "2006041000"}, std::pair<uint8_t, const char *>{0x2, "2006041020"},
      std::pair<uint8_t, const char *>{0x3, "2006041030"}, std::pair<uint8_t, const char *>{0x4, "2006041040"},
      std::pair<uint8_t, const char *>{0x5, "2006041050"}, std::pair<uint8_t, const char *>{0xA, "20060410A0"},
  };

  const auto dist = static_cast<uint16_t>(clamp<int>(distance_value, 0x1E, 0x960));

  const char *write_prefix = nullptr;
  const char *read_prefix = nullptr;
  for (const auto &item : write_prefixes) {
    if (item.first == channel_id) {
      write_prefix = item.second;
      break;
    }
  }
  for (const auto &item : read_prefixes) {
    if (item.first == channel_id) {
      read_prefix = item.second;
      break;
    }
  }
  if (write_prefix == nullptr || read_prefix == nullptr) {
    this->set_last_error_("Unsupported speaker distance channel");
    return;
  }

  char write_cmd[16]{0};
  std::snprintf(write_cmd, sizeof(write_cmd), "%s%03X", write_prefix, dist);
  this->queue_simple_command_(FrameType::DC4, write_cmd, "distance_set");
  this->queue_simple_command_(FrameType::DC4, read_prefix, "poll_distance");
}

void YamahaSerialComponent::command_receiver_reset() {
  if (!this->profile_caps_.supports_extended) {
    this->set_last_error_("Receiver reset command requires extended profile support");
    return;
  }
  this->queue_simple_command_(FrameType::DC3, std::string(3, static_cast<char>(0x7F)), "receiver_reset", false);
}

void YamahaSerialComponent::command_refresh() {
  if (!this->receiver_reported_) {
    this->queue_probe_ready_();
    return;
  }
  this->queue_poll_commands_();
}

void YamahaSerialComponent::command_power_toggle() {
  if (this->main_power_on_) {
    this->command_power(false);
  } else {
    this->command_power(true);
  }
}

void YamahaSerialComponent::command_raw_text(const std::string &value) {
  if (!this->allow_raw_commands_) {
    this->set_last_error_("Raw commands disabled; set allow_raw_commands: true");
    return;
  }
  if (value.size() > MAX_RAW_INPUT_LEN) {
    this->set_last_error_("Raw Yamaha command too long");
    return;
  }
  const size_t start = value.find_first_not_of(" \t\r\n");
  std::string raw = start == std::string::npos ? "" : value.substr(start);
  if (raw.empty()) {
    this->set_last_error_("Raw Yamaha command is empty");
    return;
  }

  const auto separator = raw.find_first_of(": /");
  if (separator == std::string::npos) {
    if (this->queue_named_command_(trim_(raw))) {
      return;
    }
    this->set_last_error_("Raw Yamaha command requires a known alias or prefix: stx:, dc4:, dc1:, or dc3:reset");
    return;
  }

  const std::string prefix = normalize_token_(raw.substr(0, separator));
  std::string payload = raw.substr(separator + 1);
  while (!payload.empty() && (payload.front() == ':' || payload.front() == '/' || payload.front() == ' ')) {
    payload.erase(payload.begin());
  }

  if (prefix == "dc3" && (normalize_token_(payload) == "reset" || payload == "7F7F7F")) {
    this->command_receiver_reset();
    return;
  }
  if (prefix == "alias" || prefix == "command" || prefix == "cmd") {
    if (this->queue_named_command_(trim_(payload))) {
      return;
    }
    this->set_last_error_("Unknown Yamaha command alias: " + payload);
    return;
  }

  FrameType frame_type;
  if (prefix == "stx" || prefix == "standard" || prefix == "std") {
    frame_type = FrameType::STX;
  } else if (prefix == "dc4" || prefix == "extended" || prefix == "ext") {
    frame_type = FrameType::DC4;
  } else if (prefix == "dc1" || prefix == "ready") {
    frame_type = FrameType::DC1;
  } else {
    this->set_last_error_("Unsupported raw Yamaha command prefix: " + prefix);
    return;
  }

  if (frame_type != FrameType::DC4) {
    payload.erase(std::remove_if(payload.begin(), payload.end(), [](unsigned char ch) { return std::isspace(ch); }),
                  payload.end());
    payload = uppercase_ascii_(payload);
  }
  if (payload.empty() || payload.size() > MAX_EXTENDED_DATA_LEN + 4 || !payload_is_printable_ascii_(payload)) {
    this->set_last_error_("Invalid raw Yamaha command payload");
    return;
  }
  if (frame_type == FrameType::DC4 && !valid_extended_length_(payload)) {
    this->set_last_error_("Extended raw Yamaha command must start with 20 and an exact length");
    return;
  }

  this->queue_simple_command_(frame_type, payload, "raw");
}

bool YamahaSerialComponent::queue_named_command_(const std::string &alias) {
  const std::string key = normalize_token_(alias);
  if (key == "receiver_reset" || key == "reset") {
    this->command_receiver_reset();
    return true;
  }
  NamedCommand command;
  if (!find_named_command(key, command))
    return false;
  this->queue_simple_command_(static_cast<FrameType>(command.frame_type), command.payload, "alias",
                              command.expect_response);
  return true;
}

}  // namespace esphome::yamaha_serial
