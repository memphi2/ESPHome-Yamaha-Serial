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
  if (configured_profile == "rx_v1900") {
    return "rx_v1900";
  }
  if (configured_profile == "rx_v1900_extended") {
    return "rx_v1900_extended";
  }

  const std::string detected_model = normalize_token_(this->receiver_model_);
  const std::string configured_model = normalize_token_(this->configured_model_);
  auto infer_profile = [](const std::string &model) -> const char * {
    if (model.find("vx800") != std::string::npos || model.find("v1800") != std::string::npos ||
        model.find("v3800") != std::string::npos || model == "htr_6190") {
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
    if (model.find("v1900") != std::string::npos) {
      return "rx_v1900_extended";
    }
    return nullptr;
  };
  if (const char *profile = infer_profile(detected_model))
    return profile;
  if (const char *profile = infer_profile(configured_model))
    return profile;
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
    this->inputs_.push_back({mapping.key, mapping.label, mapping.report_id, mapping.zone2_report_id,
                             mapping.main_command, mapping.zone2_command});
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
      ESP_LOGW(TAG, "Input key '%s' is not in profile %s", override.key.c_str(), this->active_profile_.c_str());
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
  this->profile_caps_.supports_decoder_mode = true;
  this->profile_caps_.sleep_control = true;
  this->profile_caps_.supports_speaker_relays = true;
  this->profile_caps_.tuner_band_fm_is_1 = false;
  this->profile_caps_.surround_decoder = false;
  this->profile_caps_.x700_protocol = false;

  static const StaticInputMapping INPUTS[] YAMAHA_SERIAL_FLASH_TABLE = {
      {"phono", "PHONO", 0x0, 0x0, "07A14", "07AD0"},       {"cd", "CD", 0x1, 0x1, "07A15", "07AD1"},
      {"tuner", "Tuner", 0x2, 0x2, "07A16", "07AD2"},       {"cdr", "CD-R", 0x3, 0x3, "07A19", "07AD4"},
      {"md_tape", "MD/TAPE", 0x4, 0x4, "07AC9", "07ACF"},   {"dvd", "DVD", 0x5, 0x5, "07AC1", "07ACD"},
      {"dtv_ld", "D-TV/LD", 0x6, 0x6, "07A54", "07AD9"},    {"cbl_sat", "CBL/SAT", 0x7, 0x7, "07AC0", "07ACC"},
      {"sat", "SAT", 0x8, 0x8, "07ACA", "07ACB"},           {"vcr1", "VCR1", 0x9, 0x9, "07A0F", "07AD6"},
      {"vcr2_dvr", "VCR2/DVR", 0xA, 0xA, "07A13", "07AD7"}, {"vcr3", "VCR3", 0xB, 0xB, "07AC8", "07ACE"},
      {"vaux", "V-AUX", 0xC, 0xC, "07A55", "07AD8"},
  };
  this->load_input_mappings_(INPUTS, sizeof(INPUTS) / sizeof(INPUTS[0]));

  static const StaticProgramMapping PROGRAMS[] YAMAHA_SERIAL_FLASH_TABLE = {
      {"STRAIGHT", 0x80, "07EE0"},
      {"Hall A", 0x00, "07EE1"},
      {"Hall B", 0x01, "07EE2"},
      {"Hall C", 0x02, "07EE3"},
      {"Hall USA", 0x04, "07EE4"},
      {"Vienna", 0x05, "07EE5"},
      {"Live Concert", 0x06, "07EE6"},
      {"Tokyo", 0x08, "07EE7"},
      {"Freiburg", 0x09, "07EE8"},
      {"Royaumont", 0x0A, "07EE9"},
      {"Village Gate", 0x0B, "07EEA"},
      {"Village Vanguard", 0x0C, "07EEB"},
      {"Bottom Line", 0x0E, "07EEC"},
      {"Roxy Theater", 0x10, "07EED"},
      {"Warehouse Loft", 0x12, "07EEE"},
      {"Arena", 0x13, "07EEF"},
      {"Disco", 0x14, "07EF0"},
      {"Party", 0x15, "07EF1"},
      {"Game", 0x16, "07EF2"},
      {"7ch Stereo", 0x17, "07EFF"},
      {"Music Pop", 0x18, "07EF3"},
      {"DJ", 0x19, "07EF4"},
      {"Classical/Opera", 0x1A, "07EF5"},
      {"Pavillion", 0x1D, "07EF6"},
      {"Mono Movie", 0x20, "07EF7"},
      {"Sports", 0x21, "07EF8"},
      {"Spectacle", 0x24, "07EF9"},
      {"Sci-Fi", 0x25, "07EFA"},
      {"Adventure", 0x28, "07EFB"},
      {"General", 0x29, "07EFC"},
      {"ProLogic", 0x2C, "07EFD"},
      {"Standard", 0x2D, "07EFE"},
      {"PLII Movie", 0x30, "07E67"},
      {"PLII Music", 0x31, "07E68"},
      {"Neo:6 Cinema", 0x32, "07E69"},
      {"Neo:6 Music", 0x33, "07E6A"},
      {"2ch Stereo", 0x34, "07EC0"},
      {"2ch Direct Stereo", 0x35, "07EC1"},
      {"THX Cinema", 0x36, "07EC2"},
      {"THX Music", 0x37, "07EC3"},
      {"PLII Game", 0x38, "07EC7"},
      {"THX Game", 0x3C, "07EC8"},
  };
  this->load_program_mappings_(PROGRAMS, sizeof(PROGRAMS) / sizeof(PROGRAMS[0]));

  static const StaticSceneMapping SCENES[] YAMAHA_SERIAL_FLASH_TABLE = {
      {"A", "07E35"}, {"B", "07E36"}, {"C", "07E37"}, {"D", "07E38"}, {"E", "07E39"}, {"F", "07E3A"},
  };
  this->load_scene_mappings_(SCENES, sizeof(SCENES) / sizeof(SCENES[0]));
}

void YamahaSerialComponent::apply_profile_rx_vx600_extended_() {
  this->apply_profile_rx_vx600_();
  this->profile_caps_.supports_fan_mode = true;
  this->profile_caps_.supports_extended = true;
  this->profile_caps_.decode_dc2_layout = true;
}

void YamahaSerialComponent::apply_profile_rx_vx500_extended_() { this->apply_profile_rx_vx500_(); }

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
  this->profile_caps_.supports_decoder_mode = true;
  this->profile_caps_.sleep_control = true;
  this->profile_caps_.supports_speaker_relays = true;
  this->profile_caps_.tuner_band_fm_is_1 = false;
  this->profile_caps_.surround_decoder = false;
  this->profile_caps_.x700_protocol = false;

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
  this->profile_caps_.supports_decoder_mode = true;
  this->profile_caps_.sleep_control = true;
  this->profile_caps_.supports_speaker_relays = true;
  this->profile_caps_.tuner_band_fm_is_1 = false;
  this->profile_caps_.surround_decoder = false;
  // DC2 layout, format/sampling tables and memory-recall reports per the RX-Vx700
  // RS-232C manual. Untested on x700 hardware.
  this->profile_caps_.x700_protocol = true;

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
      {"STRAIGHT", 0x80, "07EE0"},
      {"Munich", 0x00, "07EE1"},
      {"Vienna", 0x05, "07EE5"},
      {"Amsterdam", 0x07, "07EE6"},
      {"Freiburg", 0x09, "07EE8"},
      {"Chamber", 0x0B, "07EAF"},
      {"Village Vanguard", 0x0D, "07EEB"},
      {"Warehouse Loft", 0x11, "07EEE"},
      {"Cellar Club", 0x0F, "07ECD"},
      {"The Bottom Line", 0x0E, "07EEC"},
      {"The Roxy Theatre", 0x10, "07EED"},
      {"Disco", 0x14, "07EF0"},
      {"Game", 0x16, "07EF2"},
      {"7ch Stereo", 0x17, "07EFF"},
      {"2ch Stereo", 0x34, "07EC0"},
      {"Sports", 0x21, "07EF8"},
      {"Action Game", 0x1E, "07EF2"},
      {"Roleplaying Game", 0x1F, "07ECE"},
      {"Music Video", 0x18, "07EF3"},
      {"Recital/Opera", 0x1C, "07EF5"},
      {"Standard", 0x2D, "07EFE"},
      {"Spectacle", 0x24, "07EF9"},
      {"Sci-Fi", 0x25, "07EFA"},
      {"Adventure", 0x28, "07EFB"},
      {"Drama", 0x29, "07EFC"},
      {"Mono Movie", 0x20, "07EF7"},
      {"Surround Decode", 0x2C, "07EFD"},
      {"THX Cinema", 0x36, "07EC2"},
      {"THX Music", 0x37, "07EC3"},
      {"THX Game", 0x3C, "07EC8"},
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
  this->profile_caps_.supports_decoder_mode = true;
  this->profile_caps_.sleep_control = true;
  this->profile_caps_.supports_speaker_relays = true;
  this->profile_caps_.tuner_band_fm_is_1 = false;
  this->profile_caps_.surround_decoder = false;
  // x700 protocol generation; untested on x800 hardware.
  this->profile_caps_.x700_protocol = true;

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
      {"STRAIGHT", 0x80, "07EE0"},
      {"Munich", 0x00, "07EE1"},
      {"Vienna", 0x05, "07EE5"},
      {"Amsterdam", 0x07, "07EE6"},
      {"Freiburg", 0x09, "07EE8"},
      {"Chamber", 0x0B, "07EAF"},
      {"Village Vanguard", 0x0D, "07EEB"},
      {"Warehouse Loft", 0x11, "07EEE"},
      {"Cellar Club", 0x0F, "07ECD"},
      {"The Bottom Line", 0x0E, "07EEC"},
      {"The Roxy Theatre", 0x10, "07EED"},
      {"Disco", 0x14, "07EF0"},
      {"Game", 0x16, "07EF2"},
      {"7ch Stereo", 0x17, "07EFF"},
      {"2ch Stereo", 0x34, "07EC0"},
      {"Sports", 0x21, "07EF8"},
      {"Action Game", 0x1E, "07EF2"},
      {"Roleplaying Game", 0x1F, "07ECE"},
      {"Music Video", 0x18, "07EF3"},
      {"Recital/Opera", 0x1C, "07EF5"},
      {"Standard", 0x2D, "07EFE"},
      {"Spectacle", 0x24, "07EF9"},
      {"Sci-Fi", 0x25, "07EFA"},
      {"Adventure", 0x28, "07EFB"},
      {"Drama", 0x29, "07EFC"},
      {"Mono Movie", 0x20, "07EF7"},
      {"Surround Decode", 0x2C, "07EFD"},
      {"THX Cinema", 0x36, "07EC2"},
      {"THX Music", 0x37, "07EC3"},
      {"THX Game", 0x3C, "07EC8"},
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

void YamahaSerialComponent::apply_profile_rx_v1900_() {
  this->profile_caps_.supports_zone2 = true;
  this->profile_caps_.supports_scene = true;
  this->profile_caps_.supports_audio_select = true;
  this->profile_caps_.supports_night_mode = false;
  this->profile_caps_.supports_fan_mode = false;
  this->profile_caps_.supports_pure_direct = true;
  this->profile_caps_.supports_extended = false;
  this->profile_caps_.decode_dc2_layout = false;
  this->profile_caps_.supports_decoder_mode = false;
  this->profile_caps_.sleep_control = false;
  this->profile_caps_.supports_speaker_relays = false;
  this->profile_caps_.tuner_band_fm_is_1 = true;
  this->profile_caps_.surround_decoder = true;
  this->profile_caps_.x700_protocol = true;

  // Single MD/CD-R input (0x03); 0x04 MD/TAPE is never reported.
  static const StaticInputMapping INPUTS[] YAMAHA_SERIAL_FLASH_TABLE = {
      {"phono", "PHONO", 0x00, 0x00, "07A14", "07AD0"},
      {"cd", "CD", 0x01, 0x01, "07A15", "07AD1"},
      {"tuner", "Tuner", 0x02, 0x02, "07A16", "07AD2"},
      {"cdr", "MD/CD-R", 0x03, 0x03, "07A19", "07AD4"},
      {"dvd", "DVD", 0x05, 0x05, "07AC1", "07ACD"},
      // Reports 0x11 in the main zone but 0x0F in zone 2.
      {"bd_hd_dvd", "BD/HD DVD", 0x11, 0x0F, "07AC8", "07ACE"},
      {"tv", "TV", 0x06, 0x06, "07A54", "07AD9"},
      {"cbl_sat", "CBL/SAT", 0x07, 0x07, "07AC0", "07ACC"},
      {"vcr", "VCR", 0x09, 0x09, "07A0F", "07AD6"},
      {"dvr", "DVR", 0x0A, 0x0A, "07A13", "07AD7"},
      {"vaux", "V-AUX", 0x0C, 0x0C, "07A55", "07AD8"},
      {"usb", "USB", 0x0D, 0x0D, "0F7F013FC0", "0F7F0140BF"},
      {"multi_ch", "Multi CH", 0x10, 0xFF, "07A87", ""},
      // Reports 0x12 in the main zone but 0x08 in zone 2.
      {"dock", "DOCK", 0x12, 0x08, "0F7F014AB5", "0F7F014BB4"},
  };
  this->load_input_mappings_(INPUTS, sizeof(INPUTS) / sizeof(INPUTS[0]));

  // Front-panel cycle order, which program next/previous follows. 7ch Enhancer is only
  // accepted in the long remote-code form; the short 07E14 form gets no reply.
  static const StaticProgramMapping PROGRAMS[] YAMAHA_SERIAL_FLASH_TABLE = {
      {"STRAIGHT", 0x80, "07EE0"},
      {"Sci-Fi", 0x25, "07EFA"},
      {"Adventure", 0x28, "07EFB"},
      {"Drama", 0x29, "07EFC"},
      {"Mono Movie", 0x20, "07EF7"},
      {"2ch Stereo", 0x34, "07EC0"},
      {"7ch Stereo", 0x17, "07EFF"},
      {"Straight Enhancer", 0x44, "07ED8"},
      {"7ch Enhancer", 0x45, "0F7E81146B"},
      {"Surround Decode", 0x2C, "07EFD"},
      {"Munich", 0x00, "07EE1"},
      {"Vienna", 0x05, "07EE5"},
      {"Amsterdam", 0x07, "07EE6"},
      {"Freiburg", 0x09, "07EE8"},
      {"Chamber", 0x0B, "07EAF"},
      {"Village Vanguard", 0x0D, "07EEB"},
      {"Warehouse Loft", 0x11, "07EEE"},
      {"Cellar Club", 0x0F, "07ECD"},
      {"The Roxy Theatre", 0x10, "07EED"},
      {"The Bottom Line", 0x0E, "07EEC"},
      {"Sports", 0x21, "07EF8"},
      {"Action Game", 0x1E, "07EF2"},
      {"Roleplaying Game", 0x1F, "07ECE"},
      {"Music Video", 0x18, "07EF3"},
      {"Recital/Opera", 0x1C, "07EF5"},
      {"Standard", 0x2D, "07EFE"},
      {"Spectacle", 0x24, "07EF9"},
  };
  this->load_program_mappings_(PROGRAMS, sizeof(PROGRAMS) / sizeof(PROGRAMS[0]));

  // System Memory is numbered 1-6 on the RX-V1900.
  static const StaticSceneMapping SCENES[] YAMAHA_SERIAL_FLASH_TABLE = {
      {"1", "07E35"}, {"2", "07E36"}, {"3", "07E37"}, {"4", "07E38"}, {"5", "07E39"}, {"6", "07E3A"},
  };
  this->load_scene_mappings_(SCENES, sizeof(SCENES) / sizeof(SCENES[0]));
}

void YamahaSerialComponent::apply_profile_rx_v1900_extended_() {
  this->apply_profile_rx_v1900_();
  this->profile_caps_.supports_extended = true;
  this->profile_caps_.decode_dc2_layout = true;
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
  } else if (profile_id == "rx_v1900") {
    this->apply_profile_rx_v1900_();
  } else if (profile_id == "rx_v1900_extended") {
    this->apply_profile_rx_v1900_extended_();
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
  this->configure_scene_select_options_();
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
  this->configure_scene_select_options_();
}

void YamahaSerialComponent::configure_input_select_options_() {
  if (this->input_source_select_ == nullptr && this->zone2_input_source_select_ == nullptr) {
    return;
  }
  // Only inputs with a select command are offered; a reported input without one still
  // shows through the input text sensors.
  std::vector<std::string> labels{};
  labels.reserve(this->inputs_.size());
  if (this->input_source_select_ != nullptr) {
    for (const auto &input : this->inputs_) {
      if (!input.main_command.empty()) {
        labels.push_back(input.label);
      }
    }
    this->api_metadata_dirty_ |= this->input_source_select_->set_options_from_labels(labels);
  }
  if (this->zone2_input_source_select_ != nullptr) {
    labels.clear();
    for (const auto &input : this->inputs_) {
      if (this->profile_caps_.supports_zone2 && !input.zone2_command.empty()) {
        labels.push_back(input.label);
      }
    }
    this->api_metadata_dirty_ |= this->zone2_input_source_select_->set_options_from_labels(labels);
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
  this->api_metadata_dirty_ |= this->program_select_->set_options_from_labels(labels);
}

void YamahaSerialComponent::configure_scene_select_options_() {
  if (this->scene_select_ == nullptr || !this->profile_initialized_) {
    return;
  }
  std::vector<std::string> labels;
  labels.reserve(this->scenes_.size());
  for (const auto &scene : this->scenes_) {
    labels.push_back(scene.label);
  }
  this->api_metadata_dirty_ |= this->scene_select_->set_options_from_labels(labels);
}

}  // namespace esphome::yamaha_serial
