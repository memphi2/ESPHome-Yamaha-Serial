// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "esphome/core/automation.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/button/button.h"
#include "esphome/components/media_player/media_player.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace esphome::yamaha_serial {

class YamahaSerialComponent;

class YamahaPowerSwitch : public switch_::Switch {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void write_state(bool state) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaMainZonePowerSwitch : public switch_::Switch {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void write_state(bool state) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaMuteSwitch : public switch_::Switch {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void write_state(bool state) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaZone2PowerSwitch : public switch_::Switch {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void write_state(bool state) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaPureDirectSwitch : public switch_::Switch {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void write_state(bool state) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaFanModeSwitch : public switch_::Switch {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void write_state(bool state) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaZone2MuteSwitch : public switch_::Switch {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void write_state(bool state) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaSpeakerASwitch : public switch_::Switch {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void write_state(bool state) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaSpeakerBSwitch : public switch_::Switch {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void write_state(bool state) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaVolumeNumber : public number::Number {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(float value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaZone2VolumeNumber : public number::Number {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(float value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaBassNumber : public number::Number {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(float value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaTrebleNumber : public number::Number {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(float value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaCenterDistanceNumber : public number::Number {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(float value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaFrontLeftDistanceNumber : public number::Number {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(float value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaFrontRightDistanceNumber : public number::Number {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(float value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaSurroundLeftDistanceNumber : public number::Number {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(float value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaSurroundRightDistanceNumber : public number::Number {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(float value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaSubwooferDistanceNumber : public number::Number {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(float value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaDimmerNumber : public number::Number {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(float value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaTunerFmFrequencyNumber : public number::Number {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(float value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaTunerAmFrequencyNumber : public number::Number {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(float value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaInputSelect : public select::Select {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }
  void set_options_from_labels(const std::vector<std::string> &labels);

 protected:
  void control(const std::string &value) override;
  YamahaSerialComponent *parent_{nullptr};
  std::vector<std::string> stored_options_{};
};

class YamahaZone2InputSelect : public select::Select {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }
  void set_options_from_labels(const std::vector<std::string> &labels);

 protected:
  void control(const std::string &value) override;
  YamahaSerialComponent *parent_{nullptr};
  std::vector<std::string> stored_options_{};
};

class YamahaProgramSelect : public select::Select {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }
  void set_options_from_labels(const std::vector<std::string> &labels);

 protected:
  void control(const std::string &value) override;
  YamahaSerialComponent *parent_{nullptr};
  std::vector<std::string> stored_options_{};
};

class YamahaSceneSelect : public select::Select {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(const std::string &value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaAudioSelectSelect : public select::Select {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(const std::string &value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaNightModeSelect : public select::Select {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(const std::string &value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaTunerPresetSelect : public select::Select {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(const std::string &value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaTunerPresetPageSelect : public select::Select {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(const std::string &value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaTunerBandSelect : public select::Select {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(const std::string &value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaSleepTimerSelect : public select::Select {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(const std::string &value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaDecoderModeSelect : public select::Select {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(const std::string &value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaExtendedSurroundSelect : public select::Select {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(const std::string &value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaSpeakerBAssignmentSelect : public select::Select {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(const std::string &value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaZone2AmpSelect : public select::Select {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void control(const std::string &value) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaRefreshButton : public button::Button {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void press_action() override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaPowerToggleButton : public button::Button {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void press_action() override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaVolumeUpButton : public button::Button {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void press_action() override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaVolumeDownButton : public button::Button {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void press_action() override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaZone2VolumeUpButton : public button::Button {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void press_action() override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaZone2VolumeDownButton : public button::Button {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void press_action() override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaReceiverResetButton : public button::Button {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void press_action() override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaTunerAutoUpButton : public button::Button {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void press_action() override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaTunerAutoDownButton : public button::Button {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }

 protected:
  void press_action() override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaMediaPlayer : public media_player::MediaPlayer {
 public:
  void set_parent(YamahaSerialComponent *parent) { this->parent_ = parent; }
  media_player::MediaPlayerTraits get_traits() override;
  bool is_muted() const override;

 protected:
  void control(const media_player::MediaPlayerCall &call) override;
  YamahaSerialComponent *parent_{nullptr};
};

class YamahaSerialComponent : public PollingComponent, public uart::UARTDevice {
 public:
  void setup() override;
  void update() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_model(const std::string &model) { this->configured_model_ = model; }
  void set_receiver_profile(const std::string &profile) { this->configured_profile_ = profile; }
  void set_periodic_poll_enabled(bool enabled) { this->periodic_poll_enabled_ = enabled; }
  void set_command_timeout(uint32_t command_timeout_ms) { this->command_timeout_ms_ = command_timeout_ms; }
  void set_max_retries(uint8_t max_retries) { this->max_retries_ = max_retries; }
  void set_command_spacing(uint32_t command_spacing_ms) { this->command_spacing_ms_ = command_spacing_ms; }
  void set_power_on_delay(uint32_t power_on_delay_ms) { this->power_on_delay_ms_ = power_on_delay_ms; }
  void set_volume_config(float min_db, float max_db, float step_db);
  void add_input_mapping(const std::string &key, const std::string &label);

  void set_power_switch(YamahaPowerSwitch *obj) { this->power_switch_ = obj; }
  void set_main_zone_power_switch(YamahaMainZonePowerSwitch *obj) { this->main_zone_power_switch_ = obj; }
  void set_mute_switch(YamahaMuteSwitch *obj) { this->mute_switch_ = obj; }
  void set_zone2_power_switch(YamahaZone2PowerSwitch *obj) { this->zone2_power_switch_ = obj; }
  void set_pure_direct_switch(YamahaPureDirectSwitch *obj) { this->pure_direct_switch_ = obj; }
  void set_fan_mode_switch(YamahaFanModeSwitch *obj) { this->fan_mode_switch_ = obj; }
  void set_zone2_mute_switch(YamahaZone2MuteSwitch *obj) { this->zone2_mute_switch_ = obj; }
  void set_speaker_a_switch(YamahaSpeakerASwitch *obj) { this->speaker_a_switch_ = obj; }
  void set_speaker_b_switch(YamahaSpeakerBSwitch *obj) { this->speaker_b_switch_ = obj; }

  void set_volume_number(YamahaVolumeNumber *obj) { this->volume_number_ = obj; }
  void set_zone2_volume_number(YamahaZone2VolumeNumber *obj) { this->zone2_volume_number_ = obj; }
  void set_bass_number(YamahaBassNumber *obj) { this->bass_number_ = obj; }
  void set_treble_number(YamahaTrebleNumber *obj) { this->treble_number_ = obj; }
  void set_center_distance_number(YamahaCenterDistanceNumber *obj) { this->center_distance_number_ = obj; }
  void set_front_left_distance_number(YamahaFrontLeftDistanceNumber *obj) { this->front_left_distance_number_ = obj; }
  void set_front_right_distance_number(YamahaFrontRightDistanceNumber *obj) { this->front_right_distance_number_ = obj; }
  void set_surround_left_distance_number(YamahaSurroundLeftDistanceNumber *obj) {
    this->surround_left_distance_number_ = obj;
  }
  void set_surround_right_distance_number(YamahaSurroundRightDistanceNumber *obj) {
    this->surround_right_distance_number_ = obj;
  }
  void set_subwoofer_distance_number(YamahaSubwooferDistanceNumber *obj) { this->subwoofer_distance_number_ = obj; }
  void set_dimmer_number(YamahaDimmerNumber *obj) { this->dimmer_number_ = obj; }
  void set_tuner_fm_frequency_number(YamahaTunerFmFrequencyNumber *obj) { this->tuner_fm_frequency_number_ = obj; }
  void set_tuner_am_frequency_number(YamahaTunerAmFrequencyNumber *obj) { this->tuner_am_frequency_number_ = obj; }

  void set_input_source_select(YamahaInputSelect *obj);
  void set_zone2_input_source_select(YamahaZone2InputSelect *obj);
  void set_program_select(YamahaProgramSelect *obj);
  void set_scene_select(YamahaSceneSelect *obj) { this->scene_select_ = obj; }
  void set_audio_select_select(YamahaAudioSelectSelect *obj) { this->audio_select_select_ = obj; }
  void set_night_mode_select(YamahaNightModeSelect *obj) { this->night_mode_select_ = obj; }
  void set_tuner_preset_select(YamahaTunerPresetSelect *obj) { this->tuner_preset_select_ = obj; }
  void set_tuner_preset_page_select(YamahaTunerPresetPageSelect *obj) { this->tuner_preset_page_select_ = obj; }
  void set_tuner_band_select(YamahaTunerBandSelect *obj) { this->tuner_band_select_ = obj; }
  void set_sleep_timer_select(YamahaSleepTimerSelect *obj) { this->sleep_timer_select_ = obj; }
  void set_decoder_mode_select(YamahaDecoderModeSelect *obj) { this->decoder_mode_select_ = obj; }
  void set_extended_surround_select(YamahaExtendedSurroundSelect *obj) { this->extended_surround_select_ = obj; }
  void set_speaker_b_assignment_select(YamahaSpeakerBAssignmentSelect *obj) {
    this->speaker_b_assignment_select_ = obj;
  }
  void set_zone2_amp_select(YamahaZone2AmpSelect *obj) { this->zone2_amp_select_ = obj; }

  void set_refresh_button(YamahaRefreshButton *obj) { this->refresh_button_ = obj; }
  void set_power_toggle_button(YamahaPowerToggleButton *obj) { this->power_toggle_button_ = obj; }
  void set_volume_up_button(YamahaVolumeUpButton *obj) { this->volume_up_button_ = obj; }
  void set_volume_down_button(YamahaVolumeDownButton *obj) { this->volume_down_button_ = obj; }
  void set_zone2_volume_up_button(YamahaZone2VolumeUpButton *obj) { this->zone2_volume_up_button_ = obj; }
  void set_zone2_volume_down_button(YamahaZone2VolumeDownButton *obj) { this->zone2_volume_down_button_ = obj; }
  void set_receiver_reset_button(YamahaReceiverResetButton *obj) { this->receiver_reset_button_ = obj; }
  void set_tuner_auto_up_button(YamahaTunerAutoUpButton *obj) { this->tuner_auto_up_button_ = obj; }
  void set_tuner_auto_down_button(YamahaTunerAutoDownButton *obj) { this->tuner_auto_down_button_ = obj; }

  void set_volume_db_sensor(sensor::Sensor *obj) { this->volume_db_sensor_ = obj; }
  void set_zone2_volume_db_sensor(sensor::Sensor *obj) { this->zone2_volume_db_sensor_ = obj; }
  void set_sampling_rate_sensor(sensor::Sensor *obj) { this->sampling_rate_sensor_ = obj; }
  void set_last_response_age_sensor(sensor::Sensor *obj) { this->last_response_age_sensor_ = obj; }
  void set_commands_sent_sensor(sensor::Sensor *obj) { this->commands_sent_sensor_ = obj; }
  void set_responses_received_sensor(sensor::Sensor *obj) { this->responses_received_sensor_ = obj; }
  void set_parse_errors_sensor(sensor::Sensor *obj) { this->parse_errors_sensor_ = obj; }
  void set_timeouts_sensor(sensor::Sensor *obj) { this->timeouts_sensor_ = obj; }
  void set_queue_drops_sensor(sensor::Sensor *obj) { this->queue_drops_sensor_ = obj; }

  void set_power_state_text_sensor(text_sensor::TextSensor *obj) { this->power_state_text_sensor_ = obj; }
  void set_input_source_text_sensor(text_sensor::TextSensor *obj) { this->input_source_text_sensor_ = obj; }
  void set_zone2_input_source_text_sensor(text_sensor::TextSensor *obj) { this->zone2_input_source_text_sensor_ = obj; }
  void set_program_text_sensor(text_sensor::TextSensor *obj) { this->program_text_sensor_ = obj; }
  void set_playback_format_text_sensor(text_sensor::TextSensor *obj) { this->playback_format_text_sensor_ = obj; }
  void set_sampling_rate_text_sensor(text_sensor::TextSensor *obj) { this->sampling_rate_text_sensor_ = obj; }
  void set_main_volume_text_sensor(text_sensor::TextSensor *obj) { this->main_volume_text_sensor_ = obj; }
  void set_audio_select_text_sensor(text_sensor::TextSensor *obj) { this->audio_select_text_sensor_ = obj; }
  void set_night_mode_text_sensor(text_sensor::TextSensor *obj) { this->night_mode_text_sensor_ = obj; }
  void set_last_error_text_sensor(text_sensor::TextSensor *obj) { this->last_error_text_sensor_ = obj; }
  void set_last_parse_error_text_sensor(text_sensor::TextSensor *obj) { this->last_parse_error_text_sensor_ = obj; }
  void set_connection_state_text_sensor(text_sensor::TextSensor *obj) { this->connection_state_text_sensor_ = obj; }
  void set_receiver_model_text_sensor(text_sensor::TextSensor *obj) { this->receiver_model_text_sensor_ = obj; }
  void set_availability_binary_sensor(binary_sensor::BinarySensor *obj) { this->availability_binary_sensor_ = obj; }
  void set_media_player(YamahaMediaPlayer *obj) {
    this->media_player_ = obj;
    this->sync_media_player_state_();
  }

  bool get_mute_state() const { return this->mute_on_; }
  float get_main_volume_db() const { return this->main_volume_db_; }
  float get_volume_min_db() const { return this->volume_min_db_; }
  float get_volume_max_db() const { return this->volume_max_db_; }

  void command_power(bool on);
  void command_main_zone_power(bool on);
  void command_mute(bool on);
  void command_zone2_power(bool on);
  void command_volume_absolute(float db, bool zone2);
  void command_volume_step(bool up, bool zone2);
  void command_input(const std::string &label_or_key, bool zone2);
  void command_program(const std::string &label);
  void command_program_step(bool next);
  void command_media_player_shortcut(const std::string &value);
  void command_scene(const std::string &label);
  void command_audio_select(const std::string &label);
  void command_night_mode(const std::string &label);
  void command_fan_mode(bool on);
  void command_pure_direct(bool on);
  void command_zone2_mute(bool on);
  void command_speaker_relay(char relay, bool on);
  void command_tuner_preset_page(const std::string &label);
  void command_tuner_band(const std::string &label);
  void command_sleep_timer(const std::string &label);
  void command_decoder_mode(const std::string &label);
  void command_extended_surround(const std::string &label);
  void command_speaker_b_assignment(const std::string &label);
  void command_zone2_amp(const std::string &label);
  void command_tuner_auto_seek(bool up);
  void command_dimmer_percent(float percent);
  void command_tuner_preset(uint8_t preset);
  void command_tuner_frequency(float frequency, bool fm);
  void command_bass_percent(float percent);
  void command_treble_percent(float percent);
  void command_speaker_distance(uint8_t channel_id, uint16_t distance_value);
  void command_receiver_reset();
  void command_refresh();
  void command_power_toggle();
  void command_raw_text(const std::string &value);

 protected:
  enum class FrameType : uint8_t {
    STX = 0x02,
    ETX = 0x03,
    DC1 = 0x11,
    DC2 = 0x12,
    DC3 = 0x13,
    DC4 = 0x14,
  };

  struct InputMapping {
    std::string key;
    std::string label;
    uint8_t report_id;
    uint8_t zone2_report_id;
    std::string main_command;
    std::string zone2_command;
  };

  struct InputOverride {
    std::string key;
    std::string label;
  };

  struct ProgramMapping {
    std::string label;
    uint8_t report_code;
    std::string command;
  };

  struct SceneMapping {
    std::string label;
    std::string command;
  };

  struct StaticInputMapping {
    const char *key;
    const char *label;
    uint8_t report_id;
    uint8_t zone2_report_id;
    const char *main_command;
    const char *zone2_command;
  };

  struct StaticProgramMapping {
    const char *label;
    uint8_t report_code;
    const char *command;
  };

  struct StaticSceneMapping {
    const char *label;
    const char *command;
  };

  struct ProfileCapabilities {
    bool supports_zone2{true};
    bool supports_scene{true};
    bool supports_audio_select{true};
    bool supports_night_mode{true};
    bool supports_fan_mode{true};
    bool supports_pure_direct{true};
    bool supports_extended{true};
    bool decode_dc2_layout{false};
  };

  struct QueuedCommand {
    FrameType frame_type;
    std::string payload;
    const char *tag{""};
    bool expect_response{true};
    uint32_t timeout_ms{0};
    uint8_t retries_left{0};
  };

  struct InFlightCommand {
    QueuedCommand command;
    uint32_t sent_at_ms{0};
    uint8_t attempts{1};
  };

  static constexpr uint8_t BUFFER_MAX_LEN = 192;
  static constexpr uint32_t FRAME_READ_TIMEOUT_MS = 550;
  static constexpr size_t MAX_QUEUE_LEN = 32;

  std::string resolve_profile_() const;
  void apply_profile_(const std::string &profile_id);
  void apply_profile_rx_vx500_();
  void apply_profile_rx_vx500_extended_();
  void apply_profile_rx_vx600_();
  void apply_profile_rx_vx600_extended_();
  void apply_profile_rx_vx700_();
  void apply_profile_rx_vx700_extended_();
  void apply_profile_rx_vx800_();
  void apply_profile_rx_vx800_extended_();
  void clear_profile_mappings_();
  void load_input_mappings_(const StaticInputMapping *mappings, size_t count);
  void load_program_mappings_(const StaticProgramMapping *mappings, size_t count);
  void load_scene_mappings_(const StaticSceneMapping *mappings, size_t count);
  void setup_default_mappings_();
  void apply_input_overrides_();
  bool is_auto_profile_() const;
  void handle_detected_model_(const std::string &model);
  void configure_input_select_options_();
  void configure_program_select_options_();
  void queue_command_(const QueuedCommand &command);
  static bool is_coalescible_tag_(const char *tag);
  void queue_simple_command_(FrameType type, const std::string &payload, const char *tag, bool expect_response = true);
  bool queue_named_command_(const std::string &alias);
  void queue_bootstrap_();
  void queue_probe_ready_();
  void queue_poll_commands_();
  void process_command_queue_();
  bool send_command_(const QueuedCommand &command);
  void process_incoming_byte_(uint8_t byte);
  void parse_frame_(const std::vector<uint8_t> &frame);
  void parse_stx_frame_(const std::vector<uint8_t> &frame);
  void parse_dc1_frame_(const std::vector<uint8_t> &frame);
  void parse_dc2_frame_(const std::vector<uint8_t> &frame);
  void parse_dc4_frame_(const std::vector<uint8_t> &frame);
  void parse_dc4_extended_payload_(const std::string &payload, const std::vector<uint8_t> &frame);
  void parse_tone_control_(const std::string &payload);
  void parse_speaker_distance_(const std::string &payload);
  void parse_tuner_station_(const std::string &payload);
  void record_parse_error_(const std::string &message);
  void record_parse_error_(const std::string &message, const std::vector<uint8_t> &frame);
  void publish_connection_state_(const std::string &state);
  void set_last_error_(const std::string &message);
  void clear_last_error_();
  void publish_diagnostics_();
  void publish_power_state_(bool on);
  void publish_zone2_power_state_(bool on);
  void publish_mute_state_(bool on);
  void publish_zone2_mute_state_(bool on);
  void publish_speaker_relay_state_(char relay, bool on);
  void publish_volume_db_(float db, bool zone2);
  void publish_input_by_report_(uint8_t report_id, bool zone2);
  void publish_input_label_(const std::string &label, bool zone2);
  void publish_program_by_report_(uint8_t report_code);
  void publish_program_label_(const std::string &label);
  void publish_bass_percent_(float percent);
  void publish_treble_percent_(float percent);
  void publish_speaker_distance_(uint8_t channel_id, uint16_t value);
  void publish_dimmer_percent_(uint8_t dimmer_nibble);
  void publish_audio_select_by_report_(uint8_t report_id);
  void publish_night_mode_by_report_(uint8_t high_nibble, uint8_t low_nibble);
  void publish_tuner_preset_by_report_(uint8_t report_id);
  void publish_tuner_preset_page_by_report_(uint8_t report_id);
  void publish_tuner_band_by_report_(uint8_t report_id);
  void publish_sleep_timer_by_report_(uint8_t report_id);
  void publish_decoder_mode_by_report_(uint8_t report_id);
  void publish_extended_surround_by_report_(uint8_t report_id);
  void publish_speaker_b_assignment_by_report_(uint8_t report_id);
  void publish_zone2_amp_by_report_(uint8_t report_id);
  void publish_playback_format_by_report_(uint8_t report_id);
  void publish_sampling_rate_by_report_(uint8_t report_id);
  void queue_extended_poll_commands_();
  void sync_media_player_state_();
  void check_availability_();
  std::optional<size_t> find_input_index_(const std::string &label_or_key) const;
  std::optional<size_t> find_program_index_(const std::string &label) const;

  static bool is_frame_start_(uint8_t value);
  static int8_t hex_nibble_(uint8_t value);
  static uint8_t nibble_to_hex_(uint8_t value);
  static std::string normalize_token_(const std::string &in);
  static std::string trim_(const std::string &in);
  static std::string frame_to_hex_(const std::vector<uint8_t> &frame);
  static std::string normalize_model_name_(const std::string &model);
  static uint8_t checksum_8bit_(const std::string &payload);
  static std::optional<float> parse_volume_text_db_(const std::string &text);
  static std::string uppercase_ascii_(const std::string &value);
  static bool payload_is_printable_ascii_(const std::string &payload);
  static uint8_t volume_db_to_raw_(float db);
  static float volume_raw_to_db_(uint8_t raw);

  std::string configured_model_{"RX-Vx500"};
  std::string configured_profile_{"auto"};
  std::string active_profile_{"unresolved"};
  std::string receiver_model_{"unknown"};
  bool profile_initialized_{false};

  float volume_min_db_{-80.0f};
  float volume_max_db_{16.5f};
  float volume_step_db_{0.5f};

  uint32_t command_timeout_ms_{1500};
  uint8_t max_retries_{2};
  uint32_t command_spacing_ms_{100};
  uint32_t power_on_delay_ms_{2000};
  bool periodic_poll_enabled_{true};

  std::deque<QueuedCommand> queue_{};
  std::optional<InFlightCommand> in_flight_{};
  uint32_t last_tx_ms_{0};
  uint32_t command_hold_until_ms_{0};
  bool receiver_reported_{false};

  bool in_frame_{false};
  uint32_t frame_started_ms_{0};
  std::vector<uint8_t> frame_buffer_{};

  std::vector<InputMapping> inputs_{};
  std::vector<InputOverride> input_overrides_{};

  std::vector<ProgramMapping> programs_{};

  std::vector<SceneMapping> scenes_{};
  ProfileCapabilities profile_caps_{};

  bool main_power_on_{false};
  bool zone2_power_on_{false};
  bool mute_on_{false};
  bool zone2_mute_on_{false};
  bool pure_direct_on_{false};
  bool fan_mode_on_{false};
  bool speaker_a_on_{false};
  bool speaker_b_on_{false};
  float main_volume_db_{NAN};
  float zone2_volume_db_{NAN};
  float bass_percent_{NAN};
  float treble_percent_{NAN};
  std::string input_label_{"unknown"};
  std::string zone2_input_label_{"unknown"};
  std::string program_label_{"unknown"};
  std::string audio_select_label_{"unknown"};
  std::string night_mode_label_{"unknown"};
  std::string tuner_preset_label_{"unknown"};
  std::string tuner_preset_page_label_{"unknown"};
  std::string tuner_band_label_{"unknown"};
  std::string sleep_timer_label_{"unknown"};
  std::string decoder_mode_label_{"unknown"};
  std::string extended_surround_label_{"unknown"};
  std::string speaker_b_assignment_label_{"unknown"};
  std::string zone2_amp_label_{"unknown"};
  std::string playback_format_{"unknown"};
  std::string sampling_rate_{"unknown"};
  float sampling_rate_hz_{NAN};
  std::string main_volume_text_{""};
  std::string scene_label_{"unknown"};
  bool available_{false};
  std::string last_error_{""};
  std::string last_parse_error_{""};

  uint32_t last_response_ms_{0};
  uint32_t last_diagnostics_publish_ms_{0};
  uint32_t commands_sent_{0};
  uint32_t responses_received_{0};
  uint32_t parse_errors_{0};
  uint32_t timeouts_{0};
  uint32_t queue_drops_{0};

  YamahaPowerSwitch *power_switch_{nullptr};
  YamahaMainZonePowerSwitch *main_zone_power_switch_{nullptr};
  YamahaMuteSwitch *mute_switch_{nullptr};
  YamahaZone2PowerSwitch *zone2_power_switch_{nullptr};
  YamahaPureDirectSwitch *pure_direct_switch_{nullptr};
  YamahaFanModeSwitch *fan_mode_switch_{nullptr};
  YamahaZone2MuteSwitch *zone2_mute_switch_{nullptr};
  YamahaSpeakerASwitch *speaker_a_switch_{nullptr};
  YamahaSpeakerBSwitch *speaker_b_switch_{nullptr};

  YamahaVolumeNumber *volume_number_{nullptr};
  YamahaZone2VolumeNumber *zone2_volume_number_{nullptr};
  YamahaBassNumber *bass_number_{nullptr};
  YamahaTrebleNumber *treble_number_{nullptr};
  YamahaCenterDistanceNumber *center_distance_number_{nullptr};
  YamahaFrontLeftDistanceNumber *front_left_distance_number_{nullptr};
  YamahaFrontRightDistanceNumber *front_right_distance_number_{nullptr};
  YamahaSurroundLeftDistanceNumber *surround_left_distance_number_{nullptr};
  YamahaSurroundRightDistanceNumber *surround_right_distance_number_{nullptr};
  YamahaSubwooferDistanceNumber *subwoofer_distance_number_{nullptr};
  YamahaDimmerNumber *dimmer_number_{nullptr};
  YamahaTunerFmFrequencyNumber *tuner_fm_frequency_number_{nullptr};
  YamahaTunerAmFrequencyNumber *tuner_am_frequency_number_{nullptr};

  YamahaInputSelect *input_source_select_{nullptr};
  YamahaZone2InputSelect *zone2_input_source_select_{nullptr};
  YamahaProgramSelect *program_select_{nullptr};
  YamahaSceneSelect *scene_select_{nullptr};
  YamahaAudioSelectSelect *audio_select_select_{nullptr};
  YamahaNightModeSelect *night_mode_select_{nullptr};
  YamahaTunerPresetSelect *tuner_preset_select_{nullptr};
  YamahaTunerPresetPageSelect *tuner_preset_page_select_{nullptr};
  YamahaTunerBandSelect *tuner_band_select_{nullptr};
  YamahaSleepTimerSelect *sleep_timer_select_{nullptr};
  YamahaDecoderModeSelect *decoder_mode_select_{nullptr};
  YamahaExtendedSurroundSelect *extended_surround_select_{nullptr};
  YamahaSpeakerBAssignmentSelect *speaker_b_assignment_select_{nullptr};
  YamahaZone2AmpSelect *zone2_amp_select_{nullptr};

  YamahaRefreshButton *refresh_button_{nullptr};
  YamahaPowerToggleButton *power_toggle_button_{nullptr};
  YamahaVolumeUpButton *volume_up_button_{nullptr};
  YamahaVolumeDownButton *volume_down_button_{nullptr};
  YamahaZone2VolumeUpButton *zone2_volume_up_button_{nullptr};
  YamahaZone2VolumeDownButton *zone2_volume_down_button_{nullptr};
  YamahaReceiverResetButton *receiver_reset_button_{nullptr};
  YamahaTunerAutoUpButton *tuner_auto_up_button_{nullptr};
  YamahaTunerAutoDownButton *tuner_auto_down_button_{nullptr};

  sensor::Sensor *volume_db_sensor_{nullptr};
  sensor::Sensor *zone2_volume_db_sensor_{nullptr};
  sensor::Sensor *sampling_rate_sensor_{nullptr};
  sensor::Sensor *last_response_age_sensor_{nullptr};
  sensor::Sensor *commands_sent_sensor_{nullptr};
  sensor::Sensor *responses_received_sensor_{nullptr};
  sensor::Sensor *parse_errors_sensor_{nullptr};
  sensor::Sensor *timeouts_sensor_{nullptr};
  sensor::Sensor *queue_drops_sensor_{nullptr};

  text_sensor::TextSensor *power_state_text_sensor_{nullptr};
  text_sensor::TextSensor *input_source_text_sensor_{nullptr};
  text_sensor::TextSensor *zone2_input_source_text_sensor_{nullptr};
  text_sensor::TextSensor *program_text_sensor_{nullptr};
  text_sensor::TextSensor *playback_format_text_sensor_{nullptr};
  text_sensor::TextSensor *sampling_rate_text_sensor_{nullptr};
  text_sensor::TextSensor *main_volume_text_sensor_{nullptr};
  text_sensor::TextSensor *audio_select_text_sensor_{nullptr};
  text_sensor::TextSensor *night_mode_text_sensor_{nullptr};
  text_sensor::TextSensor *last_error_text_sensor_{nullptr};
  text_sensor::TextSensor *last_parse_error_text_sensor_{nullptr};
  text_sensor::TextSensor *connection_state_text_sensor_{nullptr};
  text_sensor::TextSensor *receiver_model_text_sensor_{nullptr};
  binary_sensor::BinarySensor *availability_binary_sensor_{nullptr};
  YamahaMediaPlayer *media_player_{nullptr};
};

template<typename... Ts> class YamahaRawCommandAction : public Action<Ts...>, public Parented<YamahaSerialComponent> {
 public:
  TEMPLATABLE_VALUE(std::string, command)

  void play(const Ts &...x) override { this->parent_->command_raw_text(this->command_.value(x...)); }
};

}  // namespace esphome::yamaha_serial
