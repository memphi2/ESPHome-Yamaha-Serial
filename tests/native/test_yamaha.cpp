#include "yamaha_serial.h"
#include <cassert>

using namespace esphome;
class Receiver : public yamaha_serial::YamahaSerialComponent {
 public:
  using YamahaSerialComponent::process_incoming_byte_;
  using YamahaSerialComponent::process_command_queue_;
  using YamahaSerialComponent::queue_probe_ready_;
  using YamahaSerialComponent::queue_poll_commands_;
  using YamahaSerialComponent::apply_profile_;
  using YamahaSerialComponent::receiver_reported_;
  using YamahaSerialComponent::available_;
  using YamahaSerialComponent::parse_errors_;
  using YamahaSerialComponent::responses_received_;
  using YamahaSerialComponent::last_response_ms_;
  using YamahaSerialComponent::last_error_;
  using YamahaSerialComponent::in_flight_;
  using YamahaSerialComponent::queue_;
  using YamahaSerialComponent::timeouts_;
  using YamahaSerialComponent::queue_drops_;
  using YamahaSerialComponent::parse_volume_text_db_;
  using YamahaSerialComponent::active_profile_;
  using YamahaSerialComponent::inputs_;
  using YamahaSerialComponent::queue_named_command_;
  using YamahaSerialComponent::normalize_token_;
  using YamahaSerialComponent::handle_detected_model_;
  void feed(const std::string &s) { for (uint8_t byte : s) process_incoming_byte_(byte); }
};

std::string extended(const std::string &body) {
  unsigned sum = 0;
  for (unsigned char c : body) sum += c;
  char checksum[3];
  snprintf(checksum, sizeof(checksum), "%02X", sum & 255);
  return std::string(1, '\x14') + body + checksum + '\x03';
}

int main() {
  Receiver receiver;
  receiver.apply_profile_("rx_vx600_extended");
  receiver.queue_poll_commands_();
  assert(receiver.queue_.empty()); // No polling before a valid receiver report.
  receiver.command_mute(true);
  receiver.process_command_queue_();
  assert(receiver.in_flight_.has_value());
  auto valid = extended("200F0000F08RX-V1600");
  auto bad = valid;
  bad[bad.size() - 2] ^= 1;
  receiver.feed(bad);
  assert(receiver.parse_errors_ == 1);
  assert(receiver.in_flight_.has_value());
  assert(!receiver.available_ && !receiver.receiver_reported_);
  assert(receiver.responses_received_ == 0 && receiver.last_response_ms_ == 0);
  receiver.feed(valid.substr(0, 5));
  assert(receiver.responses_received_ == 0);
  receiver.feed(valid.substr(5) + std::string("\x02") + "002002\x03");
  assert(receiver.responses_received_ == 2);
  assert(receiver.available_ && receiver.receiver_reported_);
  assert(!receiver.in_flight_.has_value());
  // Valid error replies must remain visible after dispatch.
  receiver.feed(extended("20040331"));
  assert(receiver.last_error_.find("Extended command status error") == 0);
  receiver.feed(std::string("\x02") + "002600\x03");
  assert(receiver.last_error_.empty());

  Receiver queue;
  queue.set_command_timeout(100);
  queue.set_command_spacing(20);
  queue.set_max_retries(1);
  queue.command_mute(true);
  queue.process_command_queue_();
  assert(queue.sent.size() == 1);
  test_now += 101;
  queue.process_command_queue_();
  assert(queue.sent.size() == 2);
  test_now += 101;
  queue.process_command_queue_();
  assert(queue.timeouts_ == 1 && !queue.in_flight_);
  for (int i = 0; i < 40; ++i) queue.command_mute(true);
  assert(queue.queue_.size() == 32 && queue.queue_drops_ == 8);
  Receiver probes;
  for (int i = 0; i < 100; ++i) probes.queue_probe_ready_();
  assert(probes.queue_.size() == 1);
  // Interrupted/oversized frames recover at the next start marker.
  probes.feed(std::string("\x02") + "abc" + '\x02' + "002002\x03");
  probes.feed(std::string(1, '\x02') + std::string(193, '0'));
  probes.feed(std::string("\x02") + "002002\x03");
  assert(probes.parse_errors_ == 2 && probes.responses_received_ == 2);

  Receiver step;
  step.set_command_timeout(1);
  step.set_command_spacing(100);
  step.command_volume_step(true, false);
  step.process_command_queue_();
  test_now += 101;
  step.process_command_queue_();
  assert(step.sent.size() == 1 && step.timeouts_ == 1);
  step.command_volume_absolute(NAN, false);
  assert(step.queue_.empty());
  assert(!Receiver::parse_volume_text_db_("nan"));
  assert(!Receiver::parse_volume_text_db_("12junk dB"));
  assert(Receiver::parse_volume_text_db_(" -40.5 dB ") == -40.5f);

  Receiver spacing;
  spacing.set_command_timeout(1);
  spacing.set_command_spacing(200);
  spacing.command_mute(true);
  spacing.process_command_queue_();
  test_now += 2;
  spacing.process_command_queue_();
  assert(spacing.sent.size() == 1);
  test_now += 200;
  spacing.process_command_queue_();
  assert(spacing.sent.size() == 2);

  const char *models[] = {"RX-V1500", "RX-V1600", "RX-V1700", "RX-V1800"};
  const char *profiles[] = {"rx_vx500_extended", "rx_vx600_extended", "rx_vx700_extended", "rx_vx800_extended"};
  for (int i = 0; i < 4; ++i) {
    Receiver model;
    model.set_model(models[i]);
    yamaha_serial::YamahaSelect main_input;
    yamaha_serial::YamahaSelect zone2_input;
    model.set_input_source_select(&main_input);
    model.set_zone2_input_source_select(&zone2_input);
    model.setup();
    assert(model.active_profile_ == profiles[i]);
    model.command_input("md_tape", false);
    assert(model.queue_.back().payload == (i == 0 ? "07AC9" : "07A18"));
    const auto input_count = model.inputs_.size();
    model.add_input_mapping("md_tape", "Recorder");
    assert(model.inputs_.size() == input_count);
    model.command_input("Recorder", false);
    assert(model.queue_.back().payload == (i == 0 ? "07AC9" : "07A18"));
    model.add_input_mapping("not_an_input", "Invalid");
    assert(model.inputs_.size() == input_count);
    assert(!main_input.has_option("Invalid") && !zone2_input.has_option("Invalid"));
    for (const auto &input : model.inputs_) {
      assert(main_input.has_option(input.label) == !input.main_command.empty());
      assert(zone2_input.has_option(input.label) == !input.zone2_command.empty());
    }
    for (bool standard : {false, true}) {
      Receiver explicit_profile;
      const std::string profile = standard ? std::string(profiles[i]).substr(0, 8) : profiles[i];
      explicit_profile.set_model("unknown");
      explicit_profile.set_receiver_profile(profile);
      explicit_profile.setup();
      assert(explicit_profile.active_profile_ == profile);
      explicit_profile.handle_detected_model_(models[(i + 1) % 4]);
      assert(explicit_profile.active_profile_ == profile);
    }
  }
  Receiver detected;
  detected.setup();
  detected.add_input_mapping("md_tape", "Recorder");
  detected.handle_detected_model_("RX-V1600");
  assert(detected.active_profile_ == "rx_vx600_extended");
  detected.command_input("Recorder", false);
  assert(detected.queue_.back().payload == "07A18");
  Receiver aliases;
  assert(aliases.queue_named_command_("input_md_tape"));
  assert(aliases.queue_.back().payload == "07A18");
  assert(aliases.queue_named_command_("input md tape x500"));
  assert(aliases.queue_.back().payload == "07AC9");
  assert(Receiver::normalize_token_("rx_vx600_extended") == "rx_vx600_extended");
  assert(Receiver::normalize_token_(" __MD / TAPE__ ") == "md_tape");
}
