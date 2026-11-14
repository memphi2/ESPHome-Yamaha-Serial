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

void YamahaSerialComponent::parse_stx_frame_(const std::vector<uint8_t> &frame) {
  if (frame.size() != 8) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("STX status frame must contain exactly 8 bytes"), frame);
    return;
  }

  const int8_t cmd_hi = hex_nibble_(frame[3]);
  const int8_t cmd_lo = hex_nibble_(frame[4]);
  const int8_t data_hi = hex_nibble_(frame[5]);
  const int8_t data_lo = hex_nibble_(frame[6]);
  const int8_t grd = hex_nibble_(frame[2]);

  if (cmd_hi < 0 || cmd_lo < 0 || data_hi < 0 || data_lo < 0 || grd < 0 || hex_nibble_(frame[1]) < 0) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("STX frame contains non-hex status fields"), frame);
    return;
  }

  if (grd > 0) {
    this->set_last_error_(YAMAHA_SERIAL_PSTR("Guarded command response (GRD=%d)"), grd);
  }

  const uint8_t cmd = static_cast<uint8_t>((cmd_hi << 4) | cmd_lo);
  const uint8_t data = static_cast<uint8_t>((data_hi << 4) | data_lo);

  switch (cmd) {
    // x700+ format/sampling tables use the full byte (e.g. 0x10 DSD, 0xFE/0xFF); older ones the low nibble.
    case 0x10:
      this->publish_playback_format_by_report_(
          this->profile_caps_.x700_protocol ? data : static_cast<uint8_t>(data_lo));
      break;
    case 0x11:
      this->publish_sampling_rate_by_report_(
          this->profile_caps_.x700_protocol ? data : static_cast<uint8_t>(data_lo));
      break;
    // x700+ source information (RX-Vx700 RS-232C Playback Status Reports, verified on RX-V1900).
    case 0x12:
      if (this->profile_caps_.x700_protocol) {
        this->channel_layout_ = data;
        this->publish_input_channels_();
      }
      break;
    case 0x13:
      if (this->profile_caps_.x700_protocol) {
        this->channel_lfe_ = data;
        this->publish_input_channels_();
      }
      break;
    case 0x14:
      if (this->profile_caps_.x700_protocol)
        this->publish_bit_rate_by_report_(data);
      break;
    case 0x15:
      if (this->profile_caps_.x700_protocol)
        this->publish_dialogue_normalization_by_report_(data);
      break;
    case 0x16:
      if (this->profile_caps_.x700_protocol)
        this->publish_signal_flags_by_report_(data);
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
    case 0x30:
      // System Memory recall, 1-based (x700/x800 manuals; verified on RX-V1900).
      if (this->profile_caps_.x700_protocol && this->profile_caps_.supports_scene)
        this->publish_scene_by_report_(data);
      break;
    case 0x6E:
      // RX-V1900 2ch surround decoder (x700/x800 use a different decoder table).
      if (this->profile_caps_.surround_decoder)
        this->publish_surround_decoder_by_report_(data);
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
  if (frame.size() < 5 || !parse_hex_(std::string_view(reinterpret_cast<const char *>(frame.data() + 1), 2))) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("DC1 text frame shorter than minimum length"), frame);
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
  if (frame.size() < 12) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("DC2 system-status frame shorter than header/checksum"), frame);
    return;
  }
  const std::string_view body(reinterpret_cast<const char *>(frame.data() + 1), frame.size() - 4);
  const auto length = parse_hex_(body.substr(6, 2));
  const auto checksum =
      parse_hex_(std::string_view(reinterpret_cast<const char *>(frame.data() + frame.size() - 3), 2));
  if (!length || frame.size() != 12U + (*length == 0 ? MAX_EXTENDED_DATA_LEN : *length)) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("DC2 system-status length mismatch"), frame);
    return;
  }
  if (!checksum || *checksum != checksum_8bit_(body)) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("DC2 system-status checksum mismatch"), frame);
    return;
  }
  std::string model(frame.begin() + 1, frame.begin() + 6);
  this->handle_detected_model_(model);

  // Standby configuration replies contain only the first ten data bytes.
  if (this->profile_caps_.decode_dc2_layout && frame.size() >= 22) {
    const int8_t power = hex_nibble_(frame[17]);
    if (power >= 0) {
      this->publish_power_state_(power == 1 || power == 2 || power == 4 || power == 5);
      this->publish_zone2_power_state_(power == 1 || power == 3 || power == 4 || power == 6);
    }
  }

  // Extended DC2 CONFIG layout (RX-V1600 offsets; frame[20] onward verified on RX-V1900).
  if (this->profile_caps_.decode_dc2_layout && frame.size() > 138) {
    if (this->profile_caps_.x700_protocol) {
      // x700+ (verified on RX-V1900): input ID is one hex byte across frame[18..19], e.g. 0x12 DOCK.
      const int8_t input_hi = hex_nibble_(frame[18]);
      const int8_t input_lo = hex_nibble_(frame[19]);
      if (input_hi >= 0 && input_lo >= 0) {
        this->publish_input_by_report_(static_cast<uint8_t>((input_hi << 4) | input_lo), false);
      }
    } else {
      // x500/x600: single nibble at frame[18]; frame[19] is not part of the ID.
      const int8_t input = hex_nibble_(frame[18]);
      if (input >= 0) {
        this->publish_input_by_report_(static_cast<uint8_t>(input), false);
      }
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

    // RX-V1600 night-mode offset; in the x700 layout these bytes (DT27-28) hold something else.
    if (!this->profile_caps_.x700_protocol) {
      const int8_t night_hi = hex_nibble_(frame[36]);
      const int8_t night_lo = hex_nibble_(frame[37]);
      if (night_hi >= 0 && night_lo >= 0) {
        this->publish_night_mode_by_report_(static_cast<uint8_t>(night_hi), static_cast<uint8_t>(night_lo));
      }
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

    // x700 layout (DTn = frame[9 + n]); offsets and values verified against RX-V1900 captures.
    // Values use the same encoding as the matching STX reports, so reuse their publishers.
    if (this->profile_caps_.x700_protocol) {
      auto byte_at = [&frame](size_t pos) -> int {
        const int8_t hi = hex_nibble_(frame[pos]);
        const int8_t lo = hex_nibble_(frame[pos + 1]);
        return hi >= 0 && lo >= 0 ? (hi << 4) | lo : -1;
      };
      if (const int8_t v = hex_nibble_(frame[22]); v >= 0)  // DT13 zone 2 input (one nibble)
        this->publish_input_by_report_(static_cast<uint8_t>(v), true);
      if (const int8_t v = hex_nibble_(frame[23]); v >= 0)  // DT14 zone 2 mute
        this->publish_zone2_mute_state_(v == 1);
      if (const int v = byte_at(26); v >= 0)  // DT17-18 zone 2 volume
        this->publish_volume_db_(volume_raw_to_db_(static_cast<uint8_t>(v)), true);
      if (const int8_t v = hex_nibble_(frame[31]); v >= 0)  // DT22 extended surround
        this->publish_extended_surround_by_report_(static_cast<uint8_t>(v));
      if (const int8_t v = hex_nibble_(frame[33]); v >= 0)  // DT24 sleep timer
        this->publish_sleep_timer_by_report_(static_cast<uint8_t>(v));
      if (const int8_t v = hex_nibble_(frame[34]); v >= 0)  // DT25 tuner preset page
        this->publish_tuner_preset_page_by_report_(static_cast<uint8_t>(v));
      if (const int8_t v = hex_nibble_(frame[35]); v >= 0)  // DT26 tuner preset
        this->publish_tuner_preset_by_report_(static_cast<uint8_t>(v));
      if (const int v = byte_at(40); v >= 0)  // DT31-32 playback format
        this->publish_playback_format_by_report_(static_cast<uint8_t>(v));
      if (const int v = byte_at(42); v >= 0)  // DT33-34 sampling rate
        this->publish_sampling_rate_by_report_(static_cast<uint8_t>(v));
      if (const int8_t v = hex_nibble_(frame[46]); v >= 0)  // DT37 tuner band
        this->publish_tuner_band_by_report_(static_cast<uint8_t>(v));
      // DT35 channel front/rear and DT38 channel LFE are single nibbles; F means "---".
      const int8_t layout = hex_nibble_(frame[44]);
      const int8_t lfe = hex_nibble_(frame[47]);
      if (layout >= 0 && lfe >= 0) {
        this->channel_layout_ = layout == 0xF ? 0xFF : static_cast<uint8_t>(layout);
        this->channel_lfe_ = lfe == 0xF ? 0xFF : static_cast<uint8_t>(lfe);
        this->publish_input_channels_();
      }
      if (frame.size() >= 186) {  // Only when the frame reaches DT173.
        if (const int v = byte_at(154); v >= 0)  // DT145-146 bit rate
          this->publish_bit_rate_by_report_(static_cast<uint8_t>(v));
        if (const int v = byte_at(156); v >= 0)  // DT147-148 dialogue normalization
          this->publish_dialogue_normalization_by_report_(static_cast<uint8_t>(v));
        if (const int v = byte_at(181); v >= 0)  // DT172-173 flags
          this->publish_signal_flags_by_report_(static_cast<uint8_t>(v));
      }
    }
  }

  this->publish_connection_state_("configured");
}

void YamahaSerialComponent::parse_dc4_frame_(const std::vector<uint8_t> &frame) {
  if (frame.size() < 8) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("DC4 extended frame shorter than minimum length"), frame);
    return;
  }

  const std::string_view raw(reinterpret_cast<const char *>(frame.data() + 1), frame.size() - 2);
  if (raw.size() < 4) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("DC4 extended payload shorter than checksum length"), frame);
    return;
  }

  const std::string_view body = raw.substr(0, raw.size() - 2);
  const std::string_view rx_sum = raw.substr(raw.size() - 2);
  const uint8_t expected_sum = checksum_8bit_(body);
  const char expected_hi = static_cast<char>(nibble_to_hex_(expected_sum >> 4));
  const char expected_lo = static_cast<char>(nibble_to_hex_(expected_sum & 0x0F));
  if (rx_sum.size() != 2 || rx_sum[0] != expected_hi || rx_sum[1] != expected_lo) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("DC4 checksum mismatch"), frame);
    return;
  }

  this->parse_dc4_extended_payload_(body, frame);
}

void YamahaSerialComponent::parse_dc4_extended_payload_(std::string_view payload, const std::vector<uint8_t> &frame) {
  if (payload.size() < 2 || payload[0] != '2' || payload[1] != '0') {
    ESP_LOGD(TAG, "Ignoring unsupported DC4 payload: %.*s", static_cast<int>(payload.size()), payload.data());
    return;
  }

  if (payload.size() < 8) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("Extended payload shorter than minimum length"), frame);
    return;
  }

  const int8_t len_hi = hex_nibble_(payload[2]);
  const int8_t len_lo = hex_nibble_(payload[3]);
  if (len_hi < 0 || len_lo < 0) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("Extended payload length contains non-hex data"), frame);
    return;
  }

  const size_t encoded_len = static_cast<size_t>((len_hi << 4) | len_lo);
  const size_t data_len = encoded_len == 0 ? MAX_EXTENDED_DATA_LEN : encoded_len;
  if (payload.size() != 4 + data_len) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("Extended payload length mismatch"), frame);
    return;
  }

  const std::string_view data = payload.substr(4, data_len);
  if (data.size() < 4) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("Extended command data shorter than status field"), frame);
    return;
  }

  const std::string_view command_id = data.substr(0, 3);
  const char status = data[3];
  if (!parse_hex_(command_id).has_value() || status < '0' || status > '4') {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("Invalid extended command or status"), frame);
    return;
  }
  if (status != '0') {
    this->set_last_error_(YAMAHA_SERIAL_PSTR("Extended command status error: %c for %.*s"), status,
                          static_cast<int>(command_id.size()), command_id.data());
    return;
  }

  const std::string_view command_data = data.substr(4);
  if (command_data.empty()) {
    return;  // Bare status acknowledgement of a write (RX-V1900); the follow-up poll reads the value.
  }
  if (command_id == "000") {
    if (!command_data.empty() && command_data[0] == 'F') {
      const std::string model = trim_(std::string(command_data.substr(1)));
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

void YamahaSerialComponent::parse_tone_control_(std::string_view payload) {
  if (payload.size() < 5) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("Tone-control payload shorter than minimum length"));
    return;
  }

  const int8_t sp_hp = hex_nibble_(static_cast<uint8_t>(payload[0]));
  const int8_t bass_treble = hex_nibble_(static_cast<uint8_t>(payload[1]));
  const int8_t gain_hi = hex_nibble_(static_cast<uint8_t>(payload[3]));
  const int8_t gain_lo = hex_nibble_(static_cast<uint8_t>(payload[4]));
  if (sp_hp < 0 || sp_hp > 1 || bass_treble < 0 || bass_treble > 1 || gain_hi < 0 || gain_lo < 0 ||
      hex_nibble_(payload[2]) < 0 || ((gain_hi << 4) | gain_lo) > 0x18) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("Tone-control payload contains non-hex data"));
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

void YamahaSerialComponent::parse_speaker_distance_(std::string_view payload) {
  if (payload.size() < 5) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("Speaker-distance payload shorter than minimum length"));
    return;
  }

  const int8_t channel = hex_nibble_(static_cast<uint8_t>(payload[0]));
  const int8_t unit = hex_nibble_(static_cast<uint8_t>(payload[1]));
  const int8_t d_hi = hex_nibble_(static_cast<uint8_t>(payload[2]));
  const int8_t d_mid = hex_nibble_(static_cast<uint8_t>(payload[3]));
  const int8_t d_lo = hex_nibble_(static_cast<uint8_t>(payload[4]));
  if (channel < 0 || unit < 0 || d_hi < 0 || d_mid < 0 || d_lo < 0) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("Speaker-distance payload contains non-hex data"));
    return;
  }

  if (unit != 0) {
    this->set_last_error_(YAMAHA_SERIAL_PSTR("Speaker distance unit is not meter (UNIT=%d)"), unit);
    return;
  }

  const uint16_t value = static_cast<uint16_t>((d_hi << 8) | (d_mid << 4) | d_lo);
  if (value < 30 || value > 2400) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("Speaker distance outside protocol range"));
    return;
  }
  this->publish_speaker_distance_(static_cast<uint8_t>(channel), value);
}

void YamahaSerialComponent::parse_tuner_station_(std::string_view payload) {
  if (payload.size() < 9) {
    this->record_parse_error_(YAMAHA_SERIAL_PSTR("Tuner-station payload shorter than minimum length"));
    return;
  }

  const char band = payload[2];
  const std::string freq(payload.substr(3, 6));
  if (band == '1') {
    char *end = nullptr;
    const float mhz = std::strtof(freq.c_str(), &end);
    if (end == freq.c_str() || *end != '\0' || !std::isfinite(mhz) || mhz < 76.0f || mhz > 108.0f) {
      this->record_parse_error_(YAMAHA_SERIAL_PSTR("Tuner FM frequency contains invalid data"));
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
    if (end == freq.c_str() || *end != '\0' || !std::isfinite(khz) || khz < 500.0f || khz > 1800.0f) {
      this->record_parse_error_(YAMAHA_SERIAL_PSTR("Tuner AM frequency contains invalid data"));
      return;
    }
    if (this->tuner_am_frequency_number_ != nullptr) {
      this->tuner_am_frequency_number_->publish_state(khz);
    }
    return;
  }
  this->record_parse_error_(YAMAHA_SERIAL_PSTR("Unknown tuner band"));
}

}  // namespace esphome::yamaha_serial
