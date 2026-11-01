// SPDX-License-Identifier: GPL-3.0-only

#include "yamaha_serial.h"

namespace esphome::yamaha_serial {

void YamahaSerialComponent::configure_command_policy_(QueuedCommand &command) const {
  const std::string_view payload(command.payload);
  bool retry_safe = false;
  if (command.frame_type == FrameType::DC4 && valid_extended_length_(payload)) {
    const auto id = parse_hex_(payload.substr(4, 3));
    if (id.has_value() && payload.size() >= 8 && (payload[7] == '0' || payload[7] == '1')) {
      command.response_type = FrameType::DC4;
      command.response_code = *id;
      if (payload[7] == '0') {
        command.selector_length = payload.size() - 8;
        retry_safe = true;
      } else {
        // These writes set absolute values. Unknown raw writes are never retried.
        switch (*id) {
          case 0x011:
            command.selector_length = 3;
            retry_safe = true;
            break;
          case 0x012:
            command.selector_length = 1;
            retry_safe = true;
            break;
          case 0x033:
          case 0x041:
          case 0x050:
            command.selector_length = 2;
            retry_safe = true;
            break;
          default:
            break;
        }
        if (command.selector_length > payload.size() - 8) {
          command.response_type = FrameType::ETX;
          retry_safe = false;
        }
      }
    }
  } else if (command.frame_type == FrameType::DC1 && payload == "000") {
    command.response_type = FrameType::DC2;
    retry_safe = true;
  } else if (command.frame_type == FrameType::STX) {
    auto report = [&](uint8_t code, uint8_t value = 0, uint8_t mask = 0) {
      command.response_type = FrameType::STX;
      command.response_code = code;
      command.response_value = value;
      command.response_mask = mask;
      retry_safe = true;
    };
    if (payload == "07E7E" || payload == "07E7F" || payload == "07EBA" || payload == "07EBB" || payload == "07A1D" ||
        payload == "07A1E") {
      report(0x20);
      command.power_on = payload == "07E7E" || payload == "07EBA" || payload == "07A1D";
    } else if (payload == "07AED") {
      command.power_on = true;
    } else if (payload == "07EA2" || payload == "07EA3") {
      report(0x23, payload == "07EA2", 0xFF);
    } else if (payload == "07EA0" || payload == "07EA1") {
      report(0x25, payload == "07EA0", 0xFF);
    } else if (payload == "07A1A" || payload == "07A1B" || payload == "07ADA" || payload == "07ADB") {
      report(payload == "07A1A" || payload == "07A1B" ? 0x26 : 0x27);
      retry_safe = false;
    } else if (payload.size() == 5 && (payload.substr(0, 3) == "230" || payload.substr(0, 3) == "231")) {
      if (const auto value = parse_hex_(payload.substr(3)); value.has_value())
        report(payload[2] == '0' ? 0x26 : 0x27, *value, 0xFF);
    } else if (payload.size() == 5 && payload.substr(0, 4) == "2200" && payload[4] >= '1' && payload[4] <= '4') {
      command.response_type = FrameType::DC1;
      command.response_code = payload[4] - '0';
      retry_safe = true;
    } else if (payload.size() == 5 && payload.substr(0, 3) == "28B") {
      if (const auto value = parse_hex_(payload.substr(3)); value.has_value())
        report(0x8B, *value, 0xFF);
    } else if (payload == "2B200" || payload == "2B201") {
      report(0xB2, payload.back() - '0', 0xFF);
    } else if (payload.size() == 5 && payload.substr(0, 4) == "2610") {
      if (const auto value = parse_hex_(payload.substr(4)); value.has_value())
        report(0x61, *value, 0x0F);
    } else {
      // Direct selectors use the same profile mappings as the entities.
      for (const auto &input : this->inputs_) {
        if (payload == input.main_command) {
          report(0x21, input.report_id, 0xFF);
          break;
        }
        if (payload == input.zone2_command) {
          report(0x24, input.zone2_report_id, 0xFF);
          break;
        }
      }
      for (const auto &program : this->programs_) {
        if (payload == program.command) {
          report(0x28, program.report_code, program.report_code == 0x80 ? 0x80 : 0x7F);
          break;
        }
      }
      struct Rule {
        const char *payload;
        uint8_t code;
        uint8_t value;
        uint8_t mask;
      };
      static const Rule rules[] = {
          {"07EA6", 0x22, 0, 0xFF},    {"07EA9", 0x22, 3, 0xFF},    {"07EAA", 0x22, 4, 0xFE}, {"07EDA", 0x22, 8, 0xFF},
          {"07EAB", 0x2E, 1, 0xFF},    {"07EAC", 0x2E, 0, 0xFF},    {"07EAD", 0x2F, 1, 0xFF}, {"07EAE", 0x2F, 0, 0xFF},
          {"07EBC", 0x35, 0, 0xFF},    {"07EBD", 0x35, 1, 0xFF},    {"07EB3", 0x2C, 4, 0xFF}, {"07EB4", 0x2C, 0, 0xFF},
          {"07EB5", 0x2C, 1, 0xFF},    {"07EB6", 0x2C, 2, 0xFF},    {"07EB7", 0x2C, 3, 0xFF}, {"07EDB", 0x5E, 0, 0xF0},
          {"07EA8", 0x5E, 0x10, 0xF0}, {"07E3B", 0x5E, 0x20, 0xF0}, {"07EB8", 0x2D, 0, 0},    {"07EB9", 0x2D, 0, 0xFF},
          {"07E7C", 0x2D, 3, 0xFF},    {"07EDC", 0x2D, 4, 0xFF},    {"07EDD", 0x2D, 5, 0xFF}, {"07EDE", 0x2D, 6, 0xFF},
          {"07E28", 0x3D, 0, 0xFF},    {"07E29", 0x3D, 1, 0xFF},    {"07E99", 0x3E, 0, 0},    {"07E9A", 0x3E, 0, 0xFF},
      };
      for (const auto &rule : rules) {
        if (payload == rule.payload) {
          report(rule.code, rule.value, rule.mask);
          break;
        }
      }
      if (payload.size() == 5 && payload.substr(0, 4) == "07AE") {
        if (const auto value = parse_hex_(payload.substr(4)); value.has_value() && *value <= 12) {
          report(*value < 5 ? 0x29 : 0x2A, *value < 5 ? *value : *value - 5, 0xFF);
        }
      }
      const bool v2500 = this->configured_model_.find("2500") != std::string::npos ||
                         this->receiver_model_.find("2500") != std::string::npos;
      if (payload == (v2500 ? "07E82" : "07E80"))
        report(0x8C, 1, 0xFF);
      if (payload == (v2500 ? "07E83" : "07E82"))
        report(0x8C, 0, 0xFF);
    }
  }
  command.expect_response = command.expect_response && command.response_type != FrameType::ETX;
  command.retries_left = command.expect_response && retry_safe ? this->max_retries_ : 0;
  if (command.power_on)
    command.timeout_ms += this->power_on_delay_ms_;
}

bool YamahaSerialComponent::response_matches_(const QueuedCommand &command, const std::vector<uint8_t> &frame) const {
  if (!command.expect_response || frame[0] != static_cast<uint8_t>(command.response_type))
    return false;
  const std::string_view payload(reinterpret_cast<const char *>(frame.data() + 1), frame.size() - 2);
  if (command.response_type == FrameType::DC2)
    return true;
  if (command.response_type == FrameType::DC1)
    return parse_hex_(payload.substr(0, 2)) == command.response_code;
  if (command.response_type == FrameType::DC4) {
    if (payload.size() < 10 || parse_hex_(payload.substr(4, 3)) != command.response_code)
      return false;
    if (payload[7] != '0')
      return true;  // A matching rejection is final, not success.
    return payload.size() >= 10U + command.selector_length &&
           payload.substr(8, command.selector_length) ==
               std::string_view(command.payload).substr(8, command.selector_length);
  }
  if (payload.size() != 6 || parse_hex_(payload.substr(2, 2)) != command.response_code)
    return false;
  if (payload[1] != '0')
    return true;
  const auto value = parse_hex_(payload.substr(4, 2));
  if (!value.has_value())
    return false;
  if (command.response_code == 0x20) {
    const bool main_on = *value == 1 || *value == 2 || *value == 4 || *value == 5;
    const bool zone2_on = *value == 1 || *value == 3 || *value == 4 || *value == 6;
    if (command.payload == "07E7E" || command.payload == "07E7F")
      return main_on == command.power_on;
    if (command.payload == "07EBA" || command.payload == "07EBB")
      return zone2_on == command.power_on;
    return main_on == command.power_on && zone2_on == command.power_on;
  }
  if (command.payload == "07EB8")
    return *value == 1 || *value == 2;
  if (command.payload == "07E99")
    return *value >= 1 && *value <= 3;
  return (*value & command.response_mask) == command.response_value;
}

}  // namespace esphome::yamaha_serial
