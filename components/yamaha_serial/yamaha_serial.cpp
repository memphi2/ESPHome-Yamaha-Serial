// SPDX-License-Identifier: GPL-3.0-only

#include "yamaha_serial.h"

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
static const char *const CMD_VOL_UP = "07A1A";
static const char *const CMD_VOL_DOWN = "07A1B";
static const char *const CMD_ZONE2_VOL_UP = "07ADA";
static const char *const CMD_ZONE2_VOL_DOWN = "07ADB";
static const char *const CMD_AUDIO_SELECT_AUTO = "07EA6";
static const char *const CMD_AUDIO_SELECT_COAX = "07EA9";
static const char *const CMD_AUDIO_SELECT_ANALOG = "07EAA";
static const char *const CMD_AUDIO_SELECT_HDMI = "07EDA";
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
static const char *const CMD_NIGHT_MODE_OFF = "28B00";
static const char *const CMD_NIGHT_MODE_CINEMA_LOW = "28B10";
static const char *const CMD_NIGHT_MODE_CINEMA_MID = "28B11";
static const char *const CMD_NIGHT_MODE_CINEMA_HIGH = "28B12";
static const char *const CMD_NIGHT_MODE_MUSIC_LOW = "28B20";
static const char *const CMD_NIGHT_MODE_MUSIC_MID = "28B21";
static const char *const CMD_NIGHT_MODE_MUSIC_HIGH = "28B22";
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

struct ReportLabel {
  uint8_t report;
  const char *label;
};

struct SamplingRateInfo {
  uint8_t report;
  const char *label;
  float hz;
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

static const char *lookup_label_(const ReportLabel *entries, size_t count, uint8_t report) {
  for (size_t i = 0; i < count; i++) {
    if (report == entries[i].report) {
      return entries[i].label;
    }
  }
  return nullptr;
}

static const SamplingRateInfo *lookup_sampling_rate_(const SamplingRateInfo *entries, size_t count, uint8_t report) {
  for (size_t i = 0; i < count; i++) {
    if (report == entries[i].report) {
      return &entries[i];
    }
  }
  return nullptr;
}

static const LabelCommand AUDIO_SELECT_COMMANDS[] YAMAHA_SERIAL_FLASH_TABLE = {
    {"auto", CMD_AUDIO_SELECT_AUTO},
    {"coax_opt", CMD_AUDIO_SELECT_COAX},
    {"coax", CMD_AUDIO_SELECT_COAX},
    {"optical", CMD_AUDIO_SELECT_COAX},
    {"analog", CMD_AUDIO_SELECT_ANALOG},
    {"hdmi", CMD_AUDIO_SELECT_HDMI},
};
static const ReportLabel AUDIO_SELECT_REPORTS[] = {
    {0x0, "Auto"},
    {0x3, "Coax/Opt"},
    {0x4, "Analog"},
    {0x5, "Analog Only"},
    {0x8, "HDMI"},
};

static const LabelCommand NIGHT_MODE_COMMANDS[] YAMAHA_SERIAL_FLASH_TABLE = {
    {"off", CMD_NIGHT_MODE_OFF},
    {"cinema_low", CMD_NIGHT_MODE_CINEMA_LOW},
    {"cinema_mid", CMD_NIGHT_MODE_CINEMA_MID},
    {"cinema_high", CMD_NIGHT_MODE_CINEMA_HIGH},
    {"music_low", CMD_NIGHT_MODE_MUSIC_LOW},
    {"music_mid", CMD_NIGHT_MODE_MUSIC_MID},
    {"music_high", CMD_NIGHT_MODE_MUSIC_HIGH},
};
static const ReportLabel NIGHT_MODE_REPORTS[] = {
    {0x00, "Off"},        {0x10, "Cinema Low"}, {0x11, "Cinema Mid"}, {0x12, "Cinema High"},
    {0x20, "Music Low"}, {0x21, "Music Mid"},  {0x22, "Music High"},
};

static const LabelCommand TUNER_PRESET_PAGE_COMMANDS[] YAMAHA_SERIAL_FLASH_TABLE = {
    {"a", "07AE0"}, {"b", "07AE1"}, {"c", "07AE2"}, {"d", "07AE3"}, {"e", "07AE4"},
};
static const ReportLabel TUNER_PRESET_PAGE_REPORTS[] = {
    {0x00, "A"}, {0x01, "B"}, {0x02, "C"}, {0x03, "D"}, {0x04, "E"},
};

static const LabelCommand TUNER_BAND_COMMANDS[] YAMAHA_SERIAL_FLASH_TABLE = {
    {"fm", "07EBC"},
    {"am", "07EBD"},
};
static const ReportLabel TUNER_BAND_REPORTS[] = {
    {0x00, "FM"},
    {0x01, "AM"},
};

static const LabelCommand SLEEP_TIMER_COMMANDS[] YAMAHA_SERIAL_FLASH_TABLE = {
    {"off", "07EB3"}, {"120_min", "07EB4"}, {"90_min", "07EB5"}, {"60_min", "07EB6"},
    {"30_min", "07EB7"}, {"120", "07EB4"}, {"90", "07EB5"}, {"60", "07EB6"}, {"30", "07EB7"},
};
static const ReportLabel SLEEP_TIMER_REPORTS[] = {
    {0x00, "120 min"}, {0x01, "90 min"}, {0x02, "60 min"}, {0x03, "30 min"}, {0x04, "Off"},
};

static const LabelCommand DECODER_MODE_COMMANDS[] YAMAHA_SERIAL_FLASH_TABLE = {
    {"auto", "07EDB"},
    {"dts", "07EA8"},
    {"aac", "07E3B"},
};
static const ReportLabel DECODER_MODE_REPORTS[] = {
    {0x00, "Auto"},
    {0x10, "DTS"},
    {0x20, "AAC"},
};

static const LabelCommand EXTENDED_SURROUND_COMMANDS[] YAMAHA_SERIAL_FLASH_TABLE = {
    {"ex_es", "07EB8"}, {"off", "07EB9"}, {"auto", "07E7C"},
    {"ex", "07EDC"},    {"pliix_movie", "07EDD"}, {"pliix_music", "07EDE"},
};
static const ReportLabel EXTENDED_SURROUND_REPORTS[] = {
    {0x00, "Off"},
    {0x01, "Matrix On"},
    {0x02, "Discrete On"},
    {0x03, "Auto"},
};

static const LabelCommand SPEAKER_B_ASSIGNMENT_COMMANDS[] YAMAHA_SERIAL_FLASH_TABLE = {
    {"main", "07E28"},
    {"zone_b", "07E29"},
};
static const ReportLabel SPEAKER_B_ASSIGNMENT_REPORTS[] = {
    {0x00, "Main"},
    {0x01, "Zone B"},
};

static const LabelCommand ZONE2_AMP_COMMANDS[] YAMAHA_SERIAL_FLASH_TABLE = {
    {"internal", "07E99"},
    {"int_presence", "07E99"},
    {"external", "07E9A"},
    {"ext", "07E9A"},
};
static const ReportLabel ZONE2_AMP_REPORTS[] = {
    {0x00, "External"},
    {0x01, "Internal"},
    {0x02, "Internal"},
    {0x03, "Internal"},
};

static const ReportLabel PLAYBACK_FORMAT_REPORTS[] = {
    {0x0, "6CH Input"},          {0x1, "Analog"},      {0x2, "PCM"},
    {0x3, "Dolby Digital"},     {0x4, "Dolby Digital 2.0"},
    {0x5, "Dolby Digital Karaoke"}, {0x6, "Dolby Digital EX"},
    {0x7, "DTS"},               {0x8, "DTS-ES"},      {0x9, "Other Digital"},
};

static const SamplingRateInfo SAMPLING_RATE_REPORTS[] = {
    {0x0, "Analog", 0.0f},        {0x1, "32kHz", 32000.0f},  {0x2, "44.1kHz", 44100.0f},
    {0x3, "48kHz", 48000.0f},    {0x4, "64kHz", 64000.0f},  {0x5, "88.2kHz", 88200.0f},
    {0x6, "96kHz", 96000.0f},    {0x7, "Unknown", NAN},     {0x8, "128.0kHz", 128000.0f},
    {0x9, "176.4kHz", 176400.0f}, {0xA, "192.0kHz", 192000.0f},
    {0xB, "48kHz (DTS 96/24)", 48000.0f},
};

void YamahaPowerSwitch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_power(state);
}

void YamahaMainZonePowerSwitch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_main_zone_power(state);
}

void YamahaMuteSwitch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_mute(state);
}

void YamahaZone2PowerSwitch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_zone2_power(state);
}

void YamahaPureDirectSwitch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_pure_direct(state);
}

void YamahaFanModeSwitch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_fan_mode(state);
}

void YamahaZone2MuteSwitch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_zone2_mute(state);
}

void YamahaSpeakerASwitch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_speaker_relay('A', state);
}

void YamahaSpeakerBSwitch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_speaker_relay('B', state);
}

void YamahaVolumeNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_volume_absolute(value, false);
}

void YamahaZone2VolumeNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_volume_absolute(value, true);
}

void YamahaBassNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_bass_percent(value);
}

void YamahaTrebleNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_treble_percent(value);
}

void YamahaCenterDistanceNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_speaker_distance(0x0, static_cast<uint16_t>(std::lround(value)));
}

void YamahaFrontLeftDistanceNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_speaker_distance(0x2, static_cast<uint16_t>(std::lround(value)));
}

void YamahaFrontRightDistanceNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_speaker_distance(0x3, static_cast<uint16_t>(std::lround(value)));
}

void YamahaSurroundLeftDistanceNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_speaker_distance(0x4, static_cast<uint16_t>(std::lround(value)));
}

void YamahaSurroundRightDistanceNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_speaker_distance(0x5, static_cast<uint16_t>(std::lround(value)));
}

void YamahaSubwooferDistanceNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_speaker_distance(0xA, static_cast<uint16_t>(std::lround(value)));
}

void YamahaDimmerNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_dimmer_percent(value);
}

void YamahaTunerFmFrequencyNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_tuner_frequency(value, true);
}

void YamahaTunerAmFrequencyNumber::control(float value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_tuner_frequency(value, false);
}

void YamahaInputSelect::set_options_from_labels(const std::vector<std::string> &labels) {
  this->stored_options_ = labels;
  FixedVector<const char *> options{};
  options.init(this->stored_options_.size());
  for (const auto &label : this->stored_options_) {
    options.push_back(label.c_str());
  }
  this->traits.set_options(options);
}

void YamahaInputSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_input(value, false);
}

void YamahaZone2InputSelect::set_options_from_labels(const std::vector<std::string> &labels) {
  this->stored_options_ = labels;
  FixedVector<const char *> options{};
  options.init(this->stored_options_.size());
  for (const auto &label : this->stored_options_) {
    options.push_back(label.c_str());
  }
  this->traits.set_options(options);
}

void YamahaZone2InputSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_input(value, true);
}

void YamahaProgramSelect::set_options_from_labels(const std::vector<std::string> &labels) {
  this->stored_options_ = labels;
  FixedVector<const char *> options{};
  options.init(this->stored_options_.size());
  for (const auto &label : this->stored_options_) {
    options.push_back(label.c_str());
  }
  this->traits.set_options(options);
}

void YamahaProgramSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_program(value);
}

void YamahaSceneSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_scene(value);
}

void YamahaAudioSelectSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_audio_select(value);
}

void YamahaNightModeSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_night_mode(value);
}

void YamahaTunerPresetSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    return;
  }
  for (auto it = value.rbegin(); it != value.rend(); ++it) {
    if (*it >= '1' && *it <= '8') {
      this->parent_->command_tuner_preset(static_cast<uint8_t>(*it - '0'));
      return;
    }
  }
  this->parent_->command_tuner_preset(0);
}

void YamahaTunerPresetPageSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_tuner_preset_page(value);
}

void YamahaTunerBandSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_tuner_band(value);
}

void YamahaSleepTimerSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_sleep_timer(value);
}

void YamahaDecoderModeSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_decoder_mode(value);
}

void YamahaExtendedSurroundSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_extended_surround(value);
}

void YamahaSpeakerBAssignmentSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_speaker_b_assignment(value);
}

void YamahaZone2AmpSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_zone2_amp(value);
}

void YamahaRefreshButton::press_action() {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_refresh();
}

void YamahaPowerToggleButton::press_action() {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_power_toggle();
}

void YamahaVolumeUpButton::press_action() {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_volume_step(true, false);
}

void YamahaVolumeDownButton::press_action() {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_volume_step(false, false);
}

void YamahaZone2VolumeUpButton::press_action() {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_volume_step(true, true);
}

void YamahaZone2VolumeDownButton::press_action() {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_volume_step(false, true);
}

void YamahaReceiverResetButton::press_action() {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_receiver_reset();
}

void YamahaTunerAutoUpButton::press_action() {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_tuner_auto_seek(true);
}

void YamahaTunerAutoDownButton::press_action() {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->command_tuner_auto_seek(false);
}

media_player::MediaPlayerTraits YamahaMediaPlayer::get_traits() {
  media_player::MediaPlayerTraits traits;
  traits.clear_feature_flags(media_player::MediaPlayerEntityFeature::BROWSE_MEDIA |
                             media_player::MediaPlayerEntityFeature::STOP |
                             media_player::MediaPlayerEntityFeature::MEDIA_ANNOUNCE);
  traits.set_supports_turn_off_on(true);
  traits.add_feature_flags(media_player::MediaPlayerEntityFeature::VOLUME_SET |
                           media_player::MediaPlayerEntityFeature::VOLUME_STEP |
                           media_player::MediaPlayerEntityFeature::VOLUME_MUTE |
                           media_player::MediaPlayerEntityFeature::PREVIOUS_TRACK |
                           media_player::MediaPlayerEntityFeature::NEXT_TRACK |
                           media_player::MediaPlayerEntityFeature::PLAY);
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

void YamahaSerialComponent::set_volume_config(float min_db, float max_db, float step_db) {
  this->volume_min_db_ = min_db;
  this->volume_max_db_ = max_db;
  if (this->volume_min_db_ > this->volume_max_db_) {
    std::swap(this->volume_min_db_, this->volume_max_db_);
  }
  this->volume_step_db_ = std::max(step_db, 0.5f);
}

void YamahaSerialComponent::set_input_source_select(YamahaInputSelect *obj) {
  this->input_source_select_ = obj;
  this->configure_input_select_options_();
}

void YamahaSerialComponent::set_zone2_input_source_select(YamahaZone2InputSelect *obj) {
  this->zone2_input_source_select_ = obj;
  this->configure_input_select_options_();
}

void YamahaSerialComponent::set_program_select(YamahaProgramSelect *obj) {
  this->program_select_ = obj;
  this->configure_program_select_options_();
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

  InputMapping mapping{};
  mapping.key = key_norm;
  mapping.label = label.empty() ? key : label;
  mapping.report_id = 0xFF;
  mapping.zone2_report_id = 0xFF;
  mapping.main_command = "";
  mapping.zone2_command = "";
  this->inputs_.push_back(mapping);
  this->configure_input_select_options_();
}

void YamahaSerialComponent::setup() {
  this->frame_buffer_.reserve(BUFFER_MAX_LEN);
  this->setup_default_mappings_();
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
  while (this->available()) {
    const int next = this->read();
    if (next < 0) {
      break;
    }
    this->process_incoming_byte_(static_cast<uint8_t>(next));
  }

  this->publish_diagnostics_();
  this->check_availability_();
  this->process_command_queue_();
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
  ESP_LOGCONFIG(TAG, "  Startup probe mode: poll waits until first receiver response");
  ESP_LOGCONFIG(TAG, "  Input mappings: %u", static_cast<unsigned>(this->inputs_.size()));
}

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
  if (zone2) {
    if (!this->profile_caps_.supports_zone2) {
      this->set_last_error_("Zone2 volume control not supported by active profile");
      return;
    }
    this->queue_simple_command_(FrameType::STX, up ? CMD_ZONE2_VOL_UP : CMD_ZONE2_VOL_DOWN,
                                up ? "zone2_volume_up" : "zone2_volume_down");
  } else {
    this->queue_simple_command_(FrameType::STX, up ? CMD_VOL_UP : CMD_VOL_DOWN, up ? "volume_up" : "volume_down");
  }
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
  const size_t target = next ? (current + 1) % this->programs_.size()
                             : (current + this->programs_.size() - 1) % this->programs_.size();
  this->command_program(this->programs_[target].label);
}

void YamahaSerialComponent::command_media_player_shortcut(const std::string &value) {
  std::string shortcut = trim_(value);
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
    while (!shortcut.empty() && (shortcut.front() == ':' || shortcut.front() == '/' || shortcut.front() == ' ' ||
                                 shortcut.front() == '=')) {
      shortcut.erase(shortcut.begin());
    }
    return true;
  };

  if (strip_prefix("yamaha://input") || strip_prefix("yamaha:input") ||
      strip_prefix("media-source://yamaha/input") || strip_prefix("media_source://yamaha/input") ||
      strip_prefix("yamaha://source") || strip_prefix("yamaha:source") ||
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
  if (strip_prefix("yamaha://scene") || strip_prefix("yamaha:scene") ||
      strip_prefix("media-source://yamaha/scene") || strip_prefix("media_source://yamaha/scene") ||
      strip_prefix("yamaha://system_memory") || strip_prefix("yamaha:system_memory") ||
      strip_prefix("media-source://yamaha/system_memory") || strip_prefix("media_source://yamaha/system_memory") ||
      strip_prefix("scene") || strip_prefix("system_memory") || strip_prefix("system memory")) {
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
  this->queue_simple_command_(FrameType::STX, on ? CMD_FAN_MODE_ON : CMD_FAN_MODE_OFF, on ? "fan_mode_on" : "fan_mode_off");
}

void YamahaSerialComponent::command_pure_direct(bool on) {
  if (!this->profile_caps_.supports_pure_direct) {
    this->set_last_error_("Pure direct control not supported by active profile");
    return;
  }
  const std::string configured_model = normalize_token_(this->configured_model_);
  const std::string reported_model = normalize_token_(this->receiver_model_);
  const bool rx_v2500 = configured_model.find("v2500") != std::string::npos ||
                        reported_model.find("v2500") != std::string::npos;
  const char *command = rx_v2500 ? (on ? "07E82" : "07E83") : (on ? CMD_PURE_DIRECT_ON : CMD_PURE_DIRECT_OFF);
  this->queue_simple_command_(FrameType::STX, command,
                              on ? "pure_direct_on" : "pure_direct_off");
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
  const std::string key = normalize_token_(label);
  const char *command = lookup_command_(SLEEP_TIMER_COMMANDS, std::size(SLEEP_TIMER_COMMANDS), key);
  if (command == nullptr) {
    this->set_last_error_("Unknown sleep timer value: " + label);
    return;
  }
  this->queue_simple_command_(FrameType::STX, command, "sleep_timer_set");
}

void YamahaSerialComponent::command_decoder_mode(const std::string &label) {
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

void YamahaSerialComponent::command_tuner_auto_seek(bool up) {
  this->queue_simple_command_(FrameType::STX, CMD_TUNER_INPUT, "tuner_input");
  this->queue_simple_command_(FrameType::STX, up ? CMD_TUNER_AUTO_UP : CMD_TUNER_AUTO_DOWN,
                              up ? "tuner_auto_up" : "tuner_auto_down");
}

void YamahaSerialComponent::command_dimmer_percent(float percent) {
  if (!this->profile_caps_.supports_extended) {
    this->set_last_error_("Dimmer control requires extended profile support");
    return;
  }
  const auto clamped = static_cast<int>(clamp(std::lround(percent), 0L, 100L));
  const uint8_t level = static_cast<uint8_t>(clamp(clamped / 25, 0, 4));
  char payload[6]{0};
  std::snprintf(payload, sizeof(payload), "2610%X", level);
  this->queue_simple_command_(FrameType::STX, payload, "dimmer_set");
}

void YamahaSerialComponent::command_tuner_preset(uint8_t preset) {
  if (!this->profile_caps_.supports_extended) {
    this->set_last_error_("Tuner preset control requires extended profile support");
    return;
  }
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
  if (this->tuner_preset_select_ != nullptr) {
    this->tuner_preset_select_->publish_state("Preset " + std::to_string(preset));
  }
}

void YamahaSerialComponent::command_tuner_frequency(float frequency, bool fm) {
  if (!this->profile_caps_.supports_extended) {
    this->set_last_error_("Tuner frequency control requires extended profile support");
    return;
  }

  char freq_chars[7]{0};
  if (fm) {
    const float clamped = clamp(frequency, 76.0f, 108.0f);
    std::snprintf(freq_chars, sizeof(freq_chars), "%06.2f", static_cast<double>(clamped));
  } else {
    const auto clamped = static_cast<unsigned>(clamp(static_cast<int>(std::lround(frequency)), 500, 1800));
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
  if (!this->profile_caps_.supports_extended) {
    this->set_last_error_("Bass control requires extended profile support");
    return;
  }
  const auto clamped = static_cast<int>(clamp(std::lround(percent), 0L, 100L));
  const uint8_t raw = static_cast<uint8_t>(clamp(static_cast<int>(std::lround(static_cast<float>(clamped) / 4.1666667f)), 0, 0x18));
  char write_cmd[14]{0};
  std::snprintf(write_cmd, sizeof(write_cmd), "20090331001%02X", raw);
  this->queue_simple_command_(FrameType::DC4, write_cmd, "bass_set");
  this->queue_simple_command_(FrameType::DC4, "2006033000", "poll_bass");
}

void YamahaSerialComponent::command_treble_percent(float percent) {
  if (!this->profile_caps_.supports_extended) {
    this->set_last_error_("Treble control requires extended profile support");
    return;
  }
  const auto clamped = static_cast<int>(clamp(std::lround(percent), 0L, 100L));
  const uint8_t raw = static_cast<uint8_t>(clamp(static_cast<int>(std::lround(static_cast<float>(clamped) / 4.1666667f)), 0, 0x18));
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
  std::string raw = trim_(value);
  if (raw.empty()) {
    this->set_last_error_("Raw Yamaha command is empty");
    return;
  }

  const auto separator = raw.find_first_of(": /");
  if (separator == std::string::npos) {
    if (this->queue_named_command_(raw)) {
      return;
    }
    this->set_last_error_("Raw Yamaha command requires a known alias or prefix: stx:, dc4:, dc1:, or dc3:reset");
    return;
  }

  const std::string prefix = normalize_token_(raw.substr(0, separator));
  std::string payload = trim_(raw.substr(separator + 1));
  while (!payload.empty() && (payload.front() == ':' || payload.front() == '/')) {
    payload.erase(payload.begin());
    payload = trim_(payload);
  }
  payload.erase(std::remove_if(payload.begin(), payload.end(),
                               [](unsigned char ch) { return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'; }),
                payload.end());
  payload = uppercase_ascii_(payload);

  if (prefix == "dc3" && (normalize_token_(payload) == "reset" || payload == "7F7F7F")) {
    this->command_receiver_reset();
    return;
  }
  if (prefix == "alias" || prefix == "command" || prefix == "cmd") {
    if (this->queue_named_command_(payload)) {
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

  if (payload.empty() || payload.size() > 160 || !payload_is_printable_ascii_(payload)) {
    this->set_last_error_("Invalid raw Yamaha command payload");
    return;
  }
  if (frame_type == FrameType::DC4 && (payload.size() < 4 || payload[0] != '2' || payload[1] != '0')) {
    this->set_last_error_("Extended raw Yamaha command must start with 20 length/header bytes");
    return;
  }

  this->queue_simple_command_(frame_type, payload, "raw");
}

bool YamahaSerialComponent::queue_named_command_(const std::string &alias) {
  struct CommandAlias {
    const char *alias;
    const char *payload;
    uint8_t frame_type;
    bool expect_response;
  } __attribute__((packed));

  static const CommandAlias COMMAND_ALIASES[] YAMAHA_SERIAL_FLASH_TABLE = {
      {"power_on", "07E7E", static_cast<uint8_t>(FrameType::STX), true},
      {"power_off", "07E7F", static_cast<uint8_t>(FrameType::STX), true},
      {"all_zone_power_on", "07A1D", static_cast<uint8_t>(FrameType::STX), true},
      {"all_zone_power_off", "07A1E", static_cast<uint8_t>(FrameType::STX), true},
      {"main_power_on", "07E7E", static_cast<uint8_t>(FrameType::STX), true},
      {"main_power_off", "07E7F", static_cast<uint8_t>(FrameType::STX), true},
      {"main_zone_power_on", "07E7E", static_cast<uint8_t>(FrameType::STX), true},
      {"main_zone_power_off", "07E7F", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_power_on", "07EBA", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_power_off", "07EBB", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_power_on", "07AED", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_power_off", "07AEE", static_cast<uint8_t>(FrameType::STX), true},
      {"volume_up", "07A1A", static_cast<uint8_t>(FrameType::STX), true},
      {"volume_down", "07A1B", static_cast<uint8_t>(FrameType::STX), true},
      {"mute_on", "07EA2", static_cast<uint8_t>(FrameType::STX), true},
      {"mute_full_on", "07EA2", static_cast<uint8_t>(FrameType::STX), true},
      {"mute_20db_on", "07EDF", static_cast<uint8_t>(FrameType::STX), true},
      {"mute_off", "07EA3", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_volume_up", "07ADA", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_volume_down", "07ADB", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_mute_on", "07EA0", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_mute_off", "07EA1", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_volume_up", "07AFD", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_volume_down", "07AFE", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_mute_on", "07E26", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_mute_off", "07E66", static_cast<uint8_t>(FrameType::STX), true},

      {"input_phono", "07A14", static_cast<uint8_t>(FrameType::STX), true},
      {"input_cd", "07A15", static_cast<uint8_t>(FrameType::STX), true},
      {"input_tuner", "07A16", static_cast<uint8_t>(FrameType::STX), true},
      {"input_cdr", "07A19", static_cast<uint8_t>(FrameType::STX), true},
      {"input_md_tape", "07A18", static_cast<uint8_t>(FrameType::STX), true},
      {"input_md_tape_rx_v1600", "07A18", static_cast<uint8_t>(FrameType::STX), true},
      {"input_md_tape_x500", "07AC9", static_cast<uint8_t>(FrameType::STX), true},
      {"input_dvd", "07AC1", static_cast<uint8_t>(FrameType::STX), true},
      {"input_dtv", "07A54", static_cast<uint8_t>(FrameType::STX), true},
      {"input_dtv_ld", "07A54", static_cast<uint8_t>(FrameType::STX), true},
      {"input_dtv_cbl", "07A54", static_cast<uint8_t>(FrameType::STX), true},
      {"input_cbl_sat", "07AC0", static_cast<uint8_t>(FrameType::STX), true},
      {"input_sat", "07ACA", static_cast<uint8_t>(FrameType::STX), true},
      {"input_vcr", "07A0F", static_cast<uint8_t>(FrameType::STX), true},
      {"input_vcr1", "07A0F", static_cast<uint8_t>(FrameType::STX), true},
      {"input_vcr2_dvr", "07A13", static_cast<uint8_t>(FrameType::STX), true},
      {"input_dvr", "07A13", static_cast<uint8_t>(FrameType::STX), true},
      {"input_vcr3", "07AC8", static_cast<uint8_t>(FrameType::STX), true},
      {"input_vaux", "07A55", static_cast<uint8_t>(FrameType::STX), true},
      {"input_vaux_dock", "07A55", static_cast<uint8_t>(FrameType::STX), true},
      {"input_multi_ch", "07A87", static_cast<uint8_t>(FrameType::STX), true},
      {"input_xm", "07AB4", static_cast<uint8_t>(FrameType::STX), true},
      {"input_bd_hd_dvd", "07AC8", static_cast<uint8_t>(FrameType::STX), true},
      {"input_net_usb", "0F7F013FC0", static_cast<uint8_t>(FrameType::STX), true},

      {"zone2_input_phono", "07AD0", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_cd", "07AD1", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_tuner", "07AD2", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_md_tape", "07AD3", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_md_tape_x500", "07ACF", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_cdr", "07AD4", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_dvd", "07ACD", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_dtv", "07AD9", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_dtv_ld", "07AD9", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_dtv_cbl", "07AD9", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_cbl_sat", "07ACC", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_sat", "07ACB", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_vcr", "07AD6", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_vcr1", "07AD6", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_vcr2_dvr", "07AD7", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_dvr", "07AD7", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_vcr3", "07ACE", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_vaux", "07AD8", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_vaux_dock", "07AD8", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_xm", "07AB8", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_bd_hd_dvd", "07ACE", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_input_net_usb", "0F7F0140BF", static_cast<uint8_t>(FrameType::STX), true},

      {"zone3_input_phono", "07AF1", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_input_cd", "07AF2", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_input_tuner", "07AF3", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_input_md_tape", "07AF4", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_input_cdr", "07AF5", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_input_dtv", "07AF6", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_input_dtv_ld", "07AF6", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_input_cbl_sat", "07AF7", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_input_sat", "07AF8", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_input_vcr", "07AF9", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_input_vcr1", "07AF9", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_input_vcr2", "07AFA", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_input_dvr", "07AFB", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_input_dvd", "07AFC", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_input_vaux", "07AF0", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_input_vaux_dock", "07AF0", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_input_xm", "07AB9", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_input_bd_hd_dvd", "07AFB", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_input_net_usb", "0F7F0141BE", static_cast<uint8_t>(FrameType::STX), true},

      {"multi_ch_input_on", "07EA4", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_input_off", "07EA5", static_cast<uint8_t>(FrameType::STX), true},
      {"pure_direct_on", "07E80", static_cast<uint8_t>(FrameType::STX), true},
      {"pure_direct_off", "07E82", static_cast<uint8_t>(FrameType::STX), true},
      {"pure_direct_on_rx_v1600", "07E80", static_cast<uint8_t>(FrameType::STX), true},
      {"pure_direct_off_rx_v1600", "07E82", static_cast<uint8_t>(FrameType::STX), true},
      {"pure_direct_on_rx_v2500", "07E82", static_cast<uint8_t>(FrameType::STX), true},
      {"pure_direct_off_rx_v2500", "07E83", static_cast<uint8_t>(FrameType::STX), true},
      {"audio_select_auto", "07EA6", static_cast<uint8_t>(FrameType::STX), true},
      {"audio_select_dd_rf", "07EA7", static_cast<uint8_t>(FrameType::STX), true},
      {"audio_select_dts", "07EA8", static_cast<uint8_t>(FrameType::STX), true},
      {"audio_select_digital", "07EA9", static_cast<uint8_t>(FrameType::STX), true},
      {"audio_select_coax_opt", "07EA9", static_cast<uint8_t>(FrameType::STX), true},
      {"audio_select_analog", "07EAA", static_cast<uint8_t>(FrameType::STX), true},
      {"audio_select_hdmi", "07EDA", static_cast<uint8_t>(FrameType::STX), true},
      {"decoder_mode_auto", "07EDB", static_cast<uint8_t>(FrameType::STX), true},
      {"decoder_mode_dts", "07EA8", static_cast<uint8_t>(FrameType::STX), true},
      {"decoder_mode_aac", "07E3B", static_cast<uint8_t>(FrameType::STX), true},

      {"report_enable", "20000", static_cast<uint8_t>(FrameType::STX), true},
      {"report_disable", "20001", static_cast<uint8_t>(FrameType::STX), true},
      {"report_delay_realtime", "20100", static_cast<uint8_t>(FrameType::STX), true},
      {"report_delay_50ms", "20101", static_cast<uint8_t>(FrameType::STX), true},
      {"report_delay_100ms", "20102", static_cast<uint8_t>(FrameType::STX), true},
      {"report_delay_150ms", "20103", static_cast<uint8_t>(FrameType::STX), true},
      {"report_delay_200ms", "20104", static_cast<uint8_t>(FrameType::STX), true},
      {"report_delay_250ms", "20105", static_cast<uint8_t>(FrameType::STX), true},
      {"report_delay_300ms", "20106", static_cast<uint8_t>(FrameType::STX), true},
      {"report_delay_350ms", "20107", static_cast<uint8_t>(FrameType::STX), true},
      {"report_delay_400ms", "20108", static_cast<uint8_t>(FrameType::STX), true},
      {"osd_message_start", "21000", static_cast<uint8_t>(FrameType::STX), true},
      {"text_request_tuner_frequency", "22000", static_cast<uint8_t>(FrameType::STX), true},
      {"text_request_main_volume", "22001", static_cast<uint8_t>(FrameType::STX), true},
      {"text_request_zone2_volume", "22002", static_cast<uint8_t>(FrameType::STX), true},
      {"text_request_main_input", "22003", static_cast<uint8_t>(FrameType::STX), true},
      {"text_request_zone2_input", "22004", static_cast<uint8_t>(FrameType::STX), true},
      {"text_request_zone3_volume", "22005", static_cast<uint8_t>(FrameType::STX), true},
      {"text_request_zone3_input", "22006", static_cast<uint8_t>(FrameType::STX), true},
      {"firmware_version_request", "22F00", static_cast<uint8_t>(FrameType::STX), true},
      {"main_level_normal", "23300", static_cast<uint8_t>(FrameType::STX), true},
      {"mute_type_full", "23800", static_cast<uint8_t>(FrameType::STX), true},
      {"mute_type_20db", "23801", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_amplifier_ext", "23E00", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_amplifier_speaker1", "23E01", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_amplifier_speaker2", "23E02", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_amplifier_both", "23E03", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_amplifier_ext", "23F00", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_amplifier_speaker1", "23F01", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_amplifier_speaker2", "23F02", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_amplifier_both", "23F03", static_cast<uint8_t>(FrameType::STX), true},
      {"input_mode_auto", "26000", static_cast<uint8_t>(FrameType::STX), true},
      {"input_mode_last", "26001", static_cast<uint8_t>(FrameType::STX), true},
      {"audio_select_set_auto", "26000", static_cast<uint8_t>(FrameType::STX), true},
      {"audio_select_set_last", "26001", static_cast<uint8_t>(FrameType::STX), true},
      {"extended_surround_set_auto", "25C00", static_cast<uint8_t>(FrameType::STX), true},
      {"extended_surround_set_last", "25C01", static_cast<uint8_t>(FrameType::STX), true},
      {"adaptive_dsp_level_auto", "25D00", static_cast<uint8_t>(FrameType::STX), true},
      {"adaptive_dsp_level_off", "25D01", static_cast<uint8_t>(FrameType::STX), true},
      {"decoder_mode_set_auto", "25F00", static_cast<uint8_t>(FrameType::STX), true},
      {"decoder_mode_set_last", "25F01", static_cast<uint8_t>(FrameType::STX), true},
      {"dimmer_minus_4", "26100", static_cast<uint8_t>(FrameType::STX), true},
      {"dimmer_minus_3", "26101", static_cast<uint8_t>(FrameType::STX), true},
      {"dimmer_minus_2", "26102", static_cast<uint8_t>(FrameType::STX), true},
      {"dimmer_minus_1", "26103", static_cast<uint8_t>(FrameType::STX), true},
      {"dimmer_0", "26104", static_cast<uint8_t>(FrameType::STX), true},
      {"gray_back_off", "26300", static_cast<uint8_t>(FrameType::STX), true},
      {"gray_back_auto", "26301", static_cast<uint8_t>(FrameType::STX), true},
      {"dynamic_range_speaker_max", "26400", static_cast<uint8_t>(FrameType::STX), true},
      {"dynamic_range_speaker_std", "26401", static_cast<uint8_t>(FrameType::STX), true},
      {"dynamic_range_speaker_min", "26402", static_cast<uint8_t>(FrameType::STX), true},
      {"dynamic_range_headphone_max", "26500", static_cast<uint8_t>(FrameType::STX), true},
      {"dynamic_range_headphone_std", "26501", static_cast<uint8_t>(FrameType::STX), true},
      {"dynamic_range_headphone_min", "26502", static_cast<uint8_t>(FrameType::STX), true},
      {"wallpaper_type1", "25800", static_cast<uint8_t>(FrameType::STX), true},
      {"wallpaper_yes", "25800", static_cast<uint8_t>(FrameType::STX), true},
      {"wallpaper_type2", "25801", static_cast<uint8_t>(FrameType::STX), true},
      {"wallpaper_type3", "25802", static_cast<uint8_t>(FrameType::STX), true},
      {"wallpaper_gray_x500", "25803", static_cast<uint8_t>(FrameType::STX), true},
      {"wallpaper_none_x500", "25804", static_cast<uint8_t>(FrameType::STX), true},
      {"wallpaper_gray", "2580E", static_cast<uint8_t>(FrameType::STX), true},
      {"wallpaper_none", "2580F", static_cast<uint8_t>(FrameType::STX), true},
      {"hdmi_auto_audio_delay_off", "25600", static_cast<uint8_t>(FrameType::STX), true},
      {"hdmi_auto_audio_delay_on", "25601", static_cast<uint8_t>(FrameType::STX), true},
      {"dsp_3d_off", "25700", static_cast<uint8_t>(FrameType::STX), true},
      {"dsp_3d_on", "25701", static_cast<uint8_t>(FrameType::STX), true},
      {"hdmi_auto_lipsync_auto", "25900", static_cast<uint8_t>(FrameType::STX), true},
      {"hdmi_auto_lipsync_off", "25901", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_volume_out_variable", "26600", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_volume_out_fixed", "26601", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_mode_1", "26700", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_mode_2", "26701", static_cast<uint8_t>(FrameType::STX), true},
      {"fl_scroll_continue", "26700", static_cast<uint8_t>(FrameType::STX), true},
      {"fl_scroll_once", "26701", static_cast<uint8_t>(FrameType::STX), true},
      {"memory_guard_off", "26800", static_cast<uint8_t>(FrameType::STX), true},
      {"memory_guard_on", "26801", static_cast<uint8_t>(FrameType::STX), true},
      {"video_conversion_off", "26900", static_cast<uint8_t>(FrameType::STX), true},
      {"video_conversion_on", "26901", static_cast<uint8_t>(FrameType::STX), true},
      {"component_osd_off", "26A00", static_cast<uint8_t>(FrameType::STX), true},
      {"component_osd_on", "26A01", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_volume_out_variable", "26B00", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_volume_out_fixed", "26B01", static_cast<uint8_t>(FrameType::STX), true},
      {"zone_osd_off", "26C00", static_cast<uint8_t>(FrameType::STX), true},
      {"zone_osd_zone2", "26C01", static_cast<uint8_t>(FrameType::STX), true},
      {"zone_osd_zone2_zone3", "26C02", static_cast<uint8_t>(FrameType::STX), true},
      {"adaptive_drc_auto", "26D00", static_cast<uint8_t>(FrameType::STX), true},
      {"adaptive_drc_last", "26D01", static_cast<uint8_t>(FrameType::STX), true},
      {"language_english", "26F00", static_cast<uint8_t>(FrameType::STX), true},
      {"language_japanese", "26F01", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_bgv_off", "26F00", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_bgv_last", "26F01", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_bgv_dvd", "26F05", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_bgv_dtv", "26F06", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_bgv_dtv_cbl", "26F06", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_bgv_cbl_sat", "26F07", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_bgv_vcr", "26F09", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_bgv_vcr1", "26F09", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_bgv_vcr2_dvr", "26F0A", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_bgv_dvr", "26F0A", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_bgv_vaux", "26F0C", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_bgv_bd_hd_dvd", "26F0F", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_center_large", "27000", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_center_small", "27001", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_center_none", "27002", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_front_large", "27100", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_front_small", "27101", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_surround_large", "27200", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_surround_small", "27201", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_surround_none", "27202", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_surround_back_large_x2", "27300", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_surround_back_large_x1", "27301", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_surround_back_small_x2", "27302", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_surround_back_small_x1", "27303", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_surround_back_none", "27304", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_presence_yes", "27400", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_presence_none", "27401", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_bass_out_subwoofer", "27500", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_bass_out_front", "27501", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_bass_out_both", "27502", static_cast<uint8_t>(FrameType::STX), true},
      {"subwoofer_phase_normal", "27600", static_cast<uint8_t>(FrameType::STX), true},
      {"subwoofer_phase_reverse", "27610", static_cast<uint8_t>(FrameType::STX), true},
      {"subwoofer_config_reverse_x500", "27601", static_cast<uint8_t>(FrameType::STX), true},
      {"subwoofer_config_none_x500", "27602", static_cast<uint8_t>(FrameType::STX), true},
      {"subwoofer2_lr", "27700", static_cast<uint8_t>(FrameType::STX), true},
      {"subwoofer2_front_rear", "27701", static_cast<uint8_t>(FrameType::STX), true},
      {"subwoofer2_none", "27702", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_center_to_center", "27800", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_center_to_main", "27801", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_subwoofer_to_subwoofer", "27900", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_subwoofer_to_main", "27901", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_surround_to_surround", "27A00", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_surround_to_main", "27A01", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_select_6ch", "27B00", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_select_8ch_cd_x500", "27B01", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_select_8ch_cd", "27B02", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_select_8ch_cdr", "27B03", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_select_8ch_md_tape", "27B04", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_select_8ch_dvd", "27B05", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_select_8ch_dtv", "27B06", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_select_8ch_dtv_cbl", "27B06", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_select_8ch_cbl_sat", "27B07", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_select_8ch_vcr", "27B09", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_select_8ch_vcr1", "27B09", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_select_8ch_vcr2_dvr", "27B0A", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_select_8ch_dvr", "27B0A", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_select_8ch_vaux", "27B0C", static_cast<uint8_t>(FrameType::STX), true},
      {"multi_ch_select_8ch_bd_hd_dvd", "27B0F", static_cast<uint8_t>(FrameType::STX), true},
      {"pr_sb_priority_presence", "27D00", static_cast<uint8_t>(FrameType::STX), true},
      {"pr_sb_priority_surround_back", "27D01", static_cast<uint8_t>(FrameType::STX), true},
      {"subwoofer_crossover_40hz", "27E00", static_cast<uint8_t>(FrameType::STX), true},
      {"subwoofer_crossover_60hz", "27E01", static_cast<uint8_t>(FrameType::STX), true},
      {"subwoofer_crossover_80hz", "27E02", static_cast<uint8_t>(FrameType::STX), true},
      {"subwoofer_crossover_90hz", "27E03", static_cast<uint8_t>(FrameType::STX), true},
      {"subwoofer_crossover_100hz", "27E04", static_cast<uint8_t>(FrameType::STX), true},
      {"subwoofer_crossover_110hz", "27E05", static_cast<uint8_t>(FrameType::STX), true},
      {"subwoofer_crossover_120hz", "27E06", static_cast<uint8_t>(FrameType::STX), true},
      {"subwoofer_crossover_160hz", "27E07", static_cast<uint8_t>(FrameType::STX), true},
      {"subwoofer_crossover_200hz", "27E08", static_cast<uint8_t>(FrameType::STX), true},
      {"test_tone_off", "28000", static_cast<uint8_t>(FrameType::STX), true},
      {"test_tone_on", "28001", static_cast<uint8_t>(FrameType::STX), true},
      {"test_tone_dolby_x500", "28001", static_cast<uint8_t>(FrameType::STX), true},
      {"test_tone_dsp_x500", "28002", static_cast<uint8_t>(FrameType::STX), true},
      {"component_ip_off", "28500", static_cast<uint8_t>(FrameType::STX), true},
      {"component_ip_on", "28501", static_cast<uint8_t>(FrameType::STX), true},
      {"hdmi_ip_off", "28600", static_cast<uint8_t>(FrameType::STX), true},
      {"hdmi_ip_on", "28601", static_cast<uint8_t>(FrameType::STX), true},
      {"hdmi_upscaling_through", "28700", static_cast<uint8_t>(FrameType::STX), true},
      {"hdmi_upscaling_480p_576p", "28701", static_cast<uint8_t>(FrameType::STX), true},
      {"hdmi_upscaling_1080i", "28702", static_cast<uint8_t>(FrameType::STX), true},
      {"hdmi_upscaling_720p", "28703", static_cast<uint8_t>(FrameType::STX), true},
      {"hdmi_upscaling_1080p", "28704", static_cast<uint8_t>(FrameType::STX), true},
      {"hdmi_aspect_through", "28800", static_cast<uint8_t>(FrameType::STX), true},
      {"hdmi_aspect_16_9_normal", "28801", static_cast<uint8_t>(FrameType::STX), true},
      {"hdmi_aspect_smart_zoom", "28802", static_cast<uint8_t>(FrameType::STX), true},
      {"thx_sb_speaker_distance_under_1ft", "28A00", static_cast<uint8_t>(FrameType::STX), true},
      {"thx_sb_speaker_distance_1_4ft", "28A01", static_cast<uint8_t>(FrameType::STX), true},
      {"thx_sb_speaker_distance_over_4ft", "28A02", static_cast<uint8_t>(FrameType::STX), true},
      {"night_mode_extended_off", "28B00", static_cast<uint8_t>(FrameType::STX), true},
      {"night_mode_cinema_low", "28B10", static_cast<uint8_t>(FrameType::STX), true},
      {"night_mode_cinema_mid", "28B11", static_cast<uint8_t>(FrameType::STX), true},
      {"night_mode_cinema_high", "28B12", static_cast<uint8_t>(FrameType::STX), true},
      {"night_mode_music_low", "28B20", static_cast<uint8_t>(FrameType::STX), true},
      {"night_mode_music_mid", "28B21", static_cast<uint8_t>(FrameType::STX), true},
      {"night_mode_music_high", "28B22", static_cast<uint8_t>(FrameType::STX), true},
      {"hdmi_support_audio_receiver", "28F00", static_cast<uint8_t>(FrameType::STX), true},
      {"hdmi_support_audio_other", "28F01", static_cast<uint8_t>(FrameType::STX), true},
      {"on_screen_off", "29300", static_cast<uint8_t>(FrameType::STX), true},
      {"on_screen_10sec", "29301", static_cast<uint8_t>(FrameType::STX), true},
      {"on_screen_30sec", "29302", static_cast<uint8_t>(FrameType::STX), true},
      {"on_screen_always", "29303", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_display_release", "29500", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_display_hold", "29501", static_cast<uint8_t>(FrameType::STX), true},
      {"ipod_charge_standby_off", "29600", static_cast<uint8_t>(FrameType::STX), true},
      {"ipod_charge_standby_auto", "29601", static_cast<uint8_t>(FrameType::STX), true},
      {"ipod_repeat_off", "29700", static_cast<uint8_t>(FrameType::STX), true},
      {"ipod_repeat_one", "29701", static_cast<uint8_t>(FrameType::STX), true},
      {"ipod_repeat_all", "29702", static_cast<uint8_t>(FrameType::STX), true},
      {"ipod_shuffle_off", "29800", static_cast<uint8_t>(FrameType::STX), true},
      {"ipod_shuffle_songs", "29801", static_cast<uint8_t>(FrameType::STX), true},
      {"ipod_shuffle_albums", "29802", static_cast<uint8_t>(FrameType::STX), true},
      {"net_usb_repeat_off", "29900", static_cast<uint8_t>(FrameType::STX), true},
      {"net_usb_repeat_single", "29901", static_cast<uint8_t>(FrameType::STX), true},
      {"net_usb_repeat_all", "29902", static_cast<uint8_t>(FrameType::STX), true},
      {"net_usb_shuffle_off", "29A00", static_cast<uint8_t>(FrameType::STX), true},
      {"net_usb_shuffle_on", "29A01", static_cast<uint8_t>(FrameType::STX), true},
      {"tone_control_bypass_x500", "2A600", static_cast<uint8_t>(FrameType::STX), true},
      {"tone_control_on_x500", "2A601", static_cast<uint8_t>(FrameType::STX), true},
      {"eq_select_auto_peq", "2A700", static_cast<uint8_t>(FrameType::STX), true},
      {"eq_select_geq", "2A701", static_cast<uint8_t>(FrameType::STX), true},
      {"eq_select_off", "2A702", static_cast<uint8_t>(FrameType::STX), true},
      {"tone_control_auto_bypass_auto", "2A800", static_cast<uint8_t>(FrameType::STX), true},
      {"tone_control_auto_bypass_off", "2A801", static_cast<uint8_t>(FrameType::STX), true},
      {"advanced_setup_off_x500", "2B000", static_cast<uint8_t>(FrameType::STX), true},
      {"advanced_setup_on_x500", "2B001", static_cast<uint8_t>(FrameType::STX), true},
      {"remote_control_id_1_x500", "2B100", static_cast<uint8_t>(FrameType::STX), true},
      {"remote_control_id_2_x500", "2B101", static_cast<uint8_t>(FrameType::STX), true},
      {"fan_control_auto_x500", "2B200", static_cast<uint8_t>(FrameType::STX), true},
      {"fan_control_continuous_x500", "2B201", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_impedance_8ohm_x500", "2B300", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_impedance_6ohm_x500", "2B301", static_cast<uint8_t>(FrameType::STX), true},
      {"tuner_setup_0_x500", "2B400", static_cast<uint8_t>(FrameType::STX), true},
      {"tuner_setup_1_x500", "2B401", static_cast<uint8_t>(FrameType::STX), true},

      {"ext_read_system_zone2", "200500000", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_system_tuner", "200500001", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_system_video", "200500002", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_system_effect_channels", "200500003", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_system_digital_format", "200500004", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_system_dc_trigger", "200500005", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_system_preset", "200500006", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_system_zone3", "200500007", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_system_ilink", "200500008", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_system_hdmi", "200500009", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_system_xm", "20050000A", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_model_name", "20050000F", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_input_speaker_out", "200500100", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_input_audio_analog", "200500101", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_input_audio_optical", "200500102", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_input_audio_coaxial", "200500103", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_input_audio_ddrf", "200500104", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_input_video_composite_s", "200500105", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_input_video_component", "200500106", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_input_audio_output_analog", "200500107", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_input_audio_output_optical", "200500108", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_input_video_output_composite_s", "200500109", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_dsp_program", "200500200", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_dsp_user_parameters", "200500201", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_dsp_maker_parameters", "200500202", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_xm_antenna", "200500300", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_xm_current_channel_name", "2006003100", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_xm_history1_channel_name", "2006003101", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_xm_history2_channel_name", "2006003102", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_xm_current_category_name", "2006003200", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_xm_history1_category_name", "2006003201", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_xm_history2_category_name", "2006003202", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_xm_current_artist_name", "2006003300", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_xm_history1_artist_name", "2006003301", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_xm_history2_artist_name", "2006003302", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_xm_current_song_name", "2006003400", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_xm_history1_song_name", "2006003401", static_cast<uint8_t>(FrameType::DC4), true},
      {"ext_read_xm_history2_song_name", "2006003402", static_cast<uint8_t>(FrameType::DC4), true},

      {"osd_off", "07EB0", static_cast<uint8_t>(FrameType::STX), true},
      {"osd_short", "07EB1", static_cast<uint8_t>(FrameType::STX), true},
      {"osd_on", "07EB1", static_cast<uint8_t>(FrameType::STX), true},
      {"osd_full", "07EB2", static_cast<uint8_t>(FrameType::STX), true},
      {"short_message_off", "07EB0", static_cast<uint8_t>(FrameType::STX), true},
      {"short_message_on", "07EB1", static_cast<uint8_t>(FrameType::STX), true},
      {"short_message_full", "07EB2", static_cast<uint8_t>(FrameType::STX), true},
      {"sleep_off", "07EB3", static_cast<uint8_t>(FrameType::STX), true},
      {"sleep_120", "07EB4", static_cast<uint8_t>(FrameType::STX), true},
      {"sleep_90", "07EB5", static_cast<uint8_t>(FrameType::STX), true},
      {"sleep_60", "07EB6", static_cast<uint8_t>(FrameType::STX), true},
      {"sleep_30", "07EB7", static_cast<uint8_t>(FrameType::STX), true},
      {"extended_surround_ex_es", "07EB8", static_cast<uint8_t>(FrameType::STX), true},
      {"extended_surround_off", "07EB9", static_cast<uint8_t>(FrameType::STX), true},
      {"extended_surround_auto", "07E7C", static_cast<uint8_t>(FrameType::STX), true},
      {"extended_surround_discrete", "07E7D", static_cast<uint8_t>(FrameType::STX), true},
      {"extended_surround_ex", "07EDC", static_cast<uint8_t>(FrameType::STX), true},
      {"extended_surround_pl2x_movie", "07EDD", static_cast<uint8_t>(FrameType::STX), true},
      {"extended_surround_pl2x_music", "07EDE", static_cast<uint8_t>(FrameType::STX), true},
      {"night_mode_off", "07E9C", static_cast<uint8_t>(FrameType::STX), true},
      {"night_mode_cinema", "07E9B", static_cast<uint8_t>(FrameType::STX), true},
      {"night_mode_music", "07ECF", static_cast<uint8_t>(FrameType::STX), true},
      {"effect_on", "07E27", static_cast<uint8_t>(FrameType::STX), true},
      {"straight", "07EE0", static_cast<uint8_t>(FrameType::STX), true},
      {"program_straight", "07EE0", static_cast<uint8_t>(FrameType::STX), true},
      {"program_hall_a", "07EE1", static_cast<uint8_t>(FrameType::STX), true},
      {"program_munich", "07EE1", static_cast<uint8_t>(FrameType::STX), true},
      {"program_hall_b", "07EE2", static_cast<uint8_t>(FrameType::STX), true},
      {"program_hall_c", "07EE3", static_cast<uint8_t>(FrameType::STX), true},
      {"program_hall_usa", "07EE4", static_cast<uint8_t>(FrameType::STX), true},
      {"program_hall_e", "07EE5", static_cast<uint8_t>(FrameType::STX), true},
      {"program_vienna", "07EE5", static_cast<uint8_t>(FrameType::STX), true},
      {"program_live_concert", "07EE6", static_cast<uint8_t>(FrameType::STX), true},
      {"program_amsterdam", "07EE6", static_cast<uint8_t>(FrameType::STX), true},
      {"program_tokyo", "07EE7", static_cast<uint8_t>(FrameType::STX), true},
      {"program_freiburg", "07EE8", static_cast<uint8_t>(FrameType::STX), true},
      {"program_royaumont", "07EE9", static_cast<uint8_t>(FrameType::STX), true},
      {"program_village_gate", "07EEA", static_cast<uint8_t>(FrameType::STX), true},
      {"program_village_vanguard", "07EEB", static_cast<uint8_t>(FrameType::STX), true},
      {"program_the_bottom_line", "07EEC", static_cast<uint8_t>(FrameType::STX), true},
      {"program_the_roxy_theatre", "07EED", static_cast<uint8_t>(FrameType::STX), true},
      {"program_the_roxy_theater", "07EED", static_cast<uint8_t>(FrameType::STX), true},
      {"program_warehouse_loft", "07EEE", static_cast<uint8_t>(FrameType::STX), true},
      {"program_arena", "07EEF", static_cast<uint8_t>(FrameType::STX), true},
      {"program_chamber", "07EAF", static_cast<uint8_t>(FrameType::STX), true},
      {"program_cellar_club", "07ECD", static_cast<uint8_t>(FrameType::STX), true},
      {"program_disco", "07EF0", static_cast<uint8_t>(FrameType::STX), true},
      {"program_party", "07EF1", static_cast<uint8_t>(FrameType::STX), true},
      {"program_game", "07EF2", static_cast<uint8_t>(FrameType::STX), true},
      {"program_action_game", "07EF2", static_cast<uint8_t>(FrameType::STX), true},
      {"program_7ch_stereo", "07EFF", static_cast<uint8_t>(FrameType::STX), true},
      {"program_2ch_stereo", "07EC0", static_cast<uint8_t>(FrameType::STX), true},
      {"program_2ch_direct_stereo", "07EC1", static_cast<uint8_t>(FrameType::STX), true},
      {"program_pop_rock", "07EF3", static_cast<uint8_t>(FrameType::STX), true},
      {"program_music_video", "07EF3", static_cast<uint8_t>(FrameType::STX), true},
      {"program_dj", "07EF4", static_cast<uint8_t>(FrameType::STX), true},
      {"program_classical_opera", "07EF5", static_cast<uint8_t>(FrameType::STX), true},
      {"program_recital_opera", "07EF5", static_cast<uint8_t>(FrameType::STX), true},
      {"program_pavillion", "07EF6", static_cast<uint8_t>(FrameType::STX), true},
      {"program_mono_movie", "07EF7", static_cast<uint8_t>(FrameType::STX), true},
      {"program_variety_sports", "07EF8", static_cast<uint8_t>(FrameType::STX), true},
      {"program_sports", "07EF8", static_cast<uint8_t>(FrameType::STX), true},
      {"program_spectacle", "07EF9", static_cast<uint8_t>(FrameType::STX), true},
      {"program_sci_fi", "07EFA", static_cast<uint8_t>(FrameType::STX), true},
      {"program_adventure", "07EFB", static_cast<uint8_t>(FrameType::STX), true},
      {"program_general", "07EFC", static_cast<uint8_t>(FrameType::STX), true},
      {"program_drama", "07EFC", static_cast<uint8_t>(FrameType::STX), true},
      {"program_normal", "07EFD", static_cast<uint8_t>(FrameType::STX), true},
      {"program_surround_decode", "07EFD", static_cast<uint8_t>(FrameType::STX), true},
      {"program_enhanced", "07EFE", static_cast<uint8_t>(FrameType::STX), true},
      {"program_standard", "07EFE", static_cast<uint8_t>(FrameType::STX), true},
      {"program_thx_cinema", "07EC2", static_cast<uint8_t>(FrameType::STX), true},
      {"program_thx_ultra2_cinema", "07EC2", static_cast<uint8_t>(FrameType::STX), true},
      {"program_thx_music", "07EC3", static_cast<uint8_t>(FrameType::STX), true},
      {"program_plii_game", "07EC7", static_cast<uint8_t>(FrameType::STX), true},
      {"program_thx_ultra2_cinema_pl2", "07EC7", static_cast<uint8_t>(FrameType::STX), true},
      {"program_thx_game", "07EC8", static_cast<uint8_t>(FrameType::STX), true},
      {"program_thx_ultra2_cinema_neo6", "07EC8", static_cast<uint8_t>(FrameType::STX), true},
      {"program_roleplaying_game", "07ECE", static_cast<uint8_t>(FrameType::STX), true},
      {"music_enhancer_on", "07ED8", static_cast<uint8_t>(FrameType::STX), true},
      {"music_enhancer_off", "07ED9", static_cast<uint8_t>(FrameType::STX), true},
      {"enhancer_7ch", "0F7E81146B", static_cast<uint8_t>(FrameType::STX), true},

      {"tuner_preset_page_a", "07AE0", static_cast<uint8_t>(FrameType::STX), true},
      {"tuner_preset_page_b", "07AE1", static_cast<uint8_t>(FrameType::STX), true},
      {"tuner_preset_page_c", "07AE2", static_cast<uint8_t>(FrameType::STX), true},
      {"tuner_preset_page_d", "07AE3", static_cast<uint8_t>(FrameType::STX), true},
      {"tuner_preset_page_e", "07AE4", static_cast<uint8_t>(FrameType::STX), true},
      {"tuner_preset_1", "07AE5", static_cast<uint8_t>(FrameType::STX), true},
      {"tuner_preset_2", "07AE6", static_cast<uint8_t>(FrameType::STX), true},
      {"tuner_preset_3", "07AE7", static_cast<uint8_t>(FrameType::STX), true},
      {"tuner_preset_4", "07AE8", static_cast<uint8_t>(FrameType::STX), true},
      {"tuner_preset_5", "07AE9", static_cast<uint8_t>(FrameType::STX), true},
      {"tuner_preset_6", "07AEA", static_cast<uint8_t>(FrameType::STX), true},
      {"tuner_preset_7", "07AEB", static_cast<uint8_t>(FrameType::STX), true},
      {"tuner_preset_8", "07AEC", static_cast<uint8_t>(FrameType::STX), true},
      {"tuner_band_fm", "07EBC", static_cast<uint8_t>(FrameType::STX), true},
      {"tuner_band_am", "07EBD", static_cast<uint8_t>(FrameType::STX), true},
      {"tuner_auto_up", "07EBE", static_cast<uint8_t>(FrameType::STX), true},
      {"tuner_auto_down", "07EBF", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_a_on", "07EAB", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_a_off", "07EAC", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_b_on", "07EAD", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_b_off", "07EAE", static_cast<uint8_t>(FrameType::STX), true},

      {"system_memory_save_1", "07E2B", static_cast<uint8_t>(FrameType::STX), true},
      {"system_memory_save_2", "07E2C", static_cast<uint8_t>(FrameType::STX), true},
      {"system_memory_save_3", "07E2D", static_cast<uint8_t>(FrameType::STX), true},
      {"system_memory_save_4", "07E2E", static_cast<uint8_t>(FrameType::STX), true},
      {"system_memory_save_5", "07E2F", static_cast<uint8_t>(FrameType::STX), true},
      {"system_memory_save_6", "07E30", static_cast<uint8_t>(FrameType::STX), true},
      {"system_memory_load_1", "07E35", static_cast<uint8_t>(FrameType::STX), true},
      {"system_memory_load_2", "07E36", static_cast<uint8_t>(FrameType::STX), true},
      {"system_memory_load_3", "07E37", static_cast<uint8_t>(FrameType::STX), true},
      {"system_memory_load_4", "07E38", static_cast<uint8_t>(FrameType::STX), true},
      {"system_memory_load_5", "07E39", static_cast<uint8_t>(FrameType::STX), true},
      {"system_memory_load_6", "07E3A", static_cast<uint8_t>(FrameType::STX), true},
      {"home_preset_memory_a", "07E2B", static_cast<uint8_t>(FrameType::STX), true},
      {"home_preset_memory_b", "07E2C", static_cast<uint8_t>(FrameType::STX), true},
      {"home_preset_memory_c", "07E2D", static_cast<uint8_t>(FrameType::STX), true},
      {"home_preset_memory_d", "07E2E", static_cast<uint8_t>(FrameType::STX), true},
      {"home_preset_memory_e", "07E2F", static_cast<uint8_t>(FrameType::STX), true},
      {"home_preset_memory_f", "07E20", static_cast<uint8_t>(FrameType::STX), true},
      {"home_preset_recall_a", "07E35", static_cast<uint8_t>(FrameType::STX), true},
      {"home_preset_recall_b", "07E36", static_cast<uint8_t>(FrameType::STX), true},
      {"home_preset_recall_c", "07E37", static_cast<uint8_t>(FrameType::STX), true},
      {"home_preset_recall_d", "07E38", static_cast<uint8_t>(FrameType::STX), true},
      {"home_preset_recall_e", "07E39", static_cast<uint8_t>(FrameType::STX), true},
      {"home_preset_recall_f", "07E3A", static_cast<uint8_t>(FrameType::STX), true},
      {"home_volume_memory_save_a", "07E6B", static_cast<uint8_t>(FrameType::STX), true},
      {"home_volume_memory_save_b", "07E6C", static_cast<uint8_t>(FrameType::STX), true},
      {"home_volume_memory_save_c", "07E6D", static_cast<uint8_t>(FrameType::STX), true},
      {"home_volume_memory_save_d", "07E6E", static_cast<uint8_t>(FrameType::STX), true},
      {"home_volume_memory_save_e", "07E6F", static_cast<uint8_t>(FrameType::STX), true},
      {"home_volume_memory_save_f", "07E60", static_cast<uint8_t>(FrameType::STX), true},
      {"home_volume_memory_load_a", "07E75", static_cast<uint8_t>(FrameType::STX), true},
      {"home_volume_memory_load_b", "07E76", static_cast<uint8_t>(FrameType::STX), true},
      {"home_volume_memory_load_c", "07E77", static_cast<uint8_t>(FrameType::STX), true},
      {"home_volume_memory_load_d", "07E78", static_cast<uint8_t>(FrameType::STX), true},
      {"home_volume_memory_load_e", "07E79", static_cast<uint8_t>(FrameType::STX), true},
      {"home_volume_memory_load_f", "07E7A", static_cast<uint8_t>(FrameType::STX), true},
      {"main_volume_memory_save_1", "07E6B", static_cast<uint8_t>(FrameType::STX), true},
      {"main_volume_memory_save_2", "07E6C", static_cast<uint8_t>(FrameType::STX), true},
      {"main_volume_memory_save_3", "07E6D", static_cast<uint8_t>(FrameType::STX), true},
      {"main_volume_memory_save_4", "07E6E", static_cast<uint8_t>(FrameType::STX), true},
      {"main_volume_memory_save_5", "07E6F", static_cast<uint8_t>(FrameType::STX), true},
      {"main_volume_memory_save_6", "07E70", static_cast<uint8_t>(FrameType::STX), true},
      {"main_volume_memory_load_1", "07E75", static_cast<uint8_t>(FrameType::STX), true},
      {"main_volume_memory_load_2", "07E76", static_cast<uint8_t>(FrameType::STX), true},
      {"main_volume_memory_load_3", "07E77", static_cast<uint8_t>(FrameType::STX), true},
      {"main_volume_memory_load_4", "07E78", static_cast<uint8_t>(FrameType::STX), true},
      {"main_volume_memory_load_5", "07E79", static_cast<uint8_t>(FrameType::STX), true},
      {"main_volume_memory_load_6", "07E7A", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_volume_memory_save_1", "07E87", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_volume_memory_save_2", "07E88", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_volume_memory_save_3", "07E89", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_volume_memory_save_4", "07E8A", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_volume_memory_save_5", "07E8B", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_volume_memory_save_6", "07E8C", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_volume_memory_load_1", "07E8D", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_volume_memory_load_2", "07E8E", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_volume_memory_load_3", "07E8F", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_volume_memory_load_4", "07E90", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_volume_memory_load_5", "07E91", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_volume_memory_load_6", "07E92", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_volume_memory_save_1", "07E20", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_volume_memory_save_2", "07E21", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_volume_memory_save_3", "07E22", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_volume_memory_save_4", "07E23", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_volume_memory_save_5", "07E24", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_volume_memory_save_6", "07E25", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_volume_memory_load_1", "07E60", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_volume_memory_load_2", "07E61", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_volume_memory_load_3", "07E62", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_volume_memory_load_4", "07E63", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_volume_memory_load_5", "07E64", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_volume_memory_load_6", "07E65", static_cast<uint8_t>(FrameType::STX), true},

      {"zone2_balance_left", "07ED4", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_balance_right", "07ED5", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_balance_left", "07ED6", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_balance_right", "07ED7", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger1_control_main", "07E32", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger1_control_zone2", "07E33", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger1_control_zone3", "07E31", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger1_control_all", "07E34", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger1_main_high", "07E73", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger1_main_low", "07E74", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger1_zone2_high", "07E71", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger1_zone2_low", "07E72", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger1_zone3_high", "07E83", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger1_zone3_low", "07E84", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger2_control_main", "07E96", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger2_control_zone2", "07E97", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger2_control_zone3", "07E9F", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger2_control_all", "07E98", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger2_main_high", "07E3E", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger2_main_low", "07E3F", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger2_zone2_high", "07E3C", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger2_zone2_low", "07E3D", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger2_zone3_high", "07E85", static_cast<uint8_t>(FrameType::STX), true},
      {"trigger2_zone3_low", "07E86", static_cast<uint8_t>(FrameType::STX), true},
      {"dual_mono_main", "07E93", static_cast<uint8_t>(FrameType::STX), true},
      {"dual_mono_sub", "07E94", static_cast<uint8_t>(FrameType::STX), true},
      {"dual_mono_all", "07E95", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_b_assignment_main", "07E28", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_b_assignment_zone_b", "07E29", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_b_assignment_zone1", "07E28", static_cast<uint8_t>(FrameType::STX), true},
      {"speaker_b_assignment_zone2", "07E29", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_amp_internal", "07E99", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_amp_external", "07E9A", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_speaker_out_on", "07E99", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_speaker_out_off", "07E9A", static_cast<uint8_t>(FrameType::STX), true},
      {"decoder_2ch_pl2x_movie", "07E67", static_cast<uint8_t>(FrameType::STX), true},
      {"decoder_2ch_pl2x_music", "07E68", static_cast<uint8_t>(FrameType::STX), true},
      {"decoder_2ch_neo6_cinema", "07E69", static_cast<uint8_t>(FrameType::STX), true},
      {"decoder_2ch_neo6_music", "07E6A", static_cast<uint8_t>(FrameType::STX), true},
      {"decoder_2ch_pl2x_game", "07EC7", static_cast<uint8_t>(FrameType::STX), true},
      {"decoder_2ch_pro_logic", "07EC9", static_cast<uint8_t>(FrameType::STX), true},
      {"decoder_2ch_cs2_cinema", "07ECA", static_cast<uint8_t>(FrameType::STX), true},
      {"decoder_2ch_cs2_music", "07ECB", static_cast<uint8_t>(FrameType::STX), true},
      {"decoder_2ch_neural_surround", "07ECC", static_cast<uint8_t>(FrameType::STX), true},

      {"xm_digit_0", "07A60", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_digit_1", "07A61", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_digit_2", "07A62", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_digit_3", "07A63", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_digit_4", "07A64", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_digit_5", "07A65", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_digit_6", "07A66", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_digit_7", "07A67", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_digit_8", "07A68", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_digit_9", "07A69", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_preset_up", "07A6A", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_preset_down", "07A6B", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_category_up", "07A6C", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_category_down", "07A6E", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_display_hold_toggle", "07A6F", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_history_next", "07A70", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_preset_memory", "07A71", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_search_all_channel", "07AB5", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_search_category", "07AB6", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_search_preset", "07AB7", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_preset_page_a", "07ABA", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_preset_page_b", "07ABB", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_preset_page_c", "07ABC", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_preset_page_d", "07ABD", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_preset_page_e", "07ABE", static_cast<uint8_t>(FrameType::STX), true},
      {"xm_enter", "07ABF", static_cast<uint8_t>(FrameType::STX), true},

      {"zone2_bass_up", "07A73", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_bass_down", "07A74", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_treble_up", "07A75", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_treble_down", "07A76", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_bass_up", "07A77", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_bass_down", "07A78", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_treble_up", "07A79", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_treble_down", "07A7A", static_cast<uint8_t>(FrameType::STX), true},
      {"gui_top_menu", "07AA0", static_cast<uint8_t>(FrameType::STX), false},
      {"gui_enter", "07ADE", static_cast<uint8_t>(FrameType::STX), false},
      {"gui_exit", "07AA1", static_cast<uint8_t>(FrameType::STX), false},
      {"gui_cursor_up", "07A9D", static_cast<uint8_t>(FrameType::STX), false},
      {"gui_cursor_down", "07A9C", static_cast<uint8_t>(FrameType::STX), false},
      {"gui_cursor_right", "07A9E", static_cast<uint8_t>(FrameType::STX), false},
      {"gui_cursor_left", "07A9F", static_cast<uint8_t>(FrameType::STX), false},
      {"main_source_display_on", "07ED0", static_cast<uint8_t>(FrameType::STX), false},
      {"main_source_display_off", "07ED1", static_cast<uint8_t>(FrameType::STX), false},
      {"zone2_source_display_on", "07ED2", static_cast<uint8_t>(FrameType::STX), false},
      {"zone2_source_display_off", "07ED3", static_cast<uint8_t>(FrameType::STX), false},

      {"ipod_menu", "0F7F010FF0", static_cast<uint8_t>(FrameType::STX), false},
      {"ipod_enter", "0F7F0111EE", static_cast<uint8_t>(FrameType::STX), false},
      {"ipod_display", "0F7F0115EA", static_cast<uint8_t>(FrameType::STX), false},
      {"ipod_cursor_up", "0F7F010EF1", static_cast<uint8_t>(FrameType::STX), false},
      {"ipod_cursor_down", "0F7F0114EB", static_cast<uint8_t>(FrameType::STX), false},
      {"ipod_cursor_right", "0F7F0112ED", static_cast<uint8_t>(FrameType::STX), false},
      {"ipod_cursor_left", "0F7F0110EF", static_cast<uint8_t>(FrameType::STX), false},
      {"ipod_play", "0F7F011EE1", static_cast<uint8_t>(FrameType::STX), false},
      {"ipod_stop", "0F7F011DE2", static_cast<uint8_t>(FrameType::STX), false},
      {"ipod_pause", "0F7F011AE5", static_cast<uint8_t>(FrameType::STX), false},
      {"ipod_skip_next", "0F7F011CE3", static_cast<uint8_t>(FrameType::STX), false},
      {"ipod_skip_previous", "0F7F011BE4", static_cast<uint8_t>(FrameType::STX), false},
      {"net_usb_input", "0F7F013FC0", static_cast<uint8_t>(FrameType::STX), true},
      {"zone2_net_usb_input", "0F7F0140BF", static_cast<uint8_t>(FrameType::STX), true},
      {"zone3_net_usb_input", "0F7F0141BE", static_cast<uint8_t>(FrameType::STX), true},
      {"net_usb_pc_mcx", "0F7F0136C9", static_cast<uint8_t>(FrameType::STX), true},
      {"net_usb_net_radio", "0F7F0137C8", static_cast<uint8_t>(FrameType::STX), true},
      {"net_usb_usb", "0F7F0138C7", static_cast<uint8_t>(FrameType::STX), true},
      {"net_usb_menu", "0F7F012FD0", static_cast<uint8_t>(FrameType::STX), false},
      {"net_usb_enter", "0F7F0131CE", static_cast<uint8_t>(FrameType::STX), false},
      {"net_usb_display", "0F7F0135CA", static_cast<uint8_t>(FrameType::STX), false},
      {"net_usb_cursor_up", "0F7F012ED1", static_cast<uint8_t>(FrameType::STX), false},
      {"net_usb_cursor_down", "0F7F0134CB", static_cast<uint8_t>(FrameType::STX), false},
      {"net_usb_cursor_right", "0F7F0132CD", static_cast<uint8_t>(FrameType::STX), false},
      {"net_usb_cursor_left", "0F7F0130CF", static_cast<uint8_t>(FrameType::STX), false},
      {"net_usb_play", "0F7F013EC1", static_cast<uint8_t>(FrameType::STX), false},
      {"net_usb_stop", "0F7F013DC2", static_cast<uint8_t>(FrameType::STX), false},
      {"net_usb_skip_next", "0F7F013CC3", static_cast<uint8_t>(FrameType::STX), false},
      {"net_usb_skip_previous", "0F7F013BC4", static_cast<uint8_t>(FrameType::STX), false},
      {"system_memory_play", "0F7A85007F", static_cast<uint8_t>(FrameType::STX), true},
      {"system_memory_stop", "0F7A85037C", static_cast<uint8_t>(FrameType::STX), true},
      {"system_memory_skip_next", "0F7A850679", static_cast<uint8_t>(FrameType::STX), true},
      {"system_memory_skip_previous", "0F7A850976", static_cast<uint8_t>(FrameType::STX), true},
  };

  const std::string key = normalize_token_(alias);
  if (key == "receiver_reset" || key == "reset") {
    this->command_receiver_reset();
    return true;
  }
  for (size_t i = 0; i < sizeof(COMMAND_ALIASES) / sizeof(COMMAND_ALIASES[0]); i++) {
#ifdef ARDUINO_ARCH_ESP8266
    CommandAlias entry;
    memcpy_P(&entry, &COMMAND_ALIASES[i], sizeof(entry));
#else
    const CommandAlias &entry = COMMAND_ALIASES[i];
#endif
    if (key == entry.alias) {
      this->queue_simple_command_(static_cast<FrameType>(entry.frame_type), entry.payload, "alias", entry.expect_response);
      return true;
    }
  }
  return false;
}

std::string YamahaSerialComponent::resolve_profile_() const {
  const std::string configured_profile = normalize_token_(this->configured_profile_);
  if (configured_profile == "rx_vx500") {
    return "rx_vx500";
  }
  if (configured_profile == "rx_vx500_extended") {
    return "rx_vx500_extended";
  }
  if (configured_profile == "rx_vx600") {
    return "rx_vx600";
  }
  if (configured_profile == "rx_vx600_extended") {
    return "rx_vx600_extended";
  }
  if (configured_profile == "rx_vx700") {
    return "rx_vx700";
  }
  if (configured_profile == "rx_vx700_extended") {
    return "rx_vx700_extended";
  }
  if (configured_profile == "rx_vx800") {
    return "rx_vx800";
  }
  if (configured_profile == "rx_vx800_extended") {
    return "rx_vx800_extended";
  }

  const std::string detected_model = normalize_token_(this->receiver_model_);
  const std::string configured_model = normalize_token_(this->configured_model_);
  const std::string model = (!detected_model.empty() && detected_model != "unknown") ? detected_model : configured_model;
  if (model.find("vx800") != std::string::npos || model.find("v1800") != std::string::npos ||
      model.find("v3800") != std::string::npos) {
    return "rx_vx800_extended";
  }
  if (model.find("vx700") != std::string::npos || model.find("v1700") != std::string::npos ||
      model.find("v2700") != std::string::npos) {
    return "rx_vx700_extended";
  }
  if (model.find("vx600") != std::string::npos || model.find("v1600") != std::string::npos ||
      model.find("v2600") != std::string::npos) {
    return "rx_vx600_extended";
  }
  if (model.find("vx500") != std::string::npos || model.find("v1500") != std::string::npos ||
      model.find("v2500") != std::string::npos) {
    return "rx_vx500_extended";
  }
  return "rx_vx500_extended";
}

void YamahaSerialComponent::clear_profile_mappings_() {
  this->inputs_.clear();
  this->programs_.clear();
  this->scenes_.clear();
}

void YamahaSerialComponent::load_input_mappings_(const StaticInputMapping *mappings, size_t count) {
  this->inputs_.clear();
  this->inputs_.reserve(count);
  for (size_t i = 0; i < count; i++) {
#ifdef ARDUINO_ARCH_ESP8266
    StaticInputMapping mapping;
    memcpy_P(&mapping, &mappings[i], sizeof(mapping));
#else
    const auto &mapping = mappings[i];
#endif
    this->inputs_.push_back(
        {mapping.key, mapping.label, mapping.report_id, mapping.zone2_report_id, mapping.main_command, mapping.zone2_command});
  }
}

void YamahaSerialComponent::load_program_mappings_(const StaticProgramMapping *mappings, size_t count) {
  this->programs_.clear();
  this->programs_.reserve(count);
  for (size_t i = 0; i < count; i++) {
#ifdef ARDUINO_ARCH_ESP8266
    StaticProgramMapping mapping;
    memcpy_P(&mapping, &mappings[i], sizeof(mapping));
#else
    const auto &mapping = mappings[i];
#endif
    this->programs_.push_back({mapping.label, mapping.report_code, mapping.command});
  }
}

void YamahaSerialComponent::load_scene_mappings_(const StaticSceneMapping *mappings, size_t count) {
  this->scenes_.clear();
  this->scenes_.reserve(count);
  for (size_t i = 0; i < count; i++) {
#ifdef ARDUINO_ARCH_ESP8266
    StaticSceneMapping mapping;
    memcpy_P(&mapping, &mappings[i], sizeof(mapping));
#else
    const auto &mapping = mappings[i];
#endif
    this->scenes_.push_back({mapping.label, mapping.command});
  }
}

void YamahaSerialComponent::apply_input_overrides_() {
  for (const auto &override : this->input_overrides_) {
    if (override.key.empty() || override.label.empty()) {
      continue;
    }

    bool found = false;
    for (auto &mapping : this->inputs_) {
      if (mapping.key != override.key) {
        continue;
      }
      mapping.label = override.label;
      found = true;
      break;
    }

    if (!found) {
      this->inputs_.push_back({override.key, override.label, 0xFF, 0xFF, "", ""});
    }
  }
}

void YamahaSerialComponent::apply_profile_rx_vx500_() {
  this->profile_caps_.supports_zone2 = true;
  this->profile_caps_.supports_scene = true;
  this->profile_caps_.supports_audio_select = true;
  this->profile_caps_.supports_night_mode = true;
  this->profile_caps_.supports_fan_mode = false;
  this->profile_caps_.supports_pure_direct = true;
  this->profile_caps_.supports_extended = false;
  this->profile_caps_.decode_dc2_layout = false;

  static const StaticInputMapping INPUTS[] YAMAHA_SERIAL_FLASH_TABLE = {
      {"phono", "PHONO", 0x0, 0x0, "07A14", "07AD0"},
      {"cd", "CD", 0x1, 0x1, "07A15", "07AD1"},
      {"tuner", "Tuner", 0x2, 0x2, "07A16", "07AD2"},
      {"cdr", "CD-R", 0x3, 0x3, "07A19", "07AD4"},
      {"md_tape", "MD/TAPE", 0x4, 0x4, "07AC9", "07ACF"},
      {"dvd", "DVD", 0x5, 0x5, "07AC1", "07ACD"},
      {"dtv_ld", "D-TV/LD", 0x6, 0x6, "07A54", "07AD9"},
      {"cbl_sat", "CBL/SAT", 0x7, 0x7, "07AC0", "07ACC"},
      {"sat", "SAT", 0x8, 0x8, "07ACA", "07ACB"},
      {"vcr1", "VCR1", 0x9, 0x9, "07A0F", "07AD6"},
      {"vcr2_dvr", "VCR2/DVR", 0xA, 0xA, "07A13", "07AD7"},
      {"vcr3", "VCR3", 0xB, 0xB, "07AC8", "07ACE"},
      {"vaux", "V-AUX", 0xC, 0xC, "07A55", "07AD8"},
  };
  this->load_input_mappings_(INPUTS, sizeof(INPUTS) / sizeof(INPUTS[0]));

  static const StaticProgramMapping PROGRAMS[] YAMAHA_SERIAL_FLASH_TABLE = {
      {"STRAIGHT", 0x80, "07EE0"},       {"Hall A", 0x00, "07EE1"},
      {"Hall B", 0x01, "07EE2"},         {"Hall C", 0x02, "07EE3"},
      {"Hall USA", 0x04, "07EE4"},       {"Vienna", 0x05, "07EE5"},
      {"Live Concert", 0x06, "07EE6"},   {"Tokyo", 0x08, "07EE7"},
      {"Freiburg", 0x09, "07EE8"},       {"Royaumont", 0x0A, "07EE9"},
      {"Village Gate", 0x0B, "07EEA"},   {"Village Vanguard", 0x0C, "07EEB"},
      {"Bottom Line", 0x0E, "07EEC"},    {"Roxy Theater", 0x10, "07EED"},
      {"Warehouse Loft", 0x12, "07EEE"}, {"Arena", 0x13, "07EEF"},
      {"Disco", 0x14, "07EF0"},          {"Party", 0x15, "07EF1"},
      {"Game", 0x16, "07EF2"},           {"7ch Stereo", 0x17, "07EFF"},
      {"Music Pop", 0x18, "07EF3"},      {"DJ", 0x19, "07EF4"},
      {"Classical/Opera", 0x1A, "07EF5"}, {"Pavillion", 0x1D, "07EF6"},
      {"Mono Movie", 0x20, "07EF7"},     {"Sports", 0x21, "07EF8"},
      {"Spectacle", 0x24, "07EF9"},      {"Sci-Fi", 0x25, "07EFA"},
      {"Adventure", 0x28, "07EFB"},      {"General", 0x29, "07EFC"},
      {"ProLogic", 0x2C, "07EFD"},       {"Standard", 0x2D, "07EFE"},
      {"PLII Movie", 0x30, "07E67"},     {"PLII Music", 0x31, "07E68"},
      {"Neo:6 Cinema", 0x32, "07E69"},   {"Neo:6 Music", 0x33, "07E6A"},
      {"2ch Stereo", 0x34, "07EC0"},     {"2ch Direct Stereo", 0x35, "07EC1"},
      {"THX Cinema", 0x36, "07EC2"},     {"THX Music", 0x37, "07EC3"},
      {"PLII Game", 0x38, "07EC7"},      {"THX Game", 0x3C, "07EC8"},
  };
  this->load_program_mappings_(PROGRAMS, sizeof(PROGRAMS) / sizeof(PROGRAMS[0]));

  static const StaticSceneMapping SCENES[] YAMAHA_SERIAL_FLASH_TABLE = {
      {"A", "07E35"},
      {"B", "07E36"},
      {"C", "07E37"},
      {"D", "07E38"},
      {"E", "07E39"},
      {"F", "07E3A"},
  };
  this->load_scene_mappings_(SCENES, sizeof(SCENES) / sizeof(SCENES[0]));
}

void YamahaSerialComponent::apply_profile_rx_vx600_extended_() {
  this->apply_profile_rx_vx600_();
  this->profile_caps_.supports_fan_mode = true;
  this->profile_caps_.supports_extended = true;
  this->profile_caps_.decode_dc2_layout = true;
}

void YamahaSerialComponent::apply_profile_rx_vx500_extended_() {
  this->apply_profile_rx_vx500_();
}

void YamahaSerialComponent::apply_profile_rx_vx600_() {
  this->apply_profile_rx_vx500_();
  this->profile_caps_.supports_zone2 = true;
  this->profile_caps_.supports_scene = true;
  this->profile_caps_.supports_audio_select = true;
  this->profile_caps_.supports_night_mode = true;
  this->profile_caps_.supports_fan_mode = false;
  this->profile_caps_.supports_pure_direct = true;
  this->profile_caps_.supports_extended = false;
  this->profile_caps_.decode_dc2_layout = false;

  for (auto &input : this->inputs_) {
    if (input.key == "md_tape") {
      input.main_command = "07A18";
      break;
    }
  }
}

void YamahaSerialComponent::apply_profile_rx_vx700_() {
  this->profile_caps_.supports_zone2 = true;
  this->profile_caps_.supports_scene = true;
  this->profile_caps_.supports_audio_select = true;
  this->profile_caps_.supports_night_mode = true;
  this->profile_caps_.supports_fan_mode = false;
  this->profile_caps_.supports_pure_direct = true;
  this->profile_caps_.supports_extended = false;
  this->profile_caps_.decode_dc2_layout = false;

  static const StaticInputMapping INPUTS[] YAMAHA_SERIAL_FLASH_TABLE = {
      {"phono", "PHONO", 0x00, 0x00, "07A14", "07AD0"},
      {"cd", "CD", 0x01, 0x01, "07A15", "07AD1"},
      {"tuner", "Tuner", 0x02, 0x02, "07A16", "07AD2"},
      {"cdr", "CD-R", 0x03, 0x03, "07A19", "07AD4"},
      {"md_tape", "MD/TAPE", 0x04, 0x04, "07A18", "07AD3"},
      {"dvd", "DVD", 0x05, 0x05, "07AC1", "07ACD"},
      {"dtv", "DTV", 0x06, 0x06, "07A54", "07AD9"},
      {"cbl_sat", "CBL/SAT", 0x07, 0x07, "07AC0", "07ACC"},
      {"vcr1", "VCR1", 0x09, 0x09, "07A0F", "07AD6"},
      {"dvr_vcr2", "DVR/VCR2", 0x0A, 0x0A, "07A13", "07AD7"},
      {"vcr3_dvr", "VCR3/DVR", 0x0B, 0x0B, "", ""},
      {"vaux_dock", "V-AUX/DOCK", 0x0C, 0x0C, "07A55", "07AD8"},
      {"net_usb", "NET/USB", 0x0D, 0x0D, "0F7F013FC0", "0F7F0140BF"},
      {"xm", "XM", 0x0E, 0x0E, "07AB4", "07AB8"},
      {"multi_ch", "Multi CH", 0x10, 0xFF, "07A87", ""},
  };
  this->load_input_mappings_(INPUTS, sizeof(INPUTS) / sizeof(INPUTS[0]));

  static const StaticProgramMapping PROGRAMS[] YAMAHA_SERIAL_FLASH_TABLE = {
      {"STRAIGHT", 0x80, "07EE0"},         {"Munich", 0x00, "07EE1"},
      {"Vienna", 0x05, "07EE5"},          {"Amsterdam", 0x07, "07EE6"},
      {"Freiburg", 0x09, "07EE8"},        {"Chamber", 0x0B, "07EAF"},
      {"Village Vanguard", 0x0D, "07EEB"}, {"Warehouse Loft", 0x11, "07EEE"},
      {"Cellar Club", 0x0F, "07ECD"},     {"The Bottom Line", 0x0E, "07EEC"},
      {"The Roxy Theatre", 0x10, "07EED"}, {"Disco", 0x14, "07EF0"},
      {"Game", 0x16, "07EF2"},            {"7ch Stereo", 0x17, "07EFF"},
      {"2ch Stereo", 0x34, "07EC0"},      {"Sports", 0x21, "07EF8"},
      {"Action Game", 0x1E, "07EF2"},     {"Roleplaying Game", 0x1F, "07ECE"},
      {"Music Video", 0x18, "07EF3"},     {"Recital/Opera", 0x1C, "07EF5"},
      {"Standard", 0x2D, "07EFE"},        {"Spectacle", 0x24, "07EF9"},
      {"Sci-Fi", 0x25, "07EFA"},          {"Adventure", 0x28, "07EFB"},
      {"Drama", 0x29, "07EFC"},           {"Mono Movie", 0x20, "07EF7"},
      {"Surround Decode", 0x2C, "07EFD"}, {"THX Cinema", 0x36, "07EC2"},
      {"THX Music", 0x37, "07EC3"},       {"THX Game", 0x3C, "07EC8"},
  };
  this->load_program_mappings_(PROGRAMS, sizeof(PROGRAMS) / sizeof(PROGRAMS[0]));

  static const StaticSceneMapping SCENES[] YAMAHA_SERIAL_FLASH_TABLE = {
      {"A", "07E35"}, {"B", "07E36"}, {"C", "07E37"}, {"D", "07E38"}, {"E", "07E39"}, {"F", "07E3A"},
  };
  this->load_scene_mappings_(SCENES, sizeof(SCENES) / sizeof(SCENES[0]));
}

void YamahaSerialComponent::apply_profile_rx_vx700_extended_() {
  this->apply_profile_rx_vx700_();
  this->profile_caps_.supports_extended = true;
}

void YamahaSerialComponent::apply_profile_rx_vx800_() {
  this->profile_caps_.supports_zone2 = true;
  this->profile_caps_.supports_scene = true;
  this->profile_caps_.supports_audio_select = true;
  this->profile_caps_.supports_night_mode = false;
  this->profile_caps_.supports_fan_mode = false;
  this->profile_caps_.supports_pure_direct = true;
  this->profile_caps_.supports_extended = false;
  this->profile_caps_.decode_dc2_layout = false;

  static const StaticInputMapping INPUTS[] YAMAHA_SERIAL_FLASH_TABLE = {
      {"phono", "PHONO", 0x00, 0x00, "07A14", "07AD0"},
      {"cd", "CD", 0x01, 0x01, "07A15", "07AD1"},
      {"tuner", "Tuner", 0x02, 0x02, "07A16", "07AD2"},
      {"cdr", "CD-R", 0x03, 0x03, "07A19", "07AD4"},
      {"md_tape", "MD/TAPE", 0x04, 0x04, "07A18", "07AD3"},
      {"dvd", "DVD", 0x05, 0x05, "07AC1", "07ACD"},
      {"dtv_cbl", "DTV/CBL", 0x06, 0x06, "07A54", "07AD9"},
      {"cbl_sat", "CBL/SAT", 0x07, 0x07, "07AC0", "07ACC"},
      {"sat", "SAT", 0x08, 0xFF, "", ""},
      {"vcr", "VCR", 0x09, 0x09, "07A0F", "07AD6"},
      {"dvr", "DVR", 0x0A, 0x0A, "07A13", "07AD7"},
      {"vcr3_dvr", "VCR3/DVR", 0x0B, 0x0B, "", ""},
      {"vaux_dock", "V-AUX/DOCK", 0x0C, 0x0C, "07A55", "07AD8"},
      {"net_usb", "NET/USB", 0x0D, 0x0D, "0F7F013FC0", "0F7F0140BF"},
      {"xm", "XM", 0x0E, 0x0E, "07AB4", "07AB8"},
      {"bd_hd_dvd", "BD/HD DVD", 0x0F, 0x0F, "07AC8", "07ACE"},
      {"multi_ch", "Multi CH", 0x10, 0xFF, "07A87", ""},
  };
  this->load_input_mappings_(INPUTS, sizeof(INPUTS) / sizeof(INPUTS[0]));

  static const StaticProgramMapping PROGRAMS[] YAMAHA_SERIAL_FLASH_TABLE = {
      {"STRAIGHT", 0x80, "07EE0"},         {"Munich", 0x00, "07EE1"},
      {"Vienna", 0x05, "07EE5"},          {"Amsterdam", 0x07, "07EE6"},
      {"Freiburg", 0x09, "07EE8"},        {"Chamber", 0x0B, "07EAF"},
      {"Village Vanguard", 0x0D, "07EEB"}, {"Warehouse Loft", 0x11, "07EEE"},
      {"Cellar Club", 0x0F, "07ECD"},     {"The Bottom Line", 0x0E, "07EEC"},
      {"The Roxy Theatre", 0x10, "07EED"}, {"Disco", 0x14, "07EF0"},
      {"Game", 0x16, "07EF2"},            {"7ch Stereo", 0x17, "07EFF"},
      {"2ch Stereo", 0x34, "07EC0"},      {"Sports", 0x21, "07EF8"},
      {"Action Game", 0x1E, "07EF2"},     {"Roleplaying Game", 0x1F, "07ECE"},
      {"Music Video", 0x18, "07EF3"},     {"Recital/Opera", 0x1C, "07EF5"},
      {"Standard", 0x2D, "07EFE"},        {"Spectacle", 0x24, "07EF9"},
      {"Sci-Fi", 0x25, "07EFA"},          {"Adventure", 0x28, "07EFB"},
      {"Drama", 0x29, "07EFC"},           {"Mono Movie", 0x20, "07EF7"},
      {"Surround Decode", 0x2C, "07EFD"}, {"THX Cinema", 0x36, "07EC2"},
      {"THX Music", 0x37, "07EC3"},       {"THX Game", 0x3C, "07EC8"},
      {"Straight Enhancer", 0x40, "07ED8"},
  };
  this->load_program_mappings_(PROGRAMS, sizeof(PROGRAMS) / sizeof(PROGRAMS[0]));

  static const StaticSceneMapping SCENES[] YAMAHA_SERIAL_FLASH_TABLE = {
      {"A", "07E35"}, {"B", "07E36"}, {"C", "07E37"}, {"D", "07E38"}, {"E", "07E39"}, {"F", "07E3A"},
  };
  this->load_scene_mappings_(SCENES, sizeof(SCENES) / sizeof(SCENES[0]));
}

void YamahaSerialComponent::apply_profile_rx_vx800_extended_() {
  this->apply_profile_rx_vx800_();
  this->profile_caps_.supports_extended = true;
}

void YamahaSerialComponent::apply_profile_(const std::string &profile_id) {
  this->clear_profile_mappings_();
  if (profile_id == "rx_vx500") {
    this->apply_profile_rx_vx500_();
  } else if (profile_id == "rx_vx500_extended") {
    this->apply_profile_rx_vx500_extended_();
  } else if (profile_id == "rx_vx600") {
    this->apply_profile_rx_vx600_();
  } else if (profile_id == "rx_vx600_extended") {
    this->apply_profile_rx_vx600_extended_();
  } else if (profile_id == "rx_vx700") {
    this->apply_profile_rx_vx700_();
  } else if (profile_id == "rx_vx700_extended") {
    this->apply_profile_rx_vx700_extended_();
  } else if (profile_id == "rx_vx800") {
    this->apply_profile_rx_vx800_();
  } else if (profile_id == "rx_vx800_extended") {
    this->apply_profile_rx_vx800_extended_();
  } else {
    this->apply_profile_rx_vx500_extended_();
  }
  this->active_profile_ = profile_id;
}

void YamahaSerialComponent::setup_default_mappings_() {
  if (this->profile_initialized_) {
    return;
  }

  const std::string profile_id = this->resolve_profile_();
  this->apply_profile_(profile_id);
  this->apply_input_overrides_();
  this->profile_initialized_ = true;
  this->configure_input_select_options_();
  this->configure_program_select_options_();
}

bool YamahaSerialComponent::is_auto_profile_() const { return normalize_token_(this->configured_profile_) == "auto"; }

void YamahaSerialComponent::handle_detected_model_(const std::string &model) {
  const std::string normalized_model = normalize_model_name_(model);
  if (normalized_model.empty()) {
    return;
  }

  if (normalized_model != this->receiver_model_) {
    this->receiver_model_ = normalized_model;
    if (this->receiver_model_text_sensor_ != nullptr) {
      this->receiver_model_text_sensor_->publish_state(this->receiver_model_);
    }
  }

  if (!this->profile_initialized_ || !this->is_auto_profile_()) {
    return;
  }

  const std::string detected_profile = this->resolve_profile_();
  if (detected_profile == this->active_profile_) {
    return;
  }

  ESP_LOGI(TAG, "Receiver model %s selects profile %s", this->receiver_model_.c_str(), detected_profile.c_str());
  this->apply_profile_(detected_profile);
  this->apply_input_overrides_();
  this->configure_input_select_options_();
  this->configure_program_select_options_();
}

void YamahaSerialComponent::configure_input_select_options_() {
  if (this->input_source_select_ == nullptr && this->zone2_input_source_select_ == nullptr) {
    return;
  }
  std::vector<std::string> labels{};
  labels.reserve(this->inputs_.size());
  for (const auto &input : this->inputs_) {
    labels.push_back(input.label);
  }

  if (this->input_source_select_ != nullptr) {
    this->input_source_select_->set_options_from_labels(labels);
  }
  if (this->zone2_input_source_select_ != nullptr) {
    this->zone2_input_source_select_->set_options_from_labels(labels);
  }
}

void YamahaSerialComponent::configure_program_select_options_() {
  if (this->program_select_ == nullptr) {
    return;
  }

  std::vector<std::string> labels{};
  labels.reserve(this->programs_.size());
  for (const auto &program : this->programs_) {
    labels.push_back(program.label);
  }
  this->program_select_->set_options_from_labels(labels);
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

void YamahaSerialComponent::queue_command_(const QueuedCommand &command) {
  if (is_coalescible_tag_(command.tag)) {
    if (this->in_flight_.has_value() && std::strcmp(this->in_flight_->command.tag, command.tag) == 0) {
      return;
    }
    for (const auto &queued : this->queue_) {
      if (std::strcmp(queued.tag, command.tag) == 0) {
        return;
      }
    }
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

  this->queue_.push_back(command);
}

void YamahaSerialComponent::queue_simple_command_(FrameType type, const std::string &payload, const char *tag,
                                                   bool expect_response) {
  if (payload.empty()) {
    return;
  }
  QueuedCommand command{};
  command.frame_type = type;
  command.payload = payload;
  command.tag = tag;
  command.expect_response = expect_response;
  command.timeout_ms = this->command_timeout_ms_;
  command.retries_left = this->max_retries_;
  this->queue_command_(command);
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
    } else if (now - in_flight.sent_at_ms > in_flight.command.timeout_ms) {
      if (in_flight.command.retries_left > 0) {
        in_flight.command.retries_left--;
        in_flight.attempts++;
        ESP_LOGW(TAG, "Command timeout for '%s', retry %u", in_flight.command.tag, in_flight.attempts);
        if (this->send_command_(in_flight.command)) {
          in_flight.sent_at_ms = now;
          this->last_tx_ms_ = now;
        }
        return;
      }
      this->timeouts_++;
      this->set_last_error_(std::string("Timeout waiting for response to '") + in_flight.command.tag + "'");
      this->in_flight_.reset();
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

  QueuedCommand command = this->queue_.front();
  this->queue_.pop_front();

  if (!this->send_command_(command)) {
    this->set_last_error_(std::string("Failed to send command '") + command.tag + "'");
    return;
  }
  this->last_tx_ms_ = now;

  if (std::strcmp(command.tag, "power_on") == 0 || std::strcmp(command.tag, "main_power_on") == 0 ||
      std::strcmp(command.tag, "zone2_power_on") == 0) {
    this->command_hold_until_ms_ = now + this->power_on_delay_ms_;
  }

  InFlightCommand in_flight{};
  in_flight.command = command;
  in_flight.sent_at_ms = now;
  in_flight.attempts = 1;
  this->in_flight_ = in_flight;
}

bool YamahaSerialComponent::send_command_(const QueuedCommand &command) {
  uint8_t packet[164];
  size_t packet_len = 0;
  const size_t expected_len = command.payload.size() + (command.frame_type == FrameType::DC4 ? 4 : 2);
  if (expected_len > sizeof(packet)) {
    this->set_last_error_("TX payload too long");
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
  this->commands_sent_++;
  ESP_LOGD(TAG, "TX [%s]: type=0x%02X payload=%s", command.tag, static_cast<uint8_t>(command.frame_type),
           command.payload.c_str());
  return true;
}

void YamahaSerialComponent::process_incoming_byte_(uint8_t byte) {
  const uint32_t now = millis();
  if (this->in_frame_ && now - this->frame_started_ms_ > FRAME_READ_TIMEOUT_MS) {
    this->record_parse_error_("UART frame timeout", this->frame_buffer_);
    this->in_frame_ = false;
    this->frame_buffer_.clear();
  }

  if (is_frame_start_(byte)) {
    if (this->in_frame_ && !this->frame_buffer_.empty()) {
      this->record_parse_error_("UART frame interrupted by new frame start", this->frame_buffer_);
    }
    this->in_frame_ = true;
    this->frame_started_ms_ = now;
    this->frame_buffer_.clear();
    this->frame_buffer_.push_back(byte);
    return;
  }

  if (!this->in_frame_) {
    return;
  }

  this->frame_buffer_.push_back(byte);
  if (this->frame_buffer_.size() > BUFFER_MAX_LEN) {
    this->record_parse_error_("UART frame overflow", this->frame_buffer_);
    this->in_frame_ = false;
    this->frame_buffer_.clear();
    return;
  }

  if (byte == static_cast<uint8_t>(FrameType::ETX)) {
    this->in_frame_ = false;
    this->parse_frame_(this->frame_buffer_);
    this->frame_buffer_.clear();
  }
}

void YamahaSerialComponent::parse_frame_(const std::vector<uint8_t> &frame) {
  if (frame.size() < 3) {
    this->record_parse_error_("Frame shorter than minimum length", frame);
    return;
  }

  this->responses_received_++;
  this->last_response_ms_ = millis();
  this->available_ = true;
  if (this->availability_binary_sensor_ != nullptr) {
    this->availability_binary_sensor_->publish_state(true);
  }
  this->publish_connection_state_("online");
  this->clear_last_error_();

  if (!this->receiver_reported_) {
    this->receiver_reported_ = true;
    this->queue_bootstrap_();
    this->queue_poll_commands_();
    ESP_LOGI(TAG, "Receiver responded; startup sync queued");
  }

  if (this->in_flight_.has_value() && this->in_flight_->command.expect_response) {
    this->in_flight_.reset();
  }

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
      this->record_parse_error_("Unknown frame type", frame);
      break;
  }
}

void YamahaSerialComponent::parse_stx_frame_(const std::vector<uint8_t> &frame) {
  if (frame.size() < 8) {
    this->record_parse_error_("STX frame shorter than Yamaha status frame", frame);
    return;
  }

  const int8_t cmd_hi = hex_nibble_(frame[3]);
  const int8_t cmd_lo = hex_nibble_(frame[4]);
  const int8_t data_hi = hex_nibble_(frame[5]);
  const int8_t data_lo = hex_nibble_(frame[6]);
  const int8_t grd = hex_nibble_(frame[2]);

  if (cmd_hi < 0 || cmd_lo < 0 || data_hi < 0 || data_lo < 0 || grd < 0) {
    this->record_parse_error_("STX frame contains non-hex status fields", frame);
    return;
  }

  if (grd > 0) {
    this->set_last_error_("Guarded command response (GRD=" + std::to_string(grd) + ")");
  }

  const uint8_t cmd = static_cast<uint8_t>((cmd_hi << 4) | cmd_lo);
  const uint8_t data = static_cast<uint8_t>((data_hi << 4) | data_lo);

  switch (cmd) {
    case 0x10:
      this->publish_playback_format_by_report_(static_cast<uint8_t>(data_lo));
      break;
    case 0x11:
      this->publish_sampling_rate_by_report_(static_cast<uint8_t>(data_lo));
      break;
    case 0x20: {
      bool main_on = this->main_power_on_;
      bool zone2_on = this->zone2_power_on_;
      switch (data) {
        case 0x00:
          main_on = false;
          zone2_on = false;
          break;
        case 0x01:
          main_on = true;
          zone2_on = true;
          break;
        case 0x02:
          main_on = true;
          zone2_on = false;
          break;
        case 0x03:
          main_on = false;
          zone2_on = true;
          break;
        case 0x04:
          main_on = true;
          zone2_on = true;
          break;
        case 0x05:
          main_on = true;
          zone2_on = false;
          break;
        case 0x06:
          main_on = false;
          zone2_on = true;
          break;
        case 0x07:
          main_on = false;
          zone2_on = false;
          break;
        default:
          main_on = (data & 0x02) != 0 || data == 0x01;
          zone2_on = (data & 0x01) != 0;
          break;
      }
      this->publish_power_state_(main_on);
      this->publish_zone2_power_state_(zone2_on);
      break;
    }
    case 0x21:
      this->publish_input_by_report_(data, false);
      break;
    case 0x22:
      this->publish_audio_select_by_report_(static_cast<uint8_t>(data_lo));
      break;
    case 0x23:
      this->publish_mute_state_(data == 0x01);
      break;
    case 0x24:
      this->publish_input_by_report_(data, true);
      break;
    case 0x25:
      this->publish_zone2_mute_state_(data == 0x01);
      break;
    case 0x26:
      this->publish_volume_db_(volume_raw_to_db_(data), false);
      break;
    case 0x27:
      this->publish_volume_db_(volume_raw_to_db_(data), true);
      break;
    case 0x28:
      this->publish_program_by_report_(data);
      break;
    case 0x29:
      this->publish_tuner_preset_page_by_report_(data);
      break;
    case 0x2A:
      this->publish_tuner_preset_by_report_(data);
      break;
    case 0x2C:
      this->publish_sleep_timer_by_report_(data);
      break;
    case 0x2D:
      this->publish_extended_surround_by_report_(data);
      break;
    case 0x2E:
      this->publish_speaker_relay_state_('A', data == 0x01);
      break;
    case 0x2F:
      this->publish_speaker_relay_state_('B', data == 0x01);
      break;
    case 0x35:
      this->publish_tuner_band_by_report_(data);
      break;
    case 0x3D:
      this->publish_speaker_b_assignment_by_report_(data);
      break;
    case 0x3E:
      this->publish_zone2_amp_by_report_(data);
      break;
    case 0x61:
      this->publish_dimmer_percent_(static_cast<uint8_t>(data_lo));
      break;
    case 0x5E:
      this->publish_decoder_mode_by_report_(static_cast<uint8_t>(data & 0xF0));
      break;
    case 0x8B:
      this->publish_night_mode_by_report_(static_cast<uint8_t>(data_hi), static_cast<uint8_t>(data_lo));
      break;
    case 0x8C:
      this->pure_direct_on_ = (data == 0x01);
      if (this->pure_direct_switch_ != nullptr) {
        this->pure_direct_switch_->publish_state(this->pure_direct_on_);
      }
      break;
    case 0xB2:
      this->fan_mode_on_ = (data == 0x01);
      if (this->fan_mode_switch_ != nullptr) {
        this->fan_mode_switch_->publish_state(this->fan_mode_on_);
      }
      break;
    case 0x3F:
      this->publish_zone2_power_state_(data == 0x01);
      break;
    default:
      break;
  }
}

void YamahaSerialComponent::parse_dc1_frame_(const std::vector<uint8_t> &frame) {
  if (frame.size() < 5) {
    this->record_parse_error_("DC1 text frame shorter than minimum length", frame);
    return;
  }

  const char rcmd0 = static_cast<char>(frame[1]);
  const char rcmd1 = static_cast<char>(frame[2]);
  std::string text(frame.begin() + 3, frame.end() - 1);
  text = trim_(text);

  if (rcmd0 == '0' && rcmd1 == '1') {
    this->main_volume_text_ = text;
    if (this->main_volume_text_sensor_ != nullptr) {
      this->main_volume_text_sensor_->publish_state(text);
    }
    if (auto volume = parse_volume_text_db_(text); volume.has_value()) {
      this->publish_volume_db_(volume.value(), false);
    }
  } else if (rcmd0 == '0' && rcmd1 == '2') {
    if (auto volume = parse_volume_text_db_(text); volume.has_value()) {
      this->publish_volume_db_(volume.value(), true);
    }
  } else if (rcmd0 == '0' && rcmd1 == '3') {
    this->publish_input_label_(text, false);
  } else if (rcmd0 == '0' && rcmd1 == '4') {
    this->publish_input_label_(text, true);
  }
}

void YamahaSerialComponent::parse_dc2_frame_(const std::vector<uint8_t> &frame) {
  if (frame.size() < 7) {
    this->record_parse_error_("DC2 system-status frame shorter than model field", frame);
    return;
  }
  std::string model(frame.begin() + 1, frame.begin() + 6);
  this->handle_detected_model_(model);

  // RX-V1600/RX-Vx600 extended layout decode.
  if (this->profile_caps_.decode_dc2_layout && frame.size() > 136) {
    const int8_t power = hex_nibble_(frame[17]);
    if (power >= 0) {
      this->publish_power_state_(power != 0);
    }

    const int8_t input = hex_nibble_(frame[18]);
    if (input >= 0) {
      this->publish_input_by_report_(static_cast<uint8_t>(input), false);
    }

    const int8_t audio_select = hex_nibble_(frame[20]);
    if (audio_select >= 0) {
      this->publish_audio_select_by_report_(static_cast<uint8_t>(audio_select));
    }

    const int8_t mute = hex_nibble_(frame[21]);
    if (mute >= 0) {
      this->publish_mute_state_(mute == 1);
    }

    const int8_t vol_hi = hex_nibble_(frame[24]);
    const int8_t vol_lo = hex_nibble_(frame[25]);
    if (vol_hi >= 0 && vol_lo >= 0) {
      const uint8_t raw = static_cast<uint8_t>((vol_hi << 4) | vol_lo);
      this->publish_volume_db_(volume_raw_to_db_(raw), false);
    }

    const int8_t program_hi = hex_nibble_(frame[28]);
    const int8_t program_lo = hex_nibble_(frame[29]);
    if (program_hi >= 0 && program_lo >= 0) {
      this->publish_program_by_report_(static_cast<uint8_t>((program_hi << 4) | program_lo));
    }

    const int8_t night_hi = hex_nibble_(frame[36]);
    const int8_t night_lo = hex_nibble_(frame[37]);
    if (night_hi >= 0 && night_lo >= 0) {
      this->publish_night_mode_by_report_(static_cast<uint8_t>(night_hi), static_cast<uint8_t>(night_lo));
    }

    const int8_t dimmer = hex_nibble_(frame[94]);
    if (dimmer >= 0) {
      this->publish_dimmer_percent_(static_cast<uint8_t>(dimmer));
    }

    const int8_t fan = hex_nibble_(frame[132]);
    if (fan >= 0) {
      this->fan_mode_on_ = (fan == 1);
      if (this->fan_mode_switch_ != nullptr) {
        this->fan_mode_switch_->publish_state(this->fan_mode_on_);
      }
    }

    const int8_t pure = hex_nibble_(frame[135]);
    if (pure >= 0) {
      this->pure_direct_on_ = (pure == 1);
      if (this->pure_direct_switch_ != nullptr) {
        this->pure_direct_switch_->publish_state(this->pure_direct_on_);
      }
    }
  }

  this->publish_connection_state_("configured");
}

void YamahaSerialComponent::parse_dc4_frame_(const std::vector<uint8_t> &frame) {
  if (frame.size() < 8) {
    this->record_parse_error_("DC4 extended frame shorter than minimum length", frame);
    return;
  }

  const std::string raw(frame.begin() + 1, frame.end() - 1);
  if (raw.size() < 4) {
    this->record_parse_error_("DC4 extended payload shorter than checksum length", frame);
    return;
  }

  const std::string body = raw.substr(0, raw.size() - 2);
  const std::string rx_sum = raw.substr(raw.size() - 2);
  const uint8_t expected_sum = checksum_8bit_(body);
  const char expected_hi = static_cast<char>(nibble_to_hex_(expected_sum >> 4));
  const char expected_lo = static_cast<char>(nibble_to_hex_(expected_sum & 0x0F));
  if (rx_sum.size() != 2 || rx_sum[0] != expected_hi || rx_sum[1] != expected_lo) {
    this->record_parse_error_("DC4 checksum mismatch", frame);
    return;
  }

  this->parse_dc4_extended_payload_(body, frame);
}

void YamahaSerialComponent::parse_dc4_extended_payload_(const std::string &payload, const std::vector<uint8_t> &frame) {
  if (payload.size() < 2 || payload[0] != '2' || payload[1] != '0') {
    ESP_LOGD(TAG, "Ignoring unsupported DC4 payload: %s", payload.c_str());
    return;
  }

  if (payload.size() < 8) {
    this->record_parse_error_("Extended payload shorter than minimum length", frame);
    return;
  }

  const int8_t len_hi = hex_nibble_(payload[2]);
  const int8_t len_lo = hex_nibble_(payload[3]);
  if (len_hi < 0 || len_lo < 0) {
    this->record_parse_error_("Extended payload length contains non-hex data", frame);
    return;
  }

  const size_t data_len = static_cast<size_t>((len_hi << 4) | len_lo);
  if (payload.size() < 4 + data_len) {
    this->record_parse_error_("Extended payload shorter than declared length", frame);
    return;
  }

  const std::string data = payload.substr(4, data_len);
  if (data.size() < 4) {
    this->record_parse_error_("Extended command data shorter than status field", frame);
    return;
  }

  const std::string command_id = data.substr(0, 3);
  const char status = data[3];
  if (status != '0') {
    this->set_last_error_("Extended command status error: " + std::string(1, status) + " for " + command_id);
    return;
  }

  const std::string command_data = data.size() > 4 ? data.substr(4) : "";
  if (command_id == "000") {
    if (!command_data.empty() && command_data[0] == 'F') {
      const std::string model = trim_(command_data.substr(1));
      this->handle_detected_model_(model);
    }
    return;
  }
  if (command_id == "033") {
    this->parse_tone_control_(command_data);
    return;
  }
  if (command_id == "041") {
    this->parse_speaker_distance_(command_data);
    return;
  }
  if (command_id == "050") {
    this->parse_tuner_station_(command_data);
    return;
  }
}

void YamahaSerialComponent::parse_tone_control_(const std::string &payload) {
  if (payload.size() < 5) {
    this->record_parse_error_("Tone-control payload shorter than minimum length");
    return;
  }

  const int8_t sp_hp = hex_nibble_(static_cast<uint8_t>(payload[0]));
  const int8_t bass_treble = hex_nibble_(static_cast<uint8_t>(payload[1]));
  const int8_t gain_hi = hex_nibble_(static_cast<uint8_t>(payload[3]));
  const int8_t gain_lo = hex_nibble_(static_cast<uint8_t>(payload[4]));
  if (sp_hp < 0 || bass_treble < 0 || gain_hi < 0 || gain_lo < 0) {
    this->record_parse_error_("Tone-control payload contains non-hex data");
    return;
  }

  if (sp_hp != 0) {
    return;
  }

  const uint8_t gain_raw = static_cast<uint8_t>((gain_hi << 4) | gain_lo);
  const float percent = static_cast<float>(gain_raw) * 4.1666667f;
  if (bass_treble == 0) {
    this->publish_bass_percent_(percent);
  } else if (bass_treble == 1) {
    this->publish_treble_percent_(percent);
  }
}

void YamahaSerialComponent::parse_speaker_distance_(const std::string &payload) {
  if (payload.size() < 5) {
    this->record_parse_error_("Speaker-distance payload shorter than minimum length");
    return;
  }

  const int8_t channel = hex_nibble_(static_cast<uint8_t>(payload[0]));
  const int8_t unit = hex_nibble_(static_cast<uint8_t>(payload[1]));
  const int8_t d_hi = hex_nibble_(static_cast<uint8_t>(payload[2]));
  const int8_t d_mid = hex_nibble_(static_cast<uint8_t>(payload[3]));
  const int8_t d_lo = hex_nibble_(static_cast<uint8_t>(payload[4]));
  if (channel < 0 || unit < 0 || d_hi < 0 || d_mid < 0 || d_lo < 0) {
    this->record_parse_error_("Speaker-distance payload contains non-hex data");
    return;
  }

  if (unit != 0) {
    this->set_last_error_("Speaker distance unit is not meter (UNIT=" + std::to_string(unit) + ")");
    return;
  }

  const uint16_t value = static_cast<uint16_t>((d_hi << 8) | (d_mid << 4) | d_lo);
  this->publish_speaker_distance_(static_cast<uint8_t>(channel), value);
}

void YamahaSerialComponent::parse_tuner_station_(const std::string &payload) {
  if (payload.size() < 9) {
    this->record_parse_error_("Tuner-station payload shorter than minimum length");
    return;
  }

  const char band = payload[2];
  const std::string freq = payload.substr(3, 6);
  if (band == '1') {
    char *end = nullptr;
    const float mhz = std::strtof(freq.c_str(), &end);
    if (end == freq.c_str()) {
      this->record_parse_error_("Tuner FM frequency contains invalid data");
      return;
    }
    if (this->tuner_fm_frequency_number_ != nullptr) {
      this->tuner_fm_frequency_number_->publish_state(mhz);
    }
    return;
  }

  if (band == '0') {
    char *end = nullptr;
    const float khz = std::strtof(freq.c_str(), &end);
    if (end == freq.c_str()) {
      this->record_parse_error_("Tuner AM frequency contains invalid data");
      return;
    }
    if (this->tuner_am_frequency_number_ != nullptr) {
      this->tuner_am_frequency_number_->publish_state(khz);
    }
  }
}

void YamahaSerialComponent::record_parse_error_(const std::string &message) {
  this->parse_errors_++;
  this->last_parse_error_ = message;
  ESP_LOGW(TAG, "Parse error: %s", message.c_str());
  if (this->last_parse_error_text_sensor_ != nullptr) {
    this->last_parse_error_text_sensor_->publish_state(this->last_parse_error_);
  }
}

void YamahaSerialComponent::record_parse_error_(const std::string &message, const std::vector<uint8_t> &frame) {
  std::string detail = message;
  if (!frame.empty()) {
    detail += " frame=";
    detail += frame_to_hex_(frame);
  }
  this->record_parse_error_(detail);
}

void YamahaSerialComponent::publish_connection_state_(const std::string &state) {
  if (this->connection_state_text_sensor_ != nullptr) {
    this->connection_state_text_sensor_->publish_state(state);
  }
}

void YamahaSerialComponent::set_last_error_(const std::string &message) {
  this->last_error_ = message;
  ESP_LOGW(TAG, "%s", message.c_str());
  if (this->last_error_text_sensor_ != nullptr) {
    this->last_error_text_sensor_->publish_state(message);
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
  if (now - this->last_diagnostics_publish_ms_ < 1000U) {
    return;
  }
  this->last_diagnostics_publish_ms_ = now;

  if (this->last_response_age_sensor_ != nullptr) {
    const float age = this->last_response_ms_ == 0 ? NAN : static_cast<float>(now - this->last_response_ms_);
    this->last_response_age_sensor_->publish_state(age);
  }
  if (this->commands_sent_sensor_ != nullptr) {
    this->commands_sent_sensor_->publish_state(static_cast<float>(this->commands_sent_));
  }
  if (this->responses_received_sensor_ != nullptr) {
    this->responses_received_sensor_->publish_state(static_cast<float>(this->responses_received_));
  }
  if (this->parse_errors_sensor_ != nullptr) {
    this->parse_errors_sensor_->publish_state(static_cast<float>(this->parse_errors_));
  }
  if (this->timeouts_sensor_ != nullptr) {
    this->timeouts_sensor_->publish_state(static_cast<float>(this->timeouts_));
  }
  if (this->queue_drops_sensor_ != nullptr) {
    this->queue_drops_sensor_->publish_state(static_cast<float>(this->queue_drops_));
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
  const char *label = lookup_label_(AUDIO_SELECT_REPORTS, std::size(AUDIO_SELECT_REPORTS), report_id);
  if (label == nullptr) {
    return;
  }
  this->audio_select_label_ = label;
  if (this->audio_select_select_ != nullptr && this->audio_select_select_->has_option(this->audio_select_label_)) {
    this->audio_select_select_->publish_state(this->audio_select_label_);
  }
  if (this->audio_select_text_sensor_ != nullptr) {
    this->audio_select_text_sensor_->publish_state(this->audio_select_label_);
  }
}

void YamahaSerialComponent::publish_night_mode_by_report_(uint8_t high_nibble, uint8_t low_nibble) {
  const uint8_t key = static_cast<uint8_t>((high_nibble << 4) | low_nibble);
  const char *label = lookup_label_(NIGHT_MODE_REPORTS, std::size(NIGHT_MODE_REPORTS), key);
  if (label == nullptr) {
    return;
  }
  this->night_mode_label_ = label;
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
  const char *label = lookup_label_(TUNER_PRESET_PAGE_REPORTS, std::size(TUNER_PRESET_PAGE_REPORTS), report_id);
  if (label == nullptr) {
    return;
  }
  this->tuner_preset_page_label_ = label;
  if (this->tuner_preset_page_select_ != nullptr &&
      this->tuner_preset_page_select_->has_option(this->tuner_preset_page_label_)) {
    this->tuner_preset_page_select_->publish_state(this->tuner_preset_page_label_);
  }
}

void YamahaSerialComponent::publish_tuner_band_by_report_(uint8_t report_id) {
  const char *label = lookup_label_(TUNER_BAND_REPORTS, std::size(TUNER_BAND_REPORTS), report_id);
  if (label == nullptr) {
    return;
  }
  this->tuner_band_label_ = label;
  if (this->tuner_band_select_ != nullptr && this->tuner_band_select_->has_option(this->tuner_band_label_)) {
    this->tuner_band_select_->publish_state(this->tuner_band_label_);
  }
}

void YamahaSerialComponent::publish_sleep_timer_by_report_(uint8_t report_id) {
  const char *label = lookup_label_(SLEEP_TIMER_REPORTS, std::size(SLEEP_TIMER_REPORTS), report_id);
  if (label == nullptr) {
    return;
  }
  this->sleep_timer_label_ = label;
  if (this->sleep_timer_select_ != nullptr && this->sleep_timer_select_->has_option(this->sleep_timer_label_)) {
    this->sleep_timer_select_->publish_state(this->sleep_timer_label_);
  }
}

void YamahaSerialComponent::publish_decoder_mode_by_report_(uint8_t report_id) {
  const char *label = lookup_label_(DECODER_MODE_REPORTS, std::size(DECODER_MODE_REPORTS), report_id);
  if (label == nullptr) {
    return;
  }
  this->decoder_mode_label_ = label;
  if (this->decoder_mode_select_ != nullptr && this->decoder_mode_select_->has_option(this->decoder_mode_label_)) {
    this->decoder_mode_select_->publish_state(this->decoder_mode_label_);
  }
}

void YamahaSerialComponent::publish_extended_surround_by_report_(uint8_t report_id) {
  const char *label = lookup_label_(EXTENDED_SURROUND_REPORTS, std::size(EXTENDED_SURROUND_REPORTS), report_id);
  if (label == nullptr) {
    return;
  }
  this->extended_surround_label_ = label;
  if (this->extended_surround_select_ != nullptr &&
      this->extended_surround_select_->has_option(this->extended_surround_label_)) {
    this->extended_surround_select_->publish_state(this->extended_surround_label_);
  }
}

void YamahaSerialComponent::publish_speaker_b_assignment_by_report_(uint8_t report_id) {
  const char *label = lookup_label_(SPEAKER_B_ASSIGNMENT_REPORTS, std::size(SPEAKER_B_ASSIGNMENT_REPORTS), report_id);
  if (label == nullptr) {
    return;
  }
  this->speaker_b_assignment_label_ = label;
  if (this->speaker_b_assignment_select_ != nullptr &&
      this->speaker_b_assignment_select_->has_option(this->speaker_b_assignment_label_)) {
    this->speaker_b_assignment_select_->publish_state(this->speaker_b_assignment_label_);
  }
}

void YamahaSerialComponent::publish_zone2_amp_by_report_(uint8_t report_id) {
  const char *label = lookup_label_(ZONE2_AMP_REPORTS, std::size(ZONE2_AMP_REPORTS), report_id);
  if (label == nullptr) {
    return;
  }
  this->zone2_amp_label_ = label;
  if (this->zone2_amp_select_ != nullptr && this->zone2_amp_select_->has_option(this->zone2_amp_label_)) {
    this->zone2_amp_select_->publish_state(this->zone2_amp_label_);
  }
}

void YamahaSerialComponent::publish_playback_format_by_report_(uint8_t report_id) {
  const char *label = lookup_label_(PLAYBACK_FORMAT_REPORTS, std::size(PLAYBACK_FORMAT_REPORTS), report_id);
  if (label == nullptr) {
    return;
  }
  this->playback_format_ = label;
  if (this->playback_format_text_sensor_ != nullptr) {
    this->playback_format_text_sensor_->publish_state(this->playback_format_);
  }
}

void YamahaSerialComponent::publish_sampling_rate_by_report_(uint8_t report_id) {
  const SamplingRateInfo *info = lookup_sampling_rate_(SAMPLING_RATE_REPORTS, std::size(SAMPLING_RATE_REPORTS), report_id);
  if (info == nullptr) {
    return;
  }
  this->sampling_rate_ = info->label;
  if (this->sampling_rate_text_sensor_ != nullptr) {
    this->sampling_rate_text_sensor_->publish_state(this->sampling_rate_);
  }
  this->sampling_rate_hz_ = info->hz;
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

void YamahaSerialComponent::check_availability_() {
  if (!this->periodic_poll_enabled_ && this->receiver_reported_) {
    if (!this->available_) {
      this->available_ = true;
      if (this->availability_binary_sensor_ != nullptr) {
        this->availability_binary_sensor_->publish_state(true);
      }
      this->publish_connection_state_("online");
    }
    return;
  }

  const uint32_t now = millis();
  const uint32_t threshold = std::max<uint32_t>(
      15000U, std::max<uint32_t>(this->command_timeout_ms_ * (this->max_retries_ + 2U), this->get_update_interval() * 3U));

  const bool available = this->last_response_ms_ != 0 && (now - this->last_response_ms_ <= threshold);
  if (available == this->available_) {
    return;
  }
  this->available_ = available;
  if (this->availability_binary_sensor_ != nullptr) {
    this->availability_binary_sensor_->publish_state(available);
  }
  if (!available) {
    this->receiver_reported_ = false;
    if (this->in_flight_.has_value() && is_coalescible_tag_(this->in_flight_->command.tag)) {
      this->in_flight_.reset();
    }
    this->queue_.erase(std::remove_if(this->queue_.begin(), this->queue_.end(),
                                      [](const QueuedCommand &cmd) { return is_coalescible_tag_(cmd.tag); }),
                       this->queue_.end());
    this->queue_probe_ready_();
  }
  this->publish_connection_state_(available ? "online" : "timeout");
}

bool YamahaSerialComponent::is_coalescible_tag_(const char *tag) {
  return std::strncmp(tag, "poll_", 5) == 0 || std::strcmp(tag, "report_enable") == 0 ||
         std::strcmp(tag, "report_delay_realtime") == 0 || std::strcmp(tag, "ready") == 0 ||
         std::strcmp(tag, "probe_ready") == 0;
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

  static const std::pair<const char *, const char *> aliases[] = {
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
    if (token != it->first) {
      continue;
    }
    for (size_t i = 0; i < this->inputs_.size(); i++) {
      if (this->inputs_[i].key == it->second) {
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
    if (ch == ' ' || ch == '-' || ch == '/' || ch == '.' || ch == ':') {
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

std::string YamahaSerialComponent::frame_to_hex_(const std::vector<uint8_t> &frame) {
  std::string out{};
  out.reserve(frame.size() * 3);
  for (size_t i = 0; i < frame.size(); i++) {
    if (i != 0) {
      out.push_back(' ');
    }
    out.push_back(static_cast<char>(nibble_to_hex_(frame[i] >> 4)));
    out.push_back(static_cast<char>(nibble_to_hex_(frame[i] & 0x0F)));
  }
  return out;
}

std::string YamahaSerialComponent::normalize_model_name_(const std::string &model) {
  std::string value = trim_(model);
  if (value.size() > 2 && std::isdigit(static_cast<unsigned char>(value[0])) != 0 &&
      std::isdigit(static_cast<unsigned char>(value[1])) != 0 && value[2] == 'R') {
    value = value.substr(2);
  }
  return value;
}

uint8_t YamahaSerialComponent::checksum_8bit_(const std::string &payload) {
  uint32_t sum = 0;
  for (const unsigned char c : payload) {
    sum += c;
  }
  return static_cast<uint8_t>(sum & 0xFFU);
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
  if (end == raw.c_str()) {
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
