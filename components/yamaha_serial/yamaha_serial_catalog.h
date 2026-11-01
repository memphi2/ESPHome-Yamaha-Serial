// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <cstdint>
#include <string>

namespace esphome::yamaha_serial {

struct NamedCommand {
  std::string payload;
  uint8_t frame_type;
  bool expect_response;
};

bool find_named_command(const std::string &name, NamedCommand &result);

}  // namespace esphome::yamaha_serial
