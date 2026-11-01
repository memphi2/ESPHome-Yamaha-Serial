// SPDX-License-Identifier: GPL-3.0-only
#include "test_support.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  Receiver receiver;
  receiver.set_model("RX-V1600");
  receiver.setup();
  receiver.set_allow_raw_commands(true);
  for (size_t i = 0; i < size; ++i) {
    receiver.process_incoming_byte_(data[i]);
    if (i % 32 == 0) {
      test_now += (data[i] & 0x80) ? 600 : 1;
      receiver.process_command_queue_();
    }
    assert(receiver.frame_buffer_.size() <= 268);
    assert(receiver.queue_.size() <= 32);
  }
  const std::string input(reinterpret_cast<const char *>(data), size);
  receiver.command_raw_text(input);
  receiver.command_media_player_shortcut(input);
  assert(receiver.queue_.size() <= 32);
  return 0;
}
