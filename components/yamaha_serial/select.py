import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select

from . import (
    CONF_YAMAHA_SERIAL_ID,
    DEFAULT_INPUTS,
    YAMAHA_SERIAL_COMPONENT_SCHEMA,
    yamaha_serial_ns,
)

DEPENDENCIES = ["yamaha_serial"]

CONF_INPUT_SOURCE = "input_source"
CONF_ZONE2_INPUT_SOURCE = "zone2_input_source"
CONF_PROGRAM = "program"
CONF_SCENE = "scene"
CONF_AUDIO_SELECT = "audio_select"
CONF_NIGHT_MODE = "night_mode"
CONF_TUNER_PRESET = "tuner_preset"
CONF_TUNER_PRESET_PAGE = "tuner_preset_page"
CONF_TUNER_BAND = "tuner_band"
CONF_SLEEP_TIMER = "sleep_timer"
CONF_DECODER_MODE = "decoder_mode"
CONF_EXTENDED_SURROUND = "extended_surround"
CONF_SPEAKER_B_ASSIGNMENT = "speaker_b_assignment"
CONF_ZONE2_AMP = "zone2_amp"

YamahaInputSelect = yamaha_serial_ns.class_("YamahaInputSelect", select.Select)
YamahaZone2InputSelect = yamaha_serial_ns.class_(
    "YamahaZone2InputSelect", select.Select
)
YamahaProgramSelect = yamaha_serial_ns.class_("YamahaProgramSelect", select.Select)
YamahaSceneSelect = yamaha_serial_ns.class_("YamahaSceneSelect", select.Select)
YamahaAudioSelectSelect = yamaha_serial_ns.class_(
    "YamahaAudioSelectSelect", select.Select
)
YamahaNightModeSelect = yamaha_serial_ns.class_(
    "YamahaNightModeSelect", select.Select
)
YamahaTunerPresetSelect = yamaha_serial_ns.class_(
    "YamahaTunerPresetSelect", select.Select
)
YamahaTunerPresetPageSelect = yamaha_serial_ns.class_(
    "YamahaTunerPresetPageSelect", select.Select
)
YamahaTunerBandSelect = yamaha_serial_ns.class_("YamahaTunerBandSelect", select.Select)
YamahaSleepTimerSelect = yamaha_serial_ns.class_(
    "YamahaSleepTimerSelect", select.Select
)
YamahaDecoderModeSelect = yamaha_serial_ns.class_(
    "YamahaDecoderModeSelect", select.Select
)
YamahaExtendedSurroundSelect = yamaha_serial_ns.class_(
    "YamahaExtendedSurroundSelect", select.Select
)
YamahaSpeakerBAssignmentSelect = yamaha_serial_ns.class_(
    "YamahaSpeakerBAssignmentSelect", select.Select
)
YamahaZone2AmpSelect = yamaha_serial_ns.class_("YamahaZone2AmpSelect", select.Select)

PROGRAM_OPTIONS = [
    "STRAIGHT",
    "Munich",
    "Hall B",
    "Hall C",
    "Hall D",
    "Vienna",
    "Amsterdam",
    "Freiburg",
    "Chamber",
    "Village Vanguard",
    "Warehouse Loft",
    "Cellar Club",
    "Bottom Line",
    "Roxy Theater",
    "The Bottom Line",
    "The Roxy Theatre",
    "Disco",
    "Game",
    "7ch Stereo",
    "2ch Stereo",
    "Sports",
    "Action Game",
    "Roleplaying Game",
    "Music Video",
    "Recital/Opera",
    "Standard",
    "Spectacle",
    "Sci-Fi",
    "Adventure",
    "Drama",
    "Mono Movie",
    "Surround Decode",
    "THX Cinema",
    "THX Music",
    "THX Game",
    "ProLogic",
    "Pro Logic",
    "Music Pop",
    "General",
    "Enhancer 2ch Low",
    "Enhancer 2ch High",
    "Enhancer 7ch Low",
    "Enhancer 7ch High",
]

SCENE_OPTIONS = ["A", "B", "C", "D", "E", "F"]
INPUT_OPTIONS = list(DEFAULT_INPUTS.values())
AUDIO_SELECT_OPTIONS = ["Auto", "Coax/Opt", "Analog", "HDMI"]
NIGHT_MODE_OPTIONS = [
    "Off",
    "Cinema Low",
    "Cinema Mid",
    "Cinema High",
    "Music Low",
    "Music Mid",
    "Music High",
]
TUNER_PRESET_OPTIONS = [
    "Preset 1",
    "Preset 2",
    "Preset 3",
    "Preset 4",
    "Preset 5",
    "Preset 6",
    "Preset 7",
    "Preset 8",
]
TUNER_PRESET_PAGE_OPTIONS = ["A", "B", "C", "D", "E"]
TUNER_BAND_OPTIONS = ["FM", "AM"]
SLEEP_TIMER_OPTIONS = ["Off", "30 min", "60 min", "90 min", "120 min"]
DECODER_MODE_OPTIONS = ["Auto", "DTS", "AAC"]
EXTENDED_SURROUND_OPTIONS = [
    "Off",
    "EX/ES",
    "Auto",
    "EX",
    "PLIIx Movie",
    "PLIIx Music",
]
SPEAKER_B_ASSIGNMENT_OPTIONS = ["Main", "Zone B"]
ZONE2_AMP_OPTIONS = ["External", "Internal"]

CONFIG_SCHEMA = YAMAHA_SERIAL_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(CONF_INPUT_SOURCE): select.select_schema(
            YamahaInputSelect, icon="mdi:audio-input-stereo-minijack"
        ),
        cv.Optional(CONF_ZONE2_INPUT_SOURCE): select.select_schema(
            YamahaZone2InputSelect, icon="mdi:audio-input-stereo-minijack"
        ),
        cv.Optional(CONF_PROGRAM): select.select_schema(
            YamahaProgramSelect, icon="mdi:music-circle-outline"
        ),
        cv.Optional(CONF_SCENE): select.select_schema(
            YamahaSceneSelect, icon="mdi:palette-outline"
        ),
        cv.Optional(CONF_AUDIO_SELECT): select.select_schema(
            YamahaAudioSelectSelect, icon="mdi:audio-input-stereo-minijack"
        ),
        cv.Optional(CONF_NIGHT_MODE): select.select_schema(
            YamahaNightModeSelect, icon="mdi:weather-night"
        ),
        cv.Optional(CONF_TUNER_PRESET): select.select_schema(
            YamahaTunerPresetSelect, icon="mdi:radio"
        ),
        cv.Optional(CONF_TUNER_PRESET_PAGE): select.select_schema(
            YamahaTunerPresetPageSelect, icon="mdi:radio"
        ),
        cv.Optional(CONF_TUNER_BAND): select.select_schema(
            YamahaTunerBandSelect, icon="mdi:radio-tower"
        ),
        cv.Optional(CONF_SLEEP_TIMER): select.select_schema(
            YamahaSleepTimerSelect, icon="mdi:timer-outline"
        ),
        cv.Optional(CONF_DECODER_MODE): select.select_schema(
            YamahaDecoderModeSelect, icon="mdi:surround-sound"
        ),
        cv.Optional(CONF_EXTENDED_SURROUND): select.select_schema(
            YamahaExtendedSurroundSelect, icon="mdi:surround-sound-7-1"
        ),
        cv.Optional(CONF_SPEAKER_B_ASSIGNMENT): select.select_schema(
            YamahaSpeakerBAssignmentSelect, icon="mdi:speaker-multiple"
        ),
        cv.Optional(CONF_ZONE2_AMP): select.select_schema(
            YamahaZone2AmpSelect, icon="mdi:amplifier"
        ),
    }
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_YAMAHA_SERIAL_ID])

    if conf := config.get(CONF_INPUT_SOURCE):
        var = await select.new_select(conf, options=INPUT_OPTIONS)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_input_source_select(var))

    if conf := config.get(CONF_ZONE2_INPUT_SOURCE):
        var = await select.new_select(conf, options=INPUT_OPTIONS)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_zone2_input_source_select(var))

    if conf := config.get(CONF_PROGRAM):
        var = await select.new_select(conf, options=PROGRAM_OPTIONS)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_program_select(var))

    if conf := config.get(CONF_SCENE):
        var = await select.new_select(conf, options=SCENE_OPTIONS)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_scene_select(var))

    if conf := config.get(CONF_AUDIO_SELECT):
        var = await select.new_select(conf, options=AUDIO_SELECT_OPTIONS)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_audio_select_select(var))

    if conf := config.get(CONF_NIGHT_MODE):
        var = await select.new_select(conf, options=NIGHT_MODE_OPTIONS)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_night_mode_select(var))

    if conf := config.get(CONF_TUNER_PRESET):
        var = await select.new_select(conf, options=TUNER_PRESET_OPTIONS)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_tuner_preset_select(var))

    if conf := config.get(CONF_TUNER_PRESET_PAGE):
        var = await select.new_select(conf, options=TUNER_PRESET_PAGE_OPTIONS)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_tuner_preset_page_select(var))

    if conf := config.get(CONF_TUNER_BAND):
        var = await select.new_select(conf, options=TUNER_BAND_OPTIONS)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_tuner_band_select(var))

    if conf := config.get(CONF_SLEEP_TIMER):
        var = await select.new_select(conf, options=SLEEP_TIMER_OPTIONS)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_sleep_timer_select(var))

    if conf := config.get(CONF_DECODER_MODE):
        var = await select.new_select(conf, options=DECODER_MODE_OPTIONS)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_decoder_mode_select(var))

    if conf := config.get(CONF_EXTENDED_SURROUND):
        var = await select.new_select(conf, options=EXTENDED_SURROUND_OPTIONS)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_extended_surround_select(var))

    if conf := config.get(CONF_SPEAKER_B_ASSIGNMENT):
        var = await select.new_select(conf, options=SPEAKER_B_ASSIGNMENT_OPTIONS)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_speaker_b_assignment_select(var))

    if conf := config.get(CONF_ZONE2_AMP):
        var = await select.new_select(conf, options=ZONE2_AMP_OPTIONS)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_zone2_amp_select(var))
