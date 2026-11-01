// SPDX-License-Identifier: GPL-3.0-only
#include "yamaha_serial_catalog.h"

#include <cstring>
#include <iterator>
#ifdef ARDUINO_ARCH_ESP8266
#include <pgmspace.h>
#define YAMAHA_SERIAL_FLASH_TABLE PROGMEM
#else
#define YAMAHA_SERIAL_FLASH_TABLE
#endif

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
    CatalogEntry entry;
#ifdef ARDUINO_ARCH_ESP8266
    memcpy_P(&entry, &COMMAND_ALIASES[mid], sizeof(entry));
    const int comparison = strcmp_P(name.c_str(), COMMAND_STRINGS + entry.name);
#else
    entry = COMMAND_ALIASES[mid];
    const int comparison = std::strcmp(name.c_str(), COMMAND_STRINGS + entry.name);
#endif
    if (comparison < 0)
      last = mid;
    else if (comparison > 0)
      first = mid + 1;
    else {
#ifdef ARDUINO_ARCH_ESP8266
      const size_t length = strlen_P(COMMAND_STRINGS + entry.payload);
      result.payload.resize(length);
      memcpy_P(result.payload.data(), COMMAND_STRINGS + entry.payload, length);
#else
      result.payload = COMMAND_STRINGS + entry.payload;
#endif
      result.frame_type = entry.frame_type;
      result.expect_response = entry.expect_response;
      return true;
    }
  }
  return false;
}

}  // namespace esphome::yamaha_serial
