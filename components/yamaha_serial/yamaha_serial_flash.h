// SPDX-License-Identifier: GPL-3.0-only
#pragma once

// Constant data that should not occupy ESP8266 RAM.
//
// The ESP8266 copies .rodata, including every string literal, into RAM at boot. Tables
// marked YAMAHA_SERIAL_FLASH_TABLE and strings wrapped in YAMAHA_SERIAL_PSTR() stay in
// flash instead, and must only be read through the helpers below: flash there is not
// byte-addressable, so strlen(), strcmp() or a plain struct copy would fault. Tables
// keep their strings inline as char arrays, so one flash_read() copies a whole entry,
// strings included, into RAM. On other platforms the helpers are plain memory accesses.

#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>

#ifdef USE_ESP8266
#include <pgmspace.h>
#define YAMAHA_SERIAL_FLASH_TABLE PROGMEM
#define YAMAHA_SERIAL_PSTR(s) PSTR(s)
#else
#define YAMAHA_SERIAL_FLASH_TABLE
#define YAMAHA_SERIAL_PSTR(s) (s)
#endif

namespace esphome::yamaha_serial {

// Inline capacity, including the terminator, of a Yamaha payload in a flash table. The
// longest table payloads are ten characters, for example NET/USB "0F7F013FC0".
static constexpr size_t FLASH_COMMAND_SIZE = 11;

/// Copies one entry of a YAMAHA_SERIAL_FLASH_TABLE array into RAM.
template<typename T> inline T flash_read(const T &flash_entry) {
  static_assert(std::is_trivially_copyable<T>::value, "flash table entries are copied bytewise");
  T entry;
#ifdef USE_ESP8266
  memcpy_P(&entry, &flash_entry, sizeof(T));
#else
  std::memcpy(&entry, &flash_entry, sizeof(T));
#endif
  return entry;
}

/// Copies a flash string (YAMAHA_SERIAL_PSTR or a flash string pool) into a std::string.
inline std::string flash_string(const char *text) {
#ifdef USE_ESP8266
  std::string result(strlen_P(text), '\0');
  memcpy_P(&result[0], text, result.size());
  return result;
#else
  return std::string(text);
#endif
}

/// strcmp() with the second string in flash.
inline int flash_strcmp(const char *ram, const char *flash) {
#ifdef USE_ESP8266
  return strcmp_P(ram, flash);
#else
  return std::strcmp(ram, flash);
#endif
}

/// vsnprintf() with the format string in flash.
inline int flash_vsnprintf(char *buffer, size_t size, const char *format, va_list args) {
#ifdef USE_ESP8266
  return vsnprintf_P(buffer, size, format, args);
#else
  return std::vsnprintf(buffer, size, format, args);
#endif
}

}  // namespace esphome::yamaha_serial
