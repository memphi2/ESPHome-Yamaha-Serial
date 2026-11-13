// SPDX-License-Identifier: GPL-3.0-only
#include "yamaha_serial_catalog.h"
#include "yamaha_serial_flash.h"

#include <iterator>

namespace esphome::yamaha_serial {
namespace {
struct CatalogEntry {
  uint16_t name;
  uint16_t payload;
  uint8_t frame_type;
  uint8_t expect_response;
};
#define YAMAHA_SERIAL_CATALOG_IMPL
#include "command_catalog.generated.h"
#undef YAMAHA_SERIAL_CATALOG_IMPL
}  // namespace

bool find_named_command(const std::string &name, NamedCommand &result) {
  size_t first = 0;
  size_t last = std::size(COMMAND_ALIASES);
  while (first < last) {
    const size_t mid = first + (last - first) / 2;
    const CatalogEntry entry = flash_read(COMMAND_ALIASES[mid]);
    const int comparison = flash_strcmp(name.c_str(), COMMAND_STRINGS + entry.name);
    if (comparison < 0)
      last = mid;
    else if (comparison > 0)
      first = mid + 1;
    else {
      result.payload = flash_string(COMMAND_STRINGS + entry.payload);
      result.frame_type = entry.frame_type;
      result.expect_response = entry.expect_response;
      return true;
    }
  }
  return false;
}

}  // namespace esphome::yamaha_serial
