from __future__ import annotations

import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]
CPP = ROOT / "components/yamaha_serial/yamaha_serial.cpp"
PROTOCOL_DOC = ROOT / "docs/protocol.md"
PROFILES_DOC = ROOT / "docs/profiles.md"


def test_core_reference_commands_present_in_component() -> None:
    text = CPP.read_text(encoding="utf-8")
    required = [
        "07E7E",  # power on
        "07E7F",  # power off
        "07EA2",  # mute on
        "07EA3",  # mute off
        "07A1A",  # volume up
        "07A1B",  # volume down
        "07EBA",  # zone2 power on
        "07EBB",  # zone2 power off
        "07ADA",  # zone2 volume up
        "07ADB",  # zone2 volume down
        "22001",  # volume text poll
        "22002",  # zone2 volume text poll
        "22003",  # input text poll
        "22004",  # zone2 input text poll
        "20000",  # report enable
        "20100",  # report realtime delay
        "07EA6",  # audio select auto
        "07EA9",  # audio select coax/opt
        "07EAA",  # audio select analog
        "07EDA",  # audio select hdmi
        "28B00",  # night mode off
        "2B201",  # fan mode on
        "07E80",  # pure direct on
        "07E82",  # pure direct off
        "07EA0",  # zone2 mute on
        "07EA1",  # zone2 mute off
        "07EAB",  # speaker relay A on
        "07EAC",  # speaker relay A off
        "07EAD",  # speaker relay B on
        "07EAE",  # speaker relay B off
        "07AE5",  # tuner preset 1
        "07AEC",  # tuner preset 8
        "07AE0",  # tuner preset page A
        "07AE4",  # tuner preset page E
        "07EBC",  # tuner band FM
        "07EBD",  # tuner band AM
        "07EBE",  # tuner auto seek up
        "07EBF",  # tuner auto seek down
        "07EB3",  # sleep timer off
        "07EB7",  # sleep timer 30
        "07EDB",  # decoder mode auto
        "07EA8",  # decoder mode DTS
        "07E3B",  # decoder mode AAC
        "07EB8",  # extended surround EX/ES
        "07E7C",  # extended surround auto
        "07E28",  # speaker B assignment main
        "07E29",  # speaker B assignment zone B
        "07E99",  # zone2 amp internal
        "07E9A",  # zone2 amp external
        "2006050000",  # tuner station read
        "200D050100",  # tuner station write prefix
        "2006033000",  # bass read
        "2006033001",  # treble read
        "2006041000",  # center distance read
        "20060410A0",  # subwoofer distance read
    ]
    missing = [cmd for cmd in required if cmd not in text]
    assert not missing, f"Missing required command constants: {missing}"


def test_rx_v1600_reference_commands_present_in_component() -> None:
    text = CPP.read_text(encoding="utf-8")
    rx_v1600_commands = [
        "07A14",  # PHONO input
        "07A15",  # CD input
        "07A16",  # Tuner input
        "07A18",  # MD/TAPE input used by RX-V1600
        "07A19",  # CD-R input
        "07A54",  # DTV input
        "07A55",  # V-AUX input
        "07AC0",  # CBL/SAT input
        "07AC1",  # DVD input
        "07E80",  # Pure Direct ON on RX-V1600
        "07E82",  # Pure Direct OFF on RX-V1600
        "20090331001",  # Bass write prefix
        "20090331011",  # Treble write prefix
        "2009041100",  # Center distance write prefix
        "2009041120",  # Front left distance write prefix
        "2009041130",  # Front right distance write prefix
        "2009041140",  # Surround left distance write prefix
        "2009041150",  # Surround right distance write prefix
        "20090411A0",  # Subwoofer distance write prefix
    ]
    missing = [cmd for cmd in rx_v1600_commands if cmd not in text]
    assert not missing, f"Missing RX-V1600 reference commands: {missing}"


def test_x500_and_x600_profiles_keep_their_standard_input_differences() -> None:
    text = CPP.read_text(encoding="utf-8")
    assert '{"md_tape", "MD/TAPE", 0x4, 0x4, "07AC9", "07ACF"}' in text
    assert 'if (input.key == "md_tape") {' in text
    assert 'input.main_command = "07A18";' in text


def test_protocol_doc_contains_command_coverage_table() -> None:
    doc = PROTOCOL_DOC.read_text(encoding="utf-8")
    assert "Command coverage" in doc
    assert "| `07E7E` | Power on |" in doc
    assert "| `230xx` | Absolute main volume |" in doc
    assert "| `07AE5`..`07AEC` | Tuner preset 1..8 | `select.tuner_preset` |" in doc
    assert "| `07AE0`..`07AE4` | Tuner preset page A-E | `select.tuner_preset_page` |" in doc
    assert "| `07EBC`/`07EBD` | Tuner band FM/AM | `select.tuner_band` |" in doc
    assert "| `07EA0`/`07EA1` | Zone2 mute on/off | `switch.zone2_mute` |" in doc
    assert "| `2006050000` | Current tuner station read |" in doc
    assert "| `2006033000`/`2006033001` | Bass/treble read |" in doc


def test_cpp_contains_profile_layout_hooks() -> None:
    text = CPP.read_text(encoding="utf-8")
    profiles_doc = PROFILES_DOC.read_text(encoding="utf-8")
    assert 'return "rx_vx500";' in text
    assert 'return "rx_vx500_extended";' in text
    assert 'return "rx_vx600";' in text
    assert 'return "rx_vx600_extended";' in text
    assert 'return "rx_vx700";' in text
    assert 'return "rx_vx700_extended";' in text
    assert 'return "rx_vx800";' in text
    assert 'return "rx_vx800_extended";' in text
    assert "apply_profile_rx_vx500_" in text
    assert "apply_profile_rx_vx500_extended_" in text
    assert "apply_profile_rx_vx600_" in text
    assert "apply_profile_rx_vx600_extended_" in text
    assert "apply_profile_rx_vx700_" in text
    assert "apply_profile_rx_vx700_extended_" in text
    assert "apply_profile_rx_vx800_" in text
    assert "apply_profile_rx_vx800_extended_" in text
    assert "Standard" in profiles_doc
    assert "Extended" in profiles_doc
    assert "handle_detected_model_()" in profiles_doc


def test_auto_detect_chooses_extended_profiles() -> None:
    text = CPP.read_text(encoding="utf-8")
    assert "receiver_model_" in text
    for profile in (
        "rx_vx500_extended",
        "rx_vx600_extended",
        "rx_vx700_extended",
        "rx_vx800_extended",
    ):
        assert f'return "{profile}";' in text


def test_detected_model_reapplies_auto_profile_and_refreshes_options() -> None:
    text = CPP.read_text(encoding="utf-8")
    start = text.index("void YamahaSerialComponent::handle_detected_model_")
    end = text.index("void YamahaSerialComponent::configure_input_select_options_", start)
    body = text[start:end]

    assert "this->is_auto_profile_()" in body
    assert "this->resolve_profile_()" in body
    assert "this->apply_profile_(detected_profile);" in body
    assert "this->apply_input_overrides_();" in body
    assert "this->configure_input_select_options_();" in body
    assert "this->configure_program_select_options_();" in body
    assert "this->handle_detected_model_(model);" in text


def test_input_label_overrides_survive_profile_reapply() -> None:
    header = (ROOT / "components/yamaha_serial/yamaha_serial.h").read_text(
        encoding="utf-8"
    )
    text = CPP.read_text(encoding="utf-8")

    assert "struct InputOverride" in header
    assert "std::vector<InputOverride> input_overrides_" in header
    assert "void YamahaSerialComponent::apply_input_overrides_()" in text
    assert "this->input_overrides_.push_back({key_norm, label});" in text
    assert "this->apply_profile_(profile_id);" in text
    assert "this->apply_profile_(detected_profile);" in text


def test_standard_and_extended_profiles_have_distinct_capabilities() -> None:
    text = CPP.read_text(encoding="utf-8")

    def body(name: str) -> str:
        match = re.search(
            rf"void YamahaSerialComponent::{name}\(\) \{{(.*?)\n\}}",
            text,
            flags=re.DOTALL,
        )
        assert match is not None
        return match.group(1)

    for name in ("apply_profile_rx_vx600_", "apply_profile_rx_vx700_", "apply_profile_rx_vx800_"):
        assert "supports_extended = false" in body(name)

    for name in (
        "apply_profile_rx_vx600_extended_",
        "apply_profile_rx_vx700_extended_",
        "apply_profile_rx_vx800_extended_",
    ):
        assert "supports_extended = true" in body(name)

    assert "void YamahaSerialComponent::apply_profile_rx_vx500_extended_()" in text


def test_cpp_contains_x500_model_detection_and_profile_content() -> None:
    text = CPP.read_text(encoding="utf-8")
    for token in (
        "vx500",
        "v1500",
        "v2500",
        "D-TV/LD",
        "VCR3",
        "Hall A",
        "2ch Direct Stereo",
        "THX Cinema",
        "07E35",
        "07E3A",
    ):
        assert token in text


def test_cpp_contains_x700_x800_variant_commands() -> None:
    text = CPP.read_text(encoding="utf-8")
    for token in (
        "v1700",
        "v2700",
        "v1800",
        "v3800",
        "0F7F013FC0",
        "0F7F0140BF",
        "BD/HD DVD",
        "Straight Enhancer",
        "supports_night_mode = false",
    ):
        assert token in text
    assert "publish_input_by_report_(data, false)" in text
    assert "case 0x07:" in text


def test_cpp_waits_for_receiver_before_regular_polling() -> None:
    text = CPP.read_text(encoding="utf-8")
    assert 'publish_connection_state_("waiting_for_receiver")' in text
    assert "queue_probe_ready_();" in text
    assert "if (!this->receiver_reported_) {" in text
    assert 'queue_simple_command_(FrameType::DC1, "000", "probe_ready", false);' in text


def test_dc4_parser_ignores_non_extended_payloads_before_minimum_length_check() -> None:
    text = CPP.read_text(encoding="utf-8")
    ignore_idx = text.index('ESP_LOGD(TAG, "Ignoring unsupported DC4 payload: %s"')
    minimum_idx = text.index('record_parse_error_("Extended payload shorter than minimum length", frame)')
    assert ignore_idx < minimum_idx
    assert "parse_dc4_extended_payload_(body, frame);" in text


def test_status_frame_parser_does_not_queue_extended_poll_burst() -> None:
    text = CPP.read_text(encoding="utf-8")
    start = text.index("void YamahaSerialComponent::parse_stx_frame_")
    end = text.index("void YamahaSerialComponent::parse_dc4_frame_")
    assert "queue_extended_poll_commands_" not in text[start:end]


def test_media_player_routes_receiver_shortcuts_through_existing_mappings() -> None:
    text = CPP.read_text(encoding="utf-8")
    assert "MediaPlayerEntityFeature::PLAY" in text
    assert "MediaPlayerEntityFeature::NEXT_TRACK" in text
    assert "MediaPlayerEntityFeature::SELECT_SOURCE" not in text
    assert "MediaPlayerEntityFeature::SELECT_SOUND_MODE" not in text
    assert "call.get_media_url().has_value()" in text
    assert "command_media_player_shortcut" in text
    assert "find_input_index_(shortcut)" in text
    assert "find_program_index_(shortcut)" in text
    assert 'strip_prefix("yamaha://input")' in text
    assert 'strip_prefix("media_player.select_source")' in text
    assert 'strip_prefix("media-source://yamaha/source")' in text
    assert 'strip_prefix("yamaha://program")' in text
    assert 'strip_prefix("yamaha://sound_mode")' in text
    assert 'strip_prefix("media_player.select_sound_mode")' in text
    assert 'strip_prefix("media-source://yamaha/sound_mode")' in text
    assert 'strip_prefix("yamaha://raw")' in text
    assert "MEDIA_PLAYER_STATE_IDLE" in text


def test_program_select_options_are_profile_driven() -> None:
    header = (ROOT / "components/yamaha_serial/yamaha_serial.h").read_text(
        encoding="utf-8"
    )
    cpp = CPP.read_text(encoding="utf-8")

    assert "void set_options_from_labels" in header
    assert "void YamahaSerialComponent::configure_program_select_options_" in cpp
    assert "this->program_select_->set_options_from_labels(labels);" in cpp
    assert "this->configure_program_select_options_();" in cpp


def test_raw_command_action_covers_raw_command_matrix() -> None:
    cpp = CPP.read_text(encoding="utf-8")
    init = (ROOT / "components/yamaha_serial/__init__.py").read_text(encoding="utf-8")

    header = (ROOT / "components/yamaha_serial/yamaha_serial.h").read_text(
        encoding="utf-8"
    )
    assert "class YamahaRawCommandAction" in header
    assert "command_raw_text" in cpp
    for prefix in ("stx", "dc4", "dc1", "dc3"):
        assert prefix in cpp
    assert "Extended raw Yamaha command must start with 20" in cpp
    assert '"yamaha_serial.raw_command"' in init
    assert "YamahaRawCommandAction" in init
    assert '"text"' not in init


def test_symbolic_raw_command_alias_catalog_covers_receiver_tables() -> None:
    cpp = CPP.read_text(encoding="utf-8")
    aliases = re.findall(
        r'\{"([^"]+)",\s*"([^"]+)",\s*static_cast<uint8_t>\(FrameType::(STX|DC4)\),\s*(true|false)\}',
        cpp,
    )

    assert "queue_named_command_" in cpp
    assert len(aliases) >= 640
    assert "Raw Yamaha command requires a known alias or prefix" in cpp
    assert 'prefix == "alias" || prefix == "command" || prefix == "cmd"' in cpp

    expected_aliases = {
        "power_on": "07E7E",
        "all_zone_power_on": "07A1D",
        "zone3_power_on": "07AED",
        "zone3_input_net_usb": "0F7F0141BE",
        "input_md_tape_rx_v1600": "07A18",
        "input_md_tape_x500": "07AC9",
        "zone2_input_md_tape_x500": "07ACF",
        "pure_direct_off_rx_v2500": "07E83",
        "system_memory_save_6": "07E30",
        "home_preset_memory_f": "07E20",
        "home_volume_memory_save_f": "07E60",
        "main_volume_memory_load_6": "07E7A",
        "trigger2_control_zone3": "07E9F",
        "xm_search_category": "07AB6",
        "program_hall_a": "07EE1",
        "program_munich": "07EE1",
        "program_surround_decode": "07EFD",
        "report_delay_400ms": "20108",
        "firmware_version_request": "22F00",
        "main_level_normal": "23300",
        "zone3_amplifier_both": "23F03",
        "wallpaper_none": "2580F",
        "dsp_3d_on": "25701",
        "hdmi_auto_lipsync_auto": "25900",
        "dynamic_range_speaker_min": "26402",
        "subwoofer_config_none_x500": "27602",
        "multi_ch_select_8ch_cd_x500": "27B01",
        "speaker_surround_back_small_x1": "27303",
        "subwoofer_crossover_200hz": "27E08",
        "test_tone_dsp_x500": "28002",
        "night_mode_music_high": "28B22",
        "net_usb_shuffle_on": "29A01",
        "advanced_setup_on_x500": "2B001",
        "speaker_impedance_6ohm_x500": "2B301",
        "ext_read_model_name": "20050000F",
        "ext_read_input_audio_optical": "200500102",
        "ext_read_dsp_user_parameters": "200500201",
        "ext_read_xm_current_song_name": "2006003400",
        "gui_cursor_up": "07A9D",
        "ipod_play": "0F7F011EE1",
        "net_usb_play": "0F7F013EC1",
        "enhancer_7ch": "0F7E81146B",
    }
    alias_payloads = {alias: (frame_type, payload) for alias, payload, frame_type, _ in aliases}
    for alias, payload in expected_aliases.items():
        assert alias_payloads[alias][1] == payload
    assert alias_payloads["ext_read_model_name"][0] == "DC4"
