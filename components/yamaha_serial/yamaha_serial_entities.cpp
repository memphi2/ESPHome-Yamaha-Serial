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

// Report tables live in flash (YAMAHA_SERIAL_FLASH_TABLE) with their labels inline.
struct ReportLabel {
  uint8_t report;
  char label[24];
};

struct SamplingRateInfo {
  uint8_t report;
  char label[20];
  float hz;
};

// Copies the entry for a report code out of a flash table; false if the code is unknown.
template<typename Entry, size_t N>
static bool lookup_report_(const Entry (&entries)[N], uint8_t report, Entry &result) {
  for (const Entry &flash_entry : entries) {
    const Entry entry = flash_read(flash_entry);
    if (entry.report == report) {
      result = entry;
      return true;
    }
  }
  return false;
}

// Sets `label` from a flash table; leaves it unchanged and returns false if the code is unknown.
template<size_t N> static bool lookup_label_(const ReportLabel (&entries)[N], uint8_t report, std::string &label) {
  ReportLabel entry;
  if (!lookup_report_(entries, report, entry)) {
    return false;
  }
  label = entry.label;
  return true;
}
#define YAMAHA_SERIAL_SELECT_REPORTS
#include "select_catalog.generated.h"
#undef YAMAHA_SERIAL_SELECT_REPORTS

static const ReportLabel PLAYBACK_FORMAT_REPORTS[] YAMAHA_SERIAL_FLASH_TABLE = {
    {0x0, "6CH Input"},
    {0x1, "Analog"},
    {0x2, "PCM"},
    {0x3, "Dolby Digital"},
    {0x4, "Dolby Digital 2.0"},
    {0x5, "Dolby Digital Karaoke"},
    {0x6, "Dolby Digital EX"},
    {0x7, "DTS"},
    {0x8, "DTS-ES"},
    {0x9, "Other Digital"},
};
static const SamplingRateInfo SAMPLING_RATE_REPORTS[] YAMAHA_SERIAL_FLASH_TABLE = {
    {0x0, "Analog", 0.0f},        {0x1, "32kHz", 32000.0f},     {0x2, "44.1kHz", 44100.0f},
    {0x3, "48kHz", 48000.0f},     {0x4, "64kHz", 64000.0f},     {0x5, "88.2kHz", 88200.0f},
    {0x6, "96kHz", 96000.0f},     {0x7, "Unknown", NAN},        {0x8, "128.0kHz", 128000.0f},
    {0x9, "176.4kHz", 176400.0f}, {0xA, "192.0kHz", 192000.0f}, {0xB, "48kHz (DTS 96/24)", 48000.0f},
};
// RX-Vx700 RS-232C spec, report 0x10 (Format) / 0x11 (Sampling); full RDAT byte.
static const ReportLabel PLAYBACK_FORMAT_REPORTS_X700[] YAMAHA_SERIAL_FLASH_TABLE = {
    {0x00, "Analog"},     {0x01, "PCM"},     {0x02, "DSD"},          {0x03, "Digital"},
    {0x04, "Dolby Digital"}, {0x05, "DTS"},  {0x06, "AAC"},          {0x07, "DTS-HD"},
    {0x08, "DTS-HD Master Audio"}, {0x09, "Dolby Digital Plus"}, {0x0A, "Dolby TrueHD"},
    {0x0B, "WMA"},        {0x0C, "MP3"},     {0xFE, "Unknown"},      {0xFF, "None"},
};
static const SamplingRateInfo SAMPLING_RATE_REPORTS_X700[] YAMAHA_SERIAL_FLASH_TABLE = {
    {0x00, "Analog", 0.0f},         {0x01, "8kHz", 8000.0f},       {0x02, "11.025kHz", 11025.0f},
    {0x03, "12kHz", 12000.0f},      {0x04, "16kHz", 16000.0f},     {0x05, "22.05kHz", 22050.0f},
    {0x06, "24kHz", 24000.0f},      {0x07, "32kHz", 32000.0f},     {0x08, "44.1kHz", 44100.0f},
    {0x09, "48kHz", 48000.0f},      {0x0A, "64kHz", 64000.0f},     {0x0B, "88.2kHz", 88200.0f},
    {0x0C, "96kHz", 96000.0f},      {0x0D, "128.0kHz", 128000.0f}, {0x0E, "176.4kHz", 176400.0f},
    {0x0F, "192.0kHz", 192000.0f},  {0x10, "2.8MHz (DSD)", 2822400.0f},
    {0xFE, "Unknown", NAN},         {0xFF, "None", NAN},
};
void YamahaSwitch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    return;
  }
  switch (this->function_) {
    case SwitchFunction::POWER:
      this->parent_->command_power(state);
      break;
    case SwitchFunction::MAIN_ZONE_POWER:
      this->parent_->command_main_zone_power(state);
      break;
    case SwitchFunction::MUTE:
      this->parent_->command_mute(state);
      break;
    case SwitchFunction::ZONE2_POWER:
      this->parent_->command_zone2_power(state);
      break;
    case SwitchFunction::PURE_DIRECT:
      this->parent_->command_pure_direct(state);
      break;
    case SwitchFunction::FAN_MODE:
      this->parent_->command_fan_mode(state);
      break;
    case SwitchFunction::ZONE2_MUTE:
      this->parent_->command_zone2_mute(state);
      break;
    case SwitchFunction::SPEAKER_A:
      this->parent_->command_speaker_relay('A', state);
      break;
    case SwitchFunction::SPEAKER_B:
      this->parent_->command_speaker_relay('B', state);
      break;
  }
}

void YamahaNumber::control(float value) {
  if (this->parent_ == nullptr || !std::isfinite(value)) {
    return;
  }
  const auto distance = static_cast<uint16_t>(std::lround(clamp(value, 30.0f, 2400.0f)));
  // The distance controls share one command and differ only in the speaker
  // index the receiver expects.
  switch (this->function_) {
    case NumberFunction::VOLUME:
      this->parent_->command_volume_absolute(value, false);
      break;
    case NumberFunction::ZONE2_VOLUME:
      this->parent_->command_volume_absolute(value, true);
      break;
    case NumberFunction::BASS:
      this->parent_->command_bass_percent(value);
      break;
    case NumberFunction::TREBLE:
      this->parent_->command_treble_percent(value);
      break;
    case NumberFunction::CENTER_DISTANCE:
      this->parent_->command_speaker_distance(0x0, distance);
      break;
    case NumberFunction::FRONT_LEFT_DISTANCE:
      this->parent_->command_speaker_distance(0x2, distance);
      break;
    case NumberFunction::FRONT_RIGHT_DISTANCE:
      this->parent_->command_speaker_distance(0x3, distance);
      break;
    case NumberFunction::SURROUND_LEFT_DISTANCE:
      this->parent_->command_speaker_distance(0x4, distance);
      break;
    case NumberFunction::SURROUND_RIGHT_DISTANCE:
      this->parent_->command_speaker_distance(0x5, distance);
      break;
    case NumberFunction::SUBWOOFER_DISTANCE:
      this->parent_->command_speaker_distance(0xA, distance);
      break;
    case NumberFunction::DIMMER:
      this->parent_->command_dimmer_percent(value);
      break;
    case NumberFunction::TUNER_FM_FREQUENCY:
      this->parent_->command_tuner_frequency(value, true);
      break;
    case NumberFunction::TUNER_AM_FREQUENCY:
      this->parent_->command_tuner_frequency(value, false);
      break;
  }
}

bool YamahaSelect::set_options_from_labels(const std::vector<std::string> &labels) {
  const auto &previous_options = this->traits.get_options();
  if (previous_options.size() == labels.size() &&
      std::equal(labels.begin(), labels.end(), previous_options.begin(),
                 [](const std::string &label, const char *previous) { return label == previous; }))
    return false;
  const std::string previous = this->current_option().c_str();
  this->set_has_state(false);
  this->stored_options_ = labels;
  FixedVector<const char *> options{};
  options.init(this->stored_options_.size());
  for (const auto &label : this->stored_options_) {
    options.push_back(label.c_str());
  }
  this->traits.set_options(options);
  if (!previous.empty() && this->has_option(previous))
    this->publish_state(previous);
  return true;
}

void YamahaSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    return;
  }
  switch (this->function_) {
    case SelectFunction::INPUT_SOURCE:
      this->parent_->command_input(value, false);
      break;
    case SelectFunction::ZONE2_INPUT_SOURCE:
      this->parent_->command_input(value, true);
      break;
    case SelectFunction::PROGRAM:
      this->parent_->command_program(value);
      break;
    case SelectFunction::SCENE:
      this->parent_->command_scene(value);
      break;
    case SelectFunction::AUDIO_SELECT:
      this->parent_->command_audio_select(value);
      break;
    case SelectFunction::NIGHT_MODE:
      this->parent_->command_night_mode(value);
      break;
    case SelectFunction::TUNER_PRESET:
      // The option label carries the preset number, for example "Preset 3".
      // The last digit 1..8 wins so a relabeled option still resolves.
      for (auto it = value.rbegin(); it != value.rend(); ++it) {
        if (*it >= '1' && *it <= '8') {
          this->parent_->command_tuner_preset(static_cast<uint8_t>(*it - '0'));
          return;
        }
      }
      this->parent_->command_tuner_preset(0);
      break;
    case SelectFunction::TUNER_PRESET_PAGE:
      this->parent_->command_tuner_preset_page(value);
      break;
    case SelectFunction::TUNER_BAND:
      this->parent_->command_tuner_band(value);
      break;
    case SelectFunction::SLEEP_TIMER:
      this->parent_->command_sleep_timer(value);
      break;
    case SelectFunction::DECODER_MODE:
      this->parent_->command_decoder_mode(value);
      break;
    case SelectFunction::EXTENDED_SURROUND:
      this->parent_->command_extended_surround(value);
      break;
    case SelectFunction::SPEAKER_B_ASSIGNMENT:
      this->parent_->command_speaker_b_assignment(value);
      break;
    case SelectFunction::ZONE2_AMP:
      this->parent_->command_zone2_amp(value);
      break;
    case SelectFunction::SURROUND_DECODER:
      this->parent_->command_surround_decoder(value);
      break;
  }
}

void YamahaButton::press_action() {
  if (this->parent_ == nullptr) {
    return;
  }
  switch (this->function_) {
    case ButtonFunction::REFRESH:
      this->parent_->command_refresh();
      break;
    case ButtonFunction::POWER_TOGGLE:
      this->parent_->command_power_toggle();
      break;
    case ButtonFunction::VOLUME_UP:
      this->parent_->command_volume_step(true, false);
      break;
    case ButtonFunction::VOLUME_DOWN:
      this->parent_->command_volume_step(false, false);
      break;
    case ButtonFunction::ZONE2_VOLUME_UP:
      this->parent_->command_volume_step(true, true);
      break;
    case ButtonFunction::ZONE2_VOLUME_DOWN:
      this->parent_->command_volume_step(false, true);
      break;
    case ButtonFunction::RECEIVER_RESET:
      this->parent_->command_receiver_reset();
      break;
    case ButtonFunction::TUNER_AUTO_UP:
      this->parent_->command_tuner_auto_seek(true);
      break;
    case ButtonFunction::TUNER_AUTO_DOWN:
      this->parent_->command_tuner_auto_seek(false);
      break;
  }
}

media_player::MediaPlayerTraits YamahaMediaPlayer::get_traits() {
  media_player::MediaPlayerTraits traits;
  traits.clear_feature_flags(media_player::MediaPlayerEntityFeature::BROWSE_MEDIA |
                             media_player::MediaPlayerEntityFeature::STOP |
                             media_player::MediaPlayerEntityFeature::MEDIA_ANNOUNCE);
  traits.set_supports_turn_off_on(true);
  traits.add_feature_flags(
      media_player::MediaPlayerEntityFeature::VOLUME_SET | media_player::MediaPlayerEntityFeature::VOLUME_STEP |
      media_player::MediaPlayerEntityFeature::VOLUME_MUTE | media_player::MediaPlayerEntityFeature::PREVIOUS_TRACK |
      media_player::MediaPlayerEntityFeature::NEXT_TRACK | media_player::MediaPlayerEntityFeature::PLAY);
  return traits;
}

bool YamahaMediaPlayer::is_muted() const { return this->parent_ != nullptr && this->parent_->get_mute_state(); }

void YamahaMediaPlayer::control(const media_player::MediaPlayerCall &call) {
  if (this->parent_ == nullptr) {
    return;
  }

  if (call.get_command().has_value()) {
    switch (call.get_command().value()) {
      case media_player::MEDIA_PLAYER_COMMAND_TURN_ON:
        this->parent_->command_power(true);
        break;
      case media_player::MEDIA_PLAYER_COMMAND_TURN_OFF:
        this->parent_->command_power(false);
        break;
      case media_player::MEDIA_PLAYER_COMMAND_TOGGLE:
        this->parent_->command_power_toggle();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_MUTE:
        this->parent_->command_mute(true);
        break;
      case media_player::MEDIA_PLAYER_COMMAND_UNMUTE:
        this->parent_->command_mute(false);
        break;
      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_UP:
        this->parent_->command_volume_step(true, false);
        break;
      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_DOWN:
        this->parent_->command_volume_step(false, false);
        break;
      case media_player::MEDIA_PLAYER_COMMAND_PLAY:
        this->parent_->command_power(true);
        break;
      case media_player::MEDIA_PLAYER_COMMAND_NEXT:
        this->parent_->command_program_step(true);
        break;
      case media_player::MEDIA_PLAYER_COMMAND_PREVIOUS:
        this->parent_->command_program_step(false);
        break;
      default:
        break;
    }
  }

  if (call.get_volume().has_value()) {
    const float normalized = clamp(call.get_volume().value(), 0.0f, 1.0f);
    const float min_db = this->parent_->get_volume_min_db();
    const float max_db = this->parent_->get_volume_max_db();
    const float db = min_db + (normalized * (max_db - min_db));
    this->parent_->command_volume_absolute(db, false);
  }

  if (call.get_media_url().has_value()) {
    this->parent_->command_media_player_shortcut(call.get_media_url().value());
  }
}
void YamahaSerialComponent::publish_power_state_(bool on) {
  this->main_power_on_ = on;
  if (this->power_switch_ != nullptr) {
    this->power_switch_->publish_state(on);
  }
  if (this->main_zone_power_switch_ != nullptr) {
    this->main_zone_power_switch_->publish_state(on);
  }
  if (this->power_state_text_sensor_ != nullptr) {
    this->power_state_text_sensor_->publish_state(on ? "ON" : "OFF");
  }
  this->sync_media_player_state_();
}

void YamahaSerialComponent::publish_zone2_power_state_(bool on) {
  this->zone2_power_on_ = on;
  if (this->zone2_power_switch_ != nullptr) {
    this->zone2_power_switch_->publish_state(on);
  }
}

void YamahaSerialComponent::publish_mute_state_(bool on) {
  this->mute_on_ = on;
  if (this->mute_switch_ != nullptr) {
    this->mute_switch_->publish_state(on);
  }
  this->sync_media_player_state_();
}

void YamahaSerialComponent::publish_zone2_mute_state_(bool on) {
  this->zone2_mute_on_ = on;
  if (this->zone2_mute_switch_ != nullptr) {
    this->zone2_mute_switch_->publish_state(on);
  }
}

void YamahaSerialComponent::publish_speaker_relay_state_(char relay, bool on) {
  if (relay == 'A') {
    this->speaker_a_on_ = on;
    if (this->speaker_a_switch_ != nullptr) {
      this->speaker_a_switch_->publish_state(on);
    }
    return;
  }
  if (relay == 'B') {
    this->speaker_b_on_ = on;
    if (this->speaker_b_switch_ != nullptr) {
      this->speaker_b_switch_->publish_state(on);
    }
  }
}

void YamahaSerialComponent::publish_volume_db_(float db, bool zone2) {
  if (zone2) {
    this->zone2_volume_db_ = db;
    if (this->zone2_volume_db_sensor_ != nullptr) {
      this->zone2_volume_db_sensor_->publish_state(db);
    }
    if (this->zone2_volume_number_ != nullptr) {
      this->zone2_volume_number_->publish_state(db);
    }
  } else {
    this->main_volume_db_ = db;
    if (this->volume_db_sensor_ != nullptr) {
      this->volume_db_sensor_->publish_state(db);
    }
    if (this->volume_number_ != nullptr) {
      this->volume_number_->publish_state(db);
    }
    this->sync_media_player_state_();
  }
}

void YamahaSerialComponent::publish_input_by_report_(uint8_t report_id, bool zone2) {
  for (const auto &input : this->inputs_) {
    const uint8_t expected = zone2 ? input.zone2_report_id : input.report_id;
    if (expected == report_id) {
      this->publish_input_label_(input.label, zone2);
      return;
    }
  }
}

void YamahaSerialComponent::publish_input_label_(const std::string &label, bool zone2) {
  const std::string value = trim_(label);
  if (value.empty()) {
    return;
  }
  if (zone2) {
    this->zone2_input_label_ = value;
    if (this->zone2_input_source_text_sensor_ != nullptr) {
      this->zone2_input_source_text_sensor_->publish_state(value);
    }
    if (this->zone2_input_source_select_ != nullptr && this->zone2_input_source_select_->has_option(value)) {
      this->zone2_input_source_select_->publish_state(value);
    }
  } else {
    this->input_label_ = value;
    if (this->input_source_text_sensor_ != nullptr) {
      this->input_source_text_sensor_->publish_state(value);
    }
    if (this->input_source_select_ != nullptr && this->input_source_select_->has_option(value)) {
      this->input_source_select_->publish_state(value);
    }
  }
}

void YamahaSerialComponent::publish_program_by_report_(uint8_t report_code) {
  if (report_code >= 0x80 && report_code <= 0xB3) {
    this->publish_program_label_("STRAIGHT");
    return;
  }

  for (const auto &program : this->programs_) {
    if (program.report_code == report_code) {
      this->publish_program_label_(program.label);
      return;
    }
  }
}

void YamahaSerialComponent::publish_program_label_(const std::string &label) {
  this->program_label_ = label;
  if (this->program_text_sensor_ != nullptr) {
    this->program_text_sensor_->publish_state(label);
  }
  if (this->program_select_ != nullptr && this->program_select_->has_option(label)) {
    this->program_select_->publish_state(label);
  }
}

void YamahaSerialComponent::publish_bass_percent_(float percent) {
  this->bass_percent_ = percent;
  if (this->bass_number_ != nullptr) {
    this->bass_number_->publish_state(percent);
  }
}

void YamahaSerialComponent::publish_treble_percent_(float percent) {
  this->treble_percent_ = percent;
  if (this->treble_number_ != nullptr) {
    this->treble_number_->publish_state(percent);
  }
}

void YamahaSerialComponent::publish_speaker_distance_(uint8_t channel_id, uint16_t value) {
  switch (channel_id) {
    case 0x0:
      if (this->center_distance_number_ != nullptr) {
        this->center_distance_number_->publish_state(static_cast<float>(value));
      }
      break;
    case 0x2:
      if (this->front_left_distance_number_ != nullptr) {
        this->front_left_distance_number_->publish_state(static_cast<float>(value));
      }
      break;
    case 0x3:
      if (this->front_right_distance_number_ != nullptr) {
        this->front_right_distance_number_->publish_state(static_cast<float>(value));
      }
      break;
    case 0x4:
      if (this->surround_left_distance_number_ != nullptr) {
        this->surround_left_distance_number_->publish_state(static_cast<float>(value));
      }
      break;
    case 0x5:
      if (this->surround_right_distance_number_ != nullptr) {
        this->surround_right_distance_number_->publish_state(static_cast<float>(value));
      }
      break;
    case 0xA:
      if (this->subwoofer_distance_number_ != nullptr) {
        this->subwoofer_distance_number_->publish_state(static_cast<float>(value));
      }
      break;
    default:
      break;
  }
}

void YamahaSerialComponent::publish_dimmer_percent_(uint8_t dimmer_nibble) {
  const float percent = static_cast<float>(clamp(static_cast<int>(dimmer_nibble), 0, 4) * 25);
  if (this->dimmer_number_ != nullptr) {
    this->dimmer_number_->publish_state(percent);
  }
}

void YamahaSerialComponent::publish_audio_select_by_report_(uint8_t report_id) {
  if (!lookup_label_(AUDIO_SELECT_REPORTS, report_id, this->audio_select_label_)) {
    return;
  }
  if (this->audio_select_select_ != nullptr && this->audio_select_select_->has_option(this->audio_select_label_)) {
    this->audio_select_select_->publish_state(this->audio_select_label_);
  }
  if (this->audio_select_text_sensor_ != nullptr) {
    this->audio_select_text_sensor_->publish_state(this->audio_select_label_);
  }
}

void YamahaSerialComponent::publish_night_mode_by_report_(uint8_t high_nibble, uint8_t low_nibble) {
  const uint8_t key = static_cast<uint8_t>((high_nibble << 4) | low_nibble);
  if (!lookup_label_(NIGHT_MODE_REPORTS, key, this->night_mode_label_)) {
    return;
  }
  if (this->night_mode_select_ != nullptr && this->night_mode_select_->has_option(this->night_mode_label_)) {
    this->night_mode_select_->publish_state(this->night_mode_label_);
  }
  if (this->night_mode_text_sensor_ != nullptr) {
    this->night_mode_text_sensor_->publish_state(this->night_mode_label_);
  }
}

void YamahaSerialComponent::publish_tuner_preset_by_report_(uint8_t report_id) {
  if (report_id > 0x07) {
    return;
  }
  this->tuner_preset_label_ = "Preset " + std::to_string(report_id + 1);
  if (this->tuner_preset_select_ != nullptr && this->tuner_preset_select_->has_option(this->tuner_preset_label_)) {
    this->tuner_preset_select_->publish_state(this->tuner_preset_label_);
  }
}

void YamahaSerialComponent::publish_tuner_preset_page_by_report_(uint8_t report_id) {
  if (!lookup_label_(TUNER_PRESET_PAGE_REPORTS, report_id, this->tuner_preset_page_label_)) {
    return;
  }
  if (this->tuner_preset_page_select_ != nullptr &&
      this->tuner_preset_page_select_->has_option(this->tuner_preset_page_label_)) {
    this->tuner_preset_page_select_->publish_state(this->tuner_preset_page_label_);
  }
}

void YamahaSerialComponent::publish_tuner_band_by_report_(uint8_t report_id) {
  // TUNER_BAND_REPORTS is 00 = FM, 01 = AM (x500-x800 manuals); the RX-V1900 reports the reverse.
  if (this->profile_caps_.tuner_band_fm_is_1 && report_id <= 1)
    report_id ^= 1;
  if (!lookup_label_(TUNER_BAND_REPORTS, report_id, this->tuner_band_label_)) {
    return;
  }
  if (this->tuner_band_select_ != nullptr && this->tuner_band_select_->has_option(this->tuner_band_label_)) {
    this->tuner_band_select_->publish_state(this->tuner_band_label_);
  }
}

void YamahaSerialComponent::publish_sleep_timer_by_report_(uint8_t report_id) {
  if (!this->profile_caps_.sleep_control) {
    return;
  }
  if (!lookup_label_(SLEEP_TIMER_REPORTS, report_id, this->sleep_timer_label_)) {
    return;
  }
  if (this->sleep_timer_select_ != nullptr && this->sleep_timer_select_->has_option(this->sleep_timer_label_)) {
    this->sleep_timer_select_->publish_state(this->sleep_timer_label_);
  }
}

void YamahaSerialComponent::publish_decoder_mode_by_report_(uint8_t report_id) {
  if (!lookup_label_(DECODER_MODE_REPORTS, report_id, this->decoder_mode_label_)) {
    return;
  }
  if (this->decoder_mode_select_ != nullptr && this->decoder_mode_select_->has_option(this->decoder_mode_label_)) {
    this->decoder_mode_select_->publish_state(this->decoder_mode_label_);
  }
}

void YamahaSerialComponent::publish_extended_surround_by_report_(uint8_t report_id) {
  if (!lookup_label_(EXTENDED_SURROUND_REPORTS, report_id, this->extended_surround_label_)) {
    return;
  }
  if (this->extended_surround_select_ != nullptr &&
      this->extended_surround_select_->has_option(this->extended_surround_label_)) {
    this->extended_surround_select_->publish_state(this->extended_surround_label_);
  }
}

void YamahaSerialComponent::publish_speaker_b_assignment_by_report_(uint8_t report_id) {
  if (!lookup_label_(SPEAKER_B_ASSIGNMENT_REPORTS, report_id, this->speaker_b_assignment_label_)) {
    return;
  }
  if (this->speaker_b_assignment_select_ != nullptr &&
      this->speaker_b_assignment_select_->has_option(this->speaker_b_assignment_label_)) {
    this->speaker_b_assignment_select_->publish_state(this->speaker_b_assignment_label_);
  }
}

void YamahaSerialComponent::publish_zone2_amp_by_report_(uint8_t report_id) {
  if (!lookup_label_(ZONE2_AMP_REPORTS, report_id, this->zone2_amp_label_)) {
    return;
  }
  if (this->zone2_amp_select_ != nullptr && this->zone2_amp_select_->has_option(this->zone2_amp_label_)) {
    this->zone2_amp_select_->publish_state(this->zone2_amp_label_);
  }
}

void YamahaSerialComponent::publish_scene_by_report_(uint8_t report_id) {
  // System Memory recall reports are 1-based.
  if (report_id == 0 || report_id > this->scenes_.size()) {
    return;
  }
  this->scene_label_ = this->scenes_[report_id - 1].label;
  if (this->scene_select_ != nullptr && this->scene_select_->has_option(this->scene_label_)) {
    this->scene_select_->publish_state(this->scene_label_);
  }
}

void YamahaSerialComponent::publish_surround_decoder_by_report_(uint8_t report_id) {
  if (!lookup_label_(SURROUND_DECODER_REPORTS, report_id, this->surround_decoder_label_)) {
    return;
  }
  if (this->surround_decoder_select_ != nullptr &&
      this->surround_decoder_select_->has_option(this->surround_decoder_label_)) {
    this->surround_decoder_select_->publish_state(this->surround_decoder_label_);
  }
}

void YamahaSerialComponent::publish_playback_format_by_report_(uint8_t report_id) {
  const bool known = this->profile_caps_.x700_protocol
                         ? lookup_label_(PLAYBACK_FORMAT_REPORTS_X700, report_id, this->playback_format_)
                         : lookup_label_(PLAYBACK_FORMAT_REPORTS, report_id, this->playback_format_);
  if (!known) {
    return;
  }
  if (this->playback_format_text_sensor_ != nullptr) {
    this->playback_format_text_sensor_->publish_state(this->playback_format_);
  }
}

void YamahaSerialComponent::publish_sampling_rate_by_report_(uint8_t report_id) {
  SamplingRateInfo info;
  const bool known = this->profile_caps_.x700_protocol ? lookup_report_(SAMPLING_RATE_REPORTS_X700, report_id, info)
                                                       : lookup_report_(SAMPLING_RATE_REPORTS, report_id, info);
  if (!known) {
    return;
  }
  this->sampling_rate_ = info.label;
  if (this->sampling_rate_text_sensor_ != nullptr) {
    this->sampling_rate_text_sensor_->publish_state(this->sampling_rate_);
  }
  this->sampling_rate_hz_ = info.hz;
  if (this->sampling_rate_sensor_ != nullptr) {
    this->sampling_rate_sensor_->publish_state(this->sampling_rate_hz_);
  }
}

void YamahaSerialComponent::sync_media_player_state_() {
  if (this->media_player_ == nullptr) {
    return;
  }

  this->media_player_->state =
      this->main_power_on_ ? media_player::MEDIA_PLAYER_STATE_IDLE : media_player::MEDIA_PLAYER_STATE_OFF;

  if (!std::isnan(this->main_volume_db_)) {
    const float span = std::max(0.1f, this->volume_max_db_ - this->volume_min_db_);
    const float normalized = clamp((this->main_volume_db_ - this->volume_min_db_) / span, 0.0f, 1.0f);
    this->media_player_->volume = normalized;
  }

  this->media_player_->publish_state();
}

}  // namespace esphome::yamaha_serial
