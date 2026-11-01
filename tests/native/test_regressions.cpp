// SPDX-License-Identifier: GPL-3.0-only
#include "test_support.h"
#include "yamaha_serial_catalog.h"
#include <fstream>

using namespace esphome::yamaha_serial;
using FrameType = Receiver::FrameType;

static std::string status(const std::string &body) { return '\x02' + body + '\x03'; }
static void ready(Receiver &r) {
  r.set_model("RX-V1600");
  r.setup();
  r.receiver_reported_ = true;
  r.available_ = true;
  r.queue_.clear();
  test_now += 10000;
}

void test_regressions() {
  Receiver matching;
  ready(matching);
  matching.command_mute(true);
  matching.process_command_queue_();
  matching.feed(status("001103")); // Sampling rate is not a mute acknowledgement.
  assert(matching.in_flight_);
  matching.feed(status("002300")); // Opposite mute state is not the requested state.
  assert(matching.in_flight_);
  matching.feed(status("002301"));
  assert(!matching.in_flight_);

  Receiver ext;
  ready(ext);
  ext.command_speaker_distance(2, 100);
  ext.process_command_queue_();
  ext.feed(extended("2009041030064")); // Same command, different channel.
  assert(ext.in_flight_);
  ext.feed(extended("2009041020064"));
  assert(!ext.in_flight_);

  Receiver polls;
  ready(polls);
  polls.queue_simple_command_(FrameType::DC4, "2006041020", "poll_distance");
  polls.queue_simple_command_(FrameType::DC4, "2006041030", "poll_distance");
  polls.queue_simple_command_(FrameType::DC4, "2006041030", "poll_distance");
  assert(polls.queue_.size() == 2);
  polls.command_speaker_distance(3, 100);
  assert(polls.queue_.size() == 4); // A write needs a new readback after it.

  Receiver offline;
  ready(offline);
  offline.set_max_retries(0);
  offline.command_mute(true);
  offline.process_command_queue_();
  test_now += 2000;
  offline.process_command_queue_();
  assert(!offline.available_ && !offline.receiver_reported_);
  offline.check_availability_();
  assert(!offline.available_);
  offline.feed(status("002301"));
  assert(offline.available_ && offline.receiver_reported_);

  Receiver active;
  ready(active);
  active.set_max_retries(0);
  active.command_mute(true);
  active.process_command_queue_();
  active.feed(status("001103"));
  test_now += 2000;
  active.process_command_queue_();
  assert(active.timeouts_ == 1 && active.available_);

  Receiver aliases;
  ready(aliases);
  aliases.set_allow_raw_commands(true);
  aliases.command_raw_text("volume_up");
  assert(aliases.queue_.back().retries_left == 0);
  aliases.queue_.clear();
  aliases.command_raw_text("power_on");
  aliases.process_command_queue_();
  assert(aliases.command_hold_until_ms_ == test_now + 2000);
  assert(aliases.in_flight_->command.timeout_ms == 3500);
  aliases.feed(status("002002"));
  aliases.command_mute(true);
  test_now += 200;
  aliases.process_command_queue_();
  assert(aliases.sent.size() == 1);

  Receiver raw;
  ready(raw);
  raw.command_raw_text("power_on");
  raw.command_media_player_shortcut("yamaha://raw/power_on");
  assert(raw.queue_.empty());
  raw.set_allow_raw_commands(true);
  raw.command_raw_text("dc4:2011011100508Hi Fi PC");
  assert(raw.queue_.back().payload == "2011011100508Hi Fi PC");
  raw.command_media_player_shortcut("yamaha://raw/dc4:2011011100508Hi Fi   ");
  assert(raw.queue_.back().payload == "2011011100508Hi Fi   ");
  raw.command_raw_text("dc4:200499910"); // Bad length, rejected.
  assert(raw.queue_.size() == 2);
  raw.command_raw_text("dc4:20049991"); // Unknown write, no unsafe repeat.
  assert(raw.queue_.back().retries_left == 0);
  const auto count = raw.queue_.size();
  raw.command_raw_text(std::string(321, 'a'));
  assert(raw.queue_.size() == count);
  const std::string large_body = "20009990" + std::string(252, 'A');
  raw.command_raw_text("dc4:" + large_body);
  assert(raw.queue_.back().payload.size() == 260);
  raw.feed(extended(large_body));
  assert(raw.parse_errors_ == 0 && raw.responses_received_ == 1);
  raw.feed(extended(large_body + 'A'));
  assert(raw.parse_errors_ == 1);
  text_sensor::TextSensor parse_error;
  raw.set_last_parse_error_text_sensor(&parse_error);
  raw.record_parse_error_("Rejected packet", std::vector<uint8_t>(268, '0'));
  assert(parse_error.state.size() <= 255);
  raw.set_last_error_(std::string(400, 'a'));
  assert(raw.last_error_.size() <= 255);

  Receiver volume;
  ready(volume);
  volume.set_volume_config(-60, -10, 2);
  volume.command_volume_step(true, false);
  assert(volume.queue_.empty());
  volume.feed(status("002670")); // -43 dB.
  volume.command_volume_step(true, false);
  volume.command_volume_step(true, false);
  assert(volume.queue_[0].payload == "23074" && volume.queue_[1].payload == "23078");
  for (float invalid : {NAN, INFINITY, -INFINITY}) {
    volume.command_volume_absolute(invalid, false);
    volume.command_dimmer_percent(invalid);
    volume.command_bass_percent(invalid);
    volume.command_treble_percent(invalid);
    volume.command_tuner_frequency(invalid, true);
  }
  assert(volume.queue_.size() == 2);
  volume.command_volume_absolute(0, false);
  assert(volume.queue_.back().payload == "230B3"); // Configured -10 dB ceiling.

  Receiver tuner;
  tuner.set_receiver_profile("rx_vx500");
  tuner.setup();
  tuner.queue_.clear();
  tuner.command_tuner_preset(1);
  assert(tuner.queue_.back().payload == "07AE5");
  YamahaNumber frequency;
  tuner.set_tuner_fm_frequency_number(&frequency);
  tuner.parse_tuner_station_("00112junk");
  assert(tuner.parse_errors_ == 1 && !frequency.has_state());
  tuner.parse_tuner_station_("001098.50");
  assert(frequency.state == 98.5f);

  Receiver reports;
  ready(reports);
  YamahaSelect surround, audio;
  surround.set_options_from_labels({"Off", "EX/ES", "Auto", "EX", "PLIIx Movie", "PLIIx Music"});
  audio.set_options_from_labels({"Auto", "Coax/Opt", "Analog", "HDMI"});
  reports.set_extended_surround_select(&surround);
  reports.set_audio_select_select(&audio);
  reports.feed(status("002D01"));
  assert(surround.current_option() == "EX/ES");
  reports.feed(status("002D06"));
  assert(surround.current_option() == "PLIIx Music");
  reports.feed(status("002205"));
  assert(audio.current_option() == "Analog");

  api::APIServer server;
  server.clients.emplace_back(std::make_unique<api::TestClient>());
  api::global_api_server = &server;
  Receiver profiles;
  YamahaSelect input;
  profiles.set_input_source_select(&input);
  profiles.setup();
  input.publish_state("VCR3");
  profiles.handle_detected_model_("R0210");
  assert(profiles.active_profile_ == "rx_vx700_extended");
  assert(!input.has_state()); // Removed option must not leave an invalid active index.
  profiles.refresh_api_metadata_();
  assert(server.clients[0]->requests == 1);
  profiles.handle_detected_model_("R0210");
  profiles.refresh_api_metadata_();
  assert(server.clients[0]->requests == 1);
  api::global_api_server = nullptr;
  Receiver unknown;
  ready(unknown);
  unknown.handle_detected_model_("R9999");
  assert(unknown.active_profile_ == "rx_vx600_extended");
  unknown.handle_detected_model_("RX-V9999");
  assert(unknown.active_profile_ == "rx_vx600_extended");

  Receiver truncated;
  ready(truncated);
  truncated.feed(std::string("\x02") + "002");
  test_now += 600;
  truncated.loop();
  assert(truncated.parse_errors_ == 1 && truncated.frame_buffer_.empty());

  Receiver dc2;
  ready(dc2);
  const std::string config_body = "R0210A0A0000000003";
  auto config = extended(config_body);
  config[0] = '\x12';
  dc2.feed(config);
  assert(dc2.parse_errors_ == 0 && dc2.receiver_model_ == "RX-V1700");
  config[config.size() - 2] ^= 1;
  dc2.feed(config);
  assert(dc2.parse_errors_ == 1 && dc2.responses_received_ == 1);

  Receiver diagnostics;
  ready(diagnostics);
  sensor::Sensor age, errors;
  diagnostics.set_last_response_age_sensor(&age);
  diagnostics.set_parse_errors_sensor(&errors);
  test_now = 60000;
  diagnostics.publish_diagnostics_();
  assert(age.publishes == 1 && errors.publishes == 1);
  test_now += 1000;
  diagnostics.publish_diagnostics_();
  assert(age.publishes == 1);
  test_now += 59000;
  diagnostics.publish_diagnostics_();
  assert(age.publishes == 2 && errors.publishes == 1);

  std::ifstream catalog("aliases.tsv");
  assert(catalog.is_open());
  std::string name, payload, frame;
  unsigned checked = 0;
  while (std::getline(catalog, name, '\t') && std::getline(catalog, payload, '\t') && std::getline(catalog, frame)) {
    NamedCommand found;
    assert(find_named_command(name, found));
    assert(found.payload == payload && found.frame_type == std::stoi(frame));
    ++checked;
  }
  assert(checked == 666);
}
