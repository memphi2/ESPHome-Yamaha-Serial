import json
from pathlib import Path

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

YamahaSelect = yamaha_serial_ns.class_("YamahaSelect", select.Select)
SelectFunction = yamaha_serial_ns.enum("SelectFunction", is_class=True)

SELECT_CATALOG = json.loads(Path(__file__).with_name("select_catalog.json").read_text())["selects"]

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
AUDIO_SELECT_OPTIONS = SELECT_CATALOG["audio_select"]["options"]
NIGHT_MODE_OPTIONS = SELECT_CATALOG["night_mode"]["options"]
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
TUNER_PRESET_PAGE_OPTIONS = SELECT_CATALOG["tuner_preset_page"]["options"]
TUNER_BAND_OPTIONS = SELECT_CATALOG["tuner_band"]["options"]
SLEEP_TIMER_OPTIONS = SELECT_CATALOG["sleep_timer"]["options"]
DECODER_MODE_OPTIONS = SELECT_CATALOG["decoder_mode"]["options"]
EXTENDED_SURROUND_OPTIONS = SELECT_CATALOG["extended_surround"]["options"]
SPEAKER_B_ASSIGNMENT_OPTIONS = SELECT_CATALOG["speaker_b_assignment"]["options"]
ZONE2_AMP_OPTIONS = SELECT_CATALOG["zone2_amp"]["options"]

# YAML key -> (component setter, receiver function, option list, icon). The
# input and program setters replace the option list at runtime from the
# configured labels and the detected receiver profile; the rest keep the list
# below.
SELECT_TYPES = {
    "input_source": (
        "set_input_source_select",
        SelectFunction.INPUT_SOURCE,
        INPUT_OPTIONS,
        "mdi:audio-input-stereo-minijack",
    ),
    "zone2_input_source": (
        "set_zone2_input_source_select",
        SelectFunction.ZONE2_INPUT_SOURCE,
        INPUT_OPTIONS,
        "mdi:audio-input-stereo-minijack",
    ),
    "program": (
        "set_program_select",
        SelectFunction.PROGRAM,
        PROGRAM_OPTIONS,
        "mdi:music-circle-outline",
    ),
    "scene": (
        "set_scene_select",
        SelectFunction.SCENE,
        SCENE_OPTIONS,
        "mdi:palette-outline",
    ),
    "audio_select": (
        "set_audio_select_select",
        SelectFunction.AUDIO_SELECT,
        AUDIO_SELECT_OPTIONS,
        "mdi:audio-input-stereo-minijack",
    ),
    "night_mode": (
        "set_night_mode_select",
        SelectFunction.NIGHT_MODE,
        NIGHT_MODE_OPTIONS,
        "mdi:weather-night",
    ),
    "tuner_preset": (
        "set_tuner_preset_select",
        SelectFunction.TUNER_PRESET,
        TUNER_PRESET_OPTIONS,
        "mdi:radio",
    ),
    "tuner_preset_page": (
        "set_tuner_preset_page_select",
        SelectFunction.TUNER_PRESET_PAGE,
        TUNER_PRESET_PAGE_OPTIONS,
        "mdi:radio",
    ),
    "tuner_band": (
        "set_tuner_band_select",
        SelectFunction.TUNER_BAND,
        TUNER_BAND_OPTIONS,
        "mdi:radio-tower",
    ),
    "sleep_timer": (
        "set_sleep_timer_select",
        SelectFunction.SLEEP_TIMER,
        SLEEP_TIMER_OPTIONS,
        "mdi:timer-outline",
    ),
    "decoder_mode": (
        "set_decoder_mode_select",
        SelectFunction.DECODER_MODE,
        DECODER_MODE_OPTIONS,
        "mdi:surround-sound",
    ),
    "extended_surround": (
        "set_extended_surround_select",
        SelectFunction.EXTENDED_SURROUND,
        EXTENDED_SURROUND_OPTIONS,
        "mdi:surround-sound-7-1",
    ),
    "speaker_b_assignment": (
        "set_speaker_b_assignment_select",
        SelectFunction.SPEAKER_B_ASSIGNMENT,
        SPEAKER_B_ASSIGNMENT_OPTIONS,
        "mdi:speaker-multiple",
    ),
    "zone2_amp": (
        "set_zone2_amp_select",
        SelectFunction.ZONE2_AMP,
        ZONE2_AMP_OPTIONS,
        "mdi:amplifier",
    ),
}

CONFIG_SCHEMA = YAMAHA_SERIAL_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(key): select.select_schema(YamahaSelect, icon=icon)
        for key, (_, _, _, icon) in SELECT_TYPES.items()
    }
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_YAMAHA_SERIAL_ID])

    for key, (setter, function, options, _) in SELECT_TYPES.items():
        if conf := config.get(key):
            var = await select.new_select(conf, options=options)
            cg.add(var.set_parent(paren))
            cg.add(var.set_function(function))
            cg.add(getattr(paren, setter)(var))
