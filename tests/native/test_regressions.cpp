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
  for (const std::string profile :
       {"rx_vx500", "rx_vx500_extended", "rx_vx600", "rx_vx600_extended", "rx_vx700", "rx_vx700_extended",
        "rx_vx800", "rx_vx800_extended", "rx_v1900", "rx_v1900_extended"}) {
    Receiver mapped;
    mapped.set_receiver_profile(profile);
    ready(mapped);
    assert(mapped.active_profile_ == profile);
    auto acknowledge = [&](uint8_t report, uint8_t value) {
      test_now += 200;
      mapped.process_command_queue_();
      assert(mapped.in_flight_ && mapped.in_flight_->command.expect_response);
      char body[7];
      snprintf(body, sizeof(body), "00%02X%02X", static_cast<unsigned>(report), static_cast<unsigned>(value));
      mapped.feed(status(body));
      assert(!mapped.in_flight_ && mapped.timeouts_ == 0 && mapped.available_);
    };
    for (const auto &program : mapped.programs_) {
      if (program.command.empty()) continue;  // Report-only program.
      mapped.command_program(program.label);
      acknowledge(0x28, program.report_code);
    }
    for (const auto &input : mapped.inputs_) {
      for (bool zone2 : {false, true}) {
        if ((zone2 ? input.zone2_command : input.main_command).empty()) continue;
        mapped.command_input(input.label, zone2);
        acknowledge(zone2 ? 0x24 : 0x21, zone2 ? input.zone2_report_id : input.report_id);
      }
    }
  }
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
  ext.feed(extended("1009041020064")); // Different DC4 software/kind cannot acknowledge an Extended write.
  assert(ext.in_flight_);
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

  // Full-length DC2 CONFIG: data[8] is power, data[9..10] are frame bytes 18..19.
  auto full_config = [](const std::string &model, const std::string &input_bytes) {
    std::string data(128, '0');
    data[8] = '1';
    data.replace(9, 2, input_bytes);
    auto frame = extended(model + "A80" + data);
    frame[0] = '\x12';
    return frame;
  };
  // RX-V1900 (R0240) auto-selects its profile and decodes a two-nibble input ID.
  Receiver v1900;
  ready(v1900);
  v1900.feed(full_config("R0240", "0D"));
  assert(v1900.parse_errors_ == 0 && v1900.receiver_model_ == "RX-V1900");
  assert(v1900.active_profile_ == "rx_v1900_extended");
  assert(v1900.input_label_ == "USB");
  v1900.feed(full_config("R0240", "12"));
  assert(v1900.input_label_ == "DOCK");
  v1900.feed(full_config("R0240", "10"));
  assert(v1900.input_label_ == "Multi CH");
  // RX-V1900 input reports verified on hardware: MD/CD-R 0x03; BD/HD DVD 0x11 main, 0x0F zone 2.
  v1900.feed(full_config("R0240", "03"));
  assert(v1900.input_label_ == "MD/CD-R");
  v1900.feed(full_config("R0240", "11"));
  assert(v1900.input_label_ == "BD/HD DVD");
  v1900.feed(status("402111"));
  assert(v1900.input_label_ == "BD/HD DVD");
  v1900.feed(status("00240F"));
  assert(v1900.zone2_input_label_ == "BD/HD DVD");
  v1900.feed(status("402104"));
  assert(v1900.input_label_ == "BD/HD DVD");  // 0x04 MD/TAPE is not an RX-V1900 input.
  v1900.feed(status("40210F"));
  assert(v1900.input_label_ == "BD/HD DVD");  // 0x0F is BD/HD DVD in zone 2 only.
  // BD/HD DVD commands are acknowledged by the replies captured on hardware.
  v1900.queue_.clear();
  v1900.command_input("bd_hd_dvd", false);
  test_now += 200;
  v1900.process_command_queue_();
  assert(v1900.in_flight_ && v1900.in_flight_->command.payload == "07AC8");
  v1900.feed(status("002111"));
  assert(!v1900.in_flight_ && v1900.timeouts_ == 0);
  v1900.command_input("bd_hd_dvd", true);
  test_now += 200;
  v1900.process_command_queue_();
  assert(v1900.in_flight_ && v1900.in_flight_->command.payload == "07ACE");
  v1900.feed(status("00240F"));
  assert(!v1900.in_flight_ && v1900.timeouts_ == 0);
  // Tuner band: x500-x800 report 00 = FM, 01 = AM (manuals); the RX-V1900 reports the reverse
  // (measured). x700/x800 must not inherit the RX-V1900 reversal (maintainer reproduction).
  for (const std::string profile : {"rx_vx600_extended", "rx_vx700_extended", "rx_vx800_extended",
                                    "rx_v1900_extended"}) {
    const bool fm_is_1 = profile == "rx_v1900_extended";
    Receiver band;
    band.set_receiver_profile(profile);
    ready(band);
    band.feed(status(fm_is_1 ? "203501" : "203500"));
    assert(band.tuner_band_label_ == "FM");
    band.feed(status(fm_is_1 ? "303500" : "303501"));
    assert(band.tuner_band_label_ == "AM");
    for (const bool fm : {true, false}) {
      band.queue_.clear();
      band.command_tuner_band(fm ? "FM" : "AM");
      test_now += 200;
      band.process_command_queue_();
      assert(band.in_flight_ && band.in_flight_->command.payload == "07A16");  // Tuner input first.
      band.feed(status("002102"));
      assert(!band.in_flight_);
      test_now += 200;
      band.process_command_queue_();
      assert(band.in_flight_ && band.in_flight_->command.payload == (fm ? "07EBC" : "07EBD"));
      band.feed(status(fm == fm_is_1 ? "003500" : "003501"));  // Opposite band: not an acknowledgement.
      assert(band.in_flight_);
      band.feed(status(fm == fm_is_1 ? "003501" : "003500"));
      assert(!band.in_flight_ && band.timeouts_ == 0);
      assert(band.tuner_band_label_ == (fm ? "FM" : "AM"));
    }
  }
  // DOCK reports 0x12 in the main zone and 0x08 in zone 2; both selects list it, and the
  // remote-code commands captured from the remote and Receiver Manager select it.
  {
    Receiver dock;
    yamaha_serial::YamahaSelect main_input;
    yamaha_serial::YamahaSelect zone2_input;
    dock.set_input_source_select(&main_input);
    dock.set_zone2_input_source_select(&zone2_input);
    dock.set_receiver_profile("rx_v1900_extended");
    ready(dock);
    assert(main_input.has_option("DOCK") && zone2_input.has_option("DOCK"));
    assert(main_input.has_option("MD/CD-R") && !main_input.has_option("MD/TAPE"));
    assert(main_input.has_option("Multi CH") && !zone2_input.has_option("Multi CH"));
    dock.feed(status("402112"));
    assert(std::string(main_input.current_option().c_str()) == "DOCK");
    dock.feed(status("402408"));  // Captured from the front-panel input knob in zone 2.
    assert(dock.zone2_input_label_ == "DOCK");
    assert(std::string(zone2_input.current_option().c_str()) == "DOCK");
    dock.queue_.clear();
    dock.command_input("DOCK", false);
    test_now += 200;
    dock.process_command_queue_();
    assert(dock.in_flight_ && dock.in_flight_->command.payload == "0F7F014AB5");
    assert(dock.in_flight_->command.retries_left > 0);  // Direct select: retry-safe.
    dock.feed(status("002112"));
    assert(!dock.in_flight_ && dock.timeouts_ == 0);
    dock.command_input("DOCK", true);
    test_now += 200;
    dock.process_command_queue_();
    assert(dock.in_flight_ && dock.in_flight_->command.payload == "0F7F014BB4");
    dock.feed(status("002408"));
    assert(!dock.in_flight_ && dock.timeouts_ == 0);
  }
  // RX-V1900 System Memory: numbered 1-6, remote recall reports STX 0x30 (1-based).
  {
    Receiver memory;
    yamaha_serial::YamahaSelect scene;
    memory.set_scene_select(&scene);
    memory.set_receiver_profile("rx_v1900_extended");
    ready(memory);
    assert(scene.has_option("1") && scene.has_option("6") && !scene.has_option("A"));
    memory.feed(status("103002"));
    assert(memory.scene_label_ == "2" && std::string(scene.current_option().c_str()) == "2");
    memory.feed(status("103001"));
    assert(memory.scene_label_ == "1");
    memory.feed(status("103007"));  // Out of range: ignored.
    assert(memory.scene_label_ == "1");
    memory.queue_.clear();
    memory.command_scene("3");
    assert(memory.queue_.size() == 1 && memory.queue_.front().payload == "07E37");
    yamaha_serial::YamahaSelect late_scene;  // Attached after the profile loaded.
    memory.set_scene_select(&late_scene);
    assert(late_scene.has_option("1") && !late_scene.has_option("A"));
    // x800 keeps A-F; x600 ignores 0x30 (not part of its protocol generation).
    Receiver x800_memory;
    yamaha_serial::YamahaSelect x800_scene;
    x800_memory.set_scene_select(&x800_scene);
    x800_memory.set_receiver_profile("rx_vx800_extended");
    ready(x800_memory);
    assert(x800_scene.has_option("A") && x800_scene.has_option("F") && !x800_scene.has_option("1"));
    Receiver x600_memory;
    x600_memory.set_receiver_profile("rx_vx600_extended");
    ready(x600_memory);
    x600_memory.feed(status("103002"));
    assert(x600_memory.scene_label_ == "unknown");
  }
  // RX-V1900 2ch surround decoder: report 0x6E; direct commands only work in Surround Decode.
  {
    Receiver decoder;
    yamaha_serial::YamahaSelect select;
    decoder.set_surround_decoder_select(&select);
    decoder.set_receiver_profile("rx_v1900_extended");
    ready(decoder);
    const std::pair<const char *, const char *> reports[] = {
        {"00", "Pro Logic"},  {"01", "PLII Movie"},   {"02", "PLII Music"},
        {"03", "PLII Game"}, {"04", "Neo:6 Cinema"}, {"05", "Neo:6 Music"},
    };
    for (const auto &[code, label] : reports) {
      decoder.feed(status(std::string("106E") + code));
      assert(decoder.surround_decoder_label_ == label);
    }
    // Surround Decode is selected first, then the decoder.
    decoder.feed(status("202825"));  // Sci-Fi
    decoder.queue_.clear();
    decoder.command_surround_decoder("PLII Movie");
    assert(decoder.queue_.size() == 2 && decoder.queue_[0].payload == "07EFD" && decoder.queue_[1].payload == "07E67");
    test_now += 200;
    decoder.process_command_queue_();
    decoder.feed(status("00282C"));
    assert(!decoder.in_flight_);
    test_now += 200;
    decoder.process_command_queue_();
    assert(decoder.in_flight_ && decoder.in_flight_->command.retries_left == 0);
    decoder.feed(status("006E02"));  // Different decoder: not an acknowledgement.
    assert(decoder.in_flight_);
    decoder.feed(status("006E01"));
    assert(!decoder.in_flight_ && decoder.timeouts_ == 0 && decoder.surround_decoder_label_ == "PLII Movie");
    // Already in Surround Decode: it is still re-selected (keeps the decoder; checked on hardware).
    decoder.command_surround_decoder("Neo:6 Music");
    assert(decoder.queue_.size() == 2 && decoder.queue_[0].payload == "07EFD" && decoder.queue_[1].payload == "07E6A");
    decoder.queue_.clear();
    // A queued program change must not leave the decoder command in the wrong DSP mode
    // (maintainer reproduction): Surround Decode is restored before it.
    decoder.command_program("Sci-Fi");
    decoder.command_surround_decoder("PLII Movie");
    assert(decoder.queue_.size() == 3 && decoder.queue_[0].payload == "07EFA" &&
           decoder.queue_[1].payload == "07EFD" && decoder.queue_[2].payload == "07E67");
    decoder.queue_.clear();
    // Pro Logic has a direct command on the RX-V1900 (07EC9 -> 6E 00, checked on hardware).
    decoder.command_surround_decoder("Pro Logic");
    assert(decoder.queue_.size() == 2 && decoder.queue_[1].payload == "07EC9");
    test_now += 200;
    decoder.process_command_queue_();
    decoder.feed(status("00282C"));
    test_now += 200;
    decoder.process_command_queue_();
    decoder.feed(status("006E00"));
    assert(!decoder.in_flight_ && decoder.timeouts_ == 0 && decoder.surround_decoder_label_ == "Pro Logic");
    // The Auto/DTS/AAC input decoder mode is ignored by the RX-V1900.
    decoder.last_error_.clear();
    decoder.command_decoder_mode("DTS");
    assert(decoder.queue_.empty() && !decoder.last_error_.empty());
  }
  // A multichannel source ignores decoder commands even in Surround Decode. That expected
  // silence sets last_error but must not mark a connected receiver unavailable, and publishes
  // nothing (maintainer reproduction). Ordinary commands still detect a lost connection.
  {
    Receiver silent;
    silent.set_receiver_profile("rx_v1900_extended");
    ready(silent);
    silent.feed(status("00282C"));
    silent.feed(status("106E01"));
    silent.queue_.clear();
    silent.command_surround_decoder("Neo:6 Music");
    test_now += 200;
    silent.process_command_queue_();
    silent.feed(status("00282C"));
    test_now += 200;
    silent.process_command_queue_();
    assert(silent.in_flight_ && silent.in_flight_->command.payload == "07E6A");
    test_now += 1600;
    silent.process_command_queue_();
    assert(!silent.in_flight_ && silent.available_ && silent.receiver_reported_);
    assert(!silent.last_error_.empty() && silent.surround_decoder_label_ == "PLII Movie");
    silent.set_max_retries(0);
    silent.command_mute(true);
    test_now += 200;
    silent.process_command_queue_();
    test_now += 2000;
    silent.process_command_queue_();
    assert(!silent.available_);
  }
  // The decoder catalog is RX-V1900-specific: x700/x800 document a different table, so they
  // neither select from it nor decode 6E with it (maintainer reproduction); x600 has no 6E.
  for (const std::string profile : {"rx_vx600_extended", "rx_vx700_extended", "rx_vx800_extended"}) {
    Receiver other;
    other.set_receiver_profile(profile);
    ready(other);
    other.feed(status("006E01"));
    other.feed(status("006E09"));
    assert(other.surround_decoder_label_ == "unknown");
    other.command_surround_decoder("Pro Logic");
    assert(other.queue_.empty() && !other.last_error_.empty());
  }
  {
    Receiver x800_decoder;
    x800_decoder.set_receiver_profile("rx_vx800_extended");
    ready(x800_decoder);
    x800_decoder.command_decoder_mode("DTS");
    assert(x800_decoder.queue_.size() == 1 && x800_decoder.queue_[0].payload == "07EA8");
  }
  // Speaker A/B: the RX-V1900 has none and never answers 07EAB-07EAE, so the relays are
  // rejected before queueing (also after auto-detection); older profiles keep them.
  for (const std::string profile : {"rx_v1900", "rx_v1900_extended"}) {
    Receiver relay;
    relay.set_receiver_profile(profile);
    ready(relay);
    for (char which : {'A', 'B'}) {
      for (bool on : {true, false}) {
        relay.last_error_.clear();
        relay.command_speaker_relay(which, on);
        assert(relay.queue_.empty() && !relay.last_error_.empty());
      }
    }
  }
  {
    Receiver detected;
    ready(detected);
    detected.feed(full_config("R0240", "05"));
    assert(detected.active_profile_ == "rx_v1900_extended");
    detected.queue_.clear();
    detected.command_speaker_relay('A', true);
    assert(detected.queue_.empty() && !detected.last_error_.empty());
    Receiver x600_relay;
    x600_relay.set_receiver_profile("rx_vx600_extended");
    ready(x600_relay);
    x600_relay.command_speaker_relay('A', true);
    x600_relay.command_speaker_relay('B', false);
    assert(x600_relay.queue_.size() == 2 && x600_relay.queue_[0].payload == "07EAB" &&
           x600_relay.queue_[1].payload == "07EAE");
  }
  // Input selects only offer inputs with a command (maintainer request): x800 SAT is reported
  // but has no command, so it is not listed.
  {
    Receiver sat;
    yamaha_serial::YamahaSelect sat_input;
    sat.set_input_source_select(&sat_input);
    sat.set_receiver_profile("rx_vx800_extended");
    ready(sat);
    assert(!sat_input.has_option("SAT") && sat_input.has_option("DVD"));
  }
  // Sleep control: the RX-V1900 profile disables it (07EB3-07EB7 drive System Sleep, whose
  // Off is never reported); other profiles keep it.
  {
    Receiver sleep;
    sleep.set_receiver_profile("rx_v1900_extended");
    ready(sleep);
    sleep.command_sleep_timer("60 min");
    assert(sleep.queue_.empty() && !sleep.last_error_.empty());
    sleep.feed(status("002C02"));  // System Sleep reply.
    sleep.feed(status("10C500"));  // Remote Sleep key report.
    assert(sleep.sleep_timer_label_ == "unknown");
    Receiver x600_sleep;
    x600_sleep.set_receiver_profile("rx_vx600_extended");
    ready(x600_sleep);
    x600_sleep.feed(status("002C02"));
    assert(x600_sleep.sleep_timer_label_ == "60 min");
    x600_sleep.feed(status("10C504"));
    assert(x600_sleep.sleep_timer_label_ == "60 min");  // 0xC5 is not a sleep report there.
    x600_sleep.command_sleep_timer("Off");
    assert(x600_sleep.queue_.size() == 1 && x600_sleep.queue_[0].payload == "07EB3");
    test_now += 200;
    x600_sleep.process_command_queue_();
    x600_sleep.feed(status("002C02"));
    assert(x600_sleep.in_flight_);  // Off still needs 2C 04.
    x600_sleep.feed(status("002C04"));
    assert(!x600_sleep.in_flight_ && x600_sleep.sleep_timer_label_ == "Off");
  }
  // DC4 writes: the RX-V1900 acknowledges with the status alone, no value echo.
  {
    Receiver tone;
    tone.set_receiver_profile("rx_v1900_extended");
    ready(tone);
    tone.command_treble_percent(75.0f);
    test_now += 200;
    tone.process_command_queue_();
    assert(tone.in_flight_ && tone.in_flight_->command.payload == "2009033101112");
    tone.feed(extended("20040330"));  // Captured reply: 033, status OK.
    assert(!tone.in_flight_ && tone.timeouts_ == 0 && tone.parse_errors_ == 0);
    test_now += 200;
    tone.process_command_queue_();
    assert(tone.in_flight_ && tone.in_flight_->command.payload == "2006033001");  // Readback follows.
    tone.feed(extended("2009033001112"));
    assert(!tone.in_flight_ && tone.treble_percent_ > 74.0f && tone.treble_percent_ < 76.0f);
    // A bare acknowledgement does not satisfy a read, and a failing status is still final.
    tone.queue_simple_command_(FrameType::DC4, "2006033000", "poll_bass");
    test_now += 200;
    tone.process_command_queue_();
    tone.feed(extended("20040330"));
    assert(tone.in_flight_);
    // Echoing receivers keep matching on the echoed selector.
    Receiver echo;
    echo.set_receiver_profile("rx_vx600_extended");
    ready(echo);
    echo.command_bass_percent(50.0f);
    test_now += 200;
    echo.process_command_queue_();
    assert(echo.in_flight_ && echo.in_flight_->command.payload == "200903310010C");
    echo.feed(extended("200903300010C"));
    assert(!echo.in_flight_ && echo.parse_errors_ == 0);
  }
  // Falling back to the x600 profile must clear the two-nibble decode.
  v1900.feed(full_config("R9999", "51"));
  assert(v1900.active_profile_ == "rx_vx600_extended");
  assert(v1900.input_label_ == "DVD");
  Receiver v1900_hint;
  v1900_hint.set_model("RX-V1900");
  v1900_hint.setup();
  assert(v1900_hint.active_profile_ == "rx_v1900_extended");
  // RX-Vx600 keeps the single-nibble input decode; byte 19 is not part of the ID.
  Receiver x600;
  x600.set_receiver_profile("rx_vx600_extended");
  ready(x600);
  x600.feed(full_config("R0210", "51"));
  assert(x600.parse_errors_ == 0 && x600.active_profile_ == "rx_vx600_extended");
  assert(x600.input_label_ == "DVD");
  assert(x600.night_mode_label_ == "Off");             // RX-V1600 night-mode offset still decoded.
  assert(x600.extended_surround_label_ == "unknown");  // x700-layout fields are not.
  // Real RX-V1900 startup capture (203 bytes, valid checksum): Tuner, Straight Enhancer,
  // main + zone 2 on, zone 2 USB muted at -49 dB, EX/ES, sleep off, preset B3, FM, analog.
  {
    const char *capture =
        "12 52 30 32 34 30 4B 42 46 40 45 30 31 39 30 30 30 34 30 32 30 30 44 31 39 31 36 35 34 34 30 31 31 34 31 32 "
        "30 30 30 30 30 30 30 30 32 30 31 46 31 30 32 30 30 30 31 30 30 32 44 32 45 32 45 32 42 32 43 32 36 32 36 32 "
        "38 32 38 32 36 30 30 30 32 30 30 31 34 31 34 30 30 37 37 41 30 30 32 30 30 35 30 31 30 30 30 30 30 31 31 31 "
        "34 30 31 32 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 31 30 35 30 37 37 30 30 30 31 "
        "36 30 30 30 30 41 30 41 30 31 46 46 46 46 30 30 31 30 30 30 30 41 30 30 31 34 41 30 30 31 34 30 31 30 41 30 "
        "41 30 30 46 46 30 30 31 30 30 30 30 30 46 46 30 34 34 30 31 36 36 03";
    std::string frame;
    for (const char *p = capture; *p != '\0';) {
      frame.push_back(static_cast<char>(std::strtoul(p, const_cast<char **>(&p), 16)));
      while (*p == ' ') ++p;
    }
    assert(frame.size() == 203);
    Receiver startup;
    startup.set_receiver_profile("rx_v1900_extended");
    ready(startup);
    startup.feed(frame);
    assert(startup.parse_errors_ == 0 && startup.receiver_model_ == "RX-V1900");
    assert(startup.main_power_on_ && startup.zone2_power_on_);
    assert(startup.input_label_ == "Tuner" && startup.program_label_ == "Straight Enhancer");
    assert(!startup.mute_on_ && startup.main_volume_db_ == -27.0f);
    assert(startup.zone2_input_label_ == "USB" && startup.zone2_mute_on_ && startup.zone2_volume_db_ == -49.0f);
    assert(startup.extended_surround_label_ == "EX/ES");
    assert(startup.sleep_timer_label_ == "unknown");  // Sleep control is disabled on the RX-V1900.
    assert(startup.tuner_preset_page_label_ == "B" && startup.tuner_preset_label_ == "Preset 3");
    assert(startup.tuner_band_label_ == "FM");
    assert(startup.playback_format_ == "Analog" && startup.sampling_rate_hz_ == 0.0f);
    assert(startup.night_mode_label_ == "unknown");  // Not decoded from the x700 layout.
  }
  // Profiles without the extended DC2 layout ignore the input bytes entirely.
  Receiver x800;
  x800.set_receiver_profile("rx_vx800_extended");
  ready(x800);
  x800.feed(full_config("R0226", "05"));
  assert(x800.parse_errors_ == 0 && x800.input_label_ == "unknown");
  // The standard RX-V1900 profile is conservative: no DC2 offset decode.
  Receiver v1900_standard;
  v1900_standard.set_receiver_profile("rx_v1900");
  ready(v1900_standard);
  v1900_standard.feed(full_config("R0240", "0D"));
  assert(v1900_standard.parse_errors_ == 0 && v1900_standard.active_profile_ == "rx_v1900");
  assert(v1900_standard.input_label_ == "unknown");

  // RX-V1900 enhancer report codes (verified 0x44/0x45). 7ch Enhancer only accepts the
  // long remote-code form, captured from Receiver Manager.
  Receiver enhancer;
  enhancer.set_receiver_profile("rx_v1900_extended");
  ready(enhancer);
  enhancer.feed(status("402844"));
  assert(enhancer.program_label_ == "Straight Enhancer");
  enhancer.feed(status("402845"));
  assert(enhancer.program_label_ == "7ch Enhancer");
  enhancer.feed(status("202880"));
  assert(enhancer.program_label_ == "STRAIGHT");
  enhancer.queue_.clear();
  enhancer.command_program("7ch Enhancer");
  test_now += 200;
  enhancer.process_command_queue_();
  assert(enhancer.in_flight_ && enhancer.in_flight_->command.payload == "0F7E81146B");
  enhancer.feed(status("002845"));
  assert(!enhancer.in_flight_ && enhancer.timeouts_ == 0 && enhancer.program_label_ == "7ch Enhancer");
  Receiver x800_enhancer;
  x800_enhancer.set_receiver_profile("rx_vx800_extended");
  ready(x800_enhancer);
  x800_enhancer.feed(status("402840"));
  assert(x800_enhancer.program_label_ == "Straight Enhancer");  // x800 table unchanged.
  // RX-V1900 program reports from a front-panel cycle, in receiver order (plus STRAIGHT).
  const std::pair<const char *, const char *> v1900_programs[] = {
      {"25", "Sci-Fi"},           {"28", "Adventure"},        {"29", "Drama"},
      {"20", "Mono Movie"},       {"34", "2ch Stereo"},       {"17", "7ch Stereo"},
      {"44", "Straight Enhancer"}, {"45", "7ch Enhancer"},    {"2C", "Surround Decode"},
      {"00", "Munich"},           {"05", "Vienna"},           {"07", "Amsterdam"},
      {"09", "Freiburg"},         {"0B", "Chamber"},          {"0D", "Village Vanguard"},
      {"11", "Warehouse Loft"},   {"0F", "Cellar Club"},      {"10", "The Roxy Theatre"},
      {"0E", "The Bottom Line"},  {"21", "Sports"},           {"1E", "Action Game"},
      {"1F", "Roleplaying Game"}, {"18", "Music Video"},      {"1C", "Recital/Opera"},
      {"2D", "Standard"},         {"24", "Spectacle"},        {"80", "STRAIGHT"},
  };
  for (const auto &[code, label] : v1900_programs) {
    enhancer.feed(status(std::string("4028") + code));
    assert(enhancer.program_label_ == label);
  }
  assert(enhancer.programs_.size() == std::size(v1900_programs));
  // Programs the RX-V1900 does not have stay in the x800 table only.
  for (const char *absent : {"Disco", "Game", "THX Cinema", "THX Music", "THX Game"}) {
    auto has = [absent](const Receiver &r) {
      for (const auto &program : r.programs_)
        if (program.label == absent) return true;
      return false;
    };
    assert(!has(enhancer) && has(x800_enhancer));
  }

  // Playback format/sampling: x700+ use the full-byte x700 tables; x500/x600 keep the old ones.
  Receiver signal_x700;
  signal_x700.set_receiver_profile("rx_v1900_extended");
  ready(signal_x700);
  signal_x700.feed(status("001004"));
  signal_x700.feed(status("001109"));
  assert(signal_x700.playback_format_ == "Dolby Digital" && signal_x700.sampling_rate_hz_ == 48000.0f);
  signal_x700.feed(status("001002"));
  signal_x700.feed(status("001110"));
  assert(signal_x700.playback_format_ == "DSD" && signal_x700.sampling_rate_hz_ == 2822400.0f);
  signal_x700.feed(status("0010FE"));
  assert(signal_x700.playback_format_ == "Unknown");
  Receiver signal_x600;
  signal_x600.set_receiver_profile("rx_vx600_extended");
  ready(signal_x600);
  signal_x600.feed(status("001004"));
  signal_x600.feed(status("001109"));
  assert(signal_x600.playback_format_ == "Dolby Digital 2.0" && signal_x600.sampling_rate_hz_ == 176400.0f);

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
