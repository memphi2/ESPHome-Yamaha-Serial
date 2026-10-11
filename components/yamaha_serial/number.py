import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    CONF_MAX_VALUE,
    CONF_MIN_VALUE,
    CONF_STEP,
    ENTITY_CATEGORY_CONFIG,
    UNIT_DECIBEL,
)

from . import CONF_YAMAHA_SERIAL_ID, YAMAHA_SERIAL_COMPONENT_SCHEMA, yamaha_serial_ns

DEPENDENCIES = ["yamaha_serial"]

YamahaNumber = yamaha_serial_ns.class_("YamahaNumber", number.Number)
NumberFunction = yamaha_serial_ns.enum("NumberFunction", is_class=True)

NUMBER_EXT_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_MIN_VALUE, default=-80.0): cv.float_range(min=-99.5, max=16.5),
        cv.Optional(CONF_MAX_VALUE, default=16.5): cv.float_range(min=-99.5, max=16.5),
        cv.Optional(CONF_STEP, default=0.5): cv.float_range(min=0.5, max=10.0),
    }
)

BASS_TREBLE_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_MIN_VALUE, default=0.0): cv.float_range(min=0.0, max=100.0),
        cv.Optional(CONF_MAX_VALUE, default=100.0): cv.float_range(min=0.0, max=100.0),
        cv.Optional(CONF_STEP, default=1.0): cv.float_range(min=1.0, max=10.0),
    }
)

DISTANCE_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_MIN_VALUE, default=30.0): cv.float_range(min=30.0, max=2400.0),
        cv.Optional(CONF_MAX_VALUE, default=2400.0): cv.float_range(
            min=30.0, max=2400.0
        ),
        cv.Optional(CONF_STEP, default=1.0): cv.float_range(min=1.0, max=100.0),
    }
)

DIMMER_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_MIN_VALUE, default=0.0): cv.float_range(min=0.0, max=100.0),
        cv.Optional(CONF_MAX_VALUE, default=100.0): cv.float_range(min=0.0, max=100.0),
        cv.Optional(CONF_STEP, default=25.0): cv.float_range(min=1.0, max=25.0),
    }
)

FM_FREQUENCY_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_MIN_VALUE, default=87.5): cv.float_range(min=76.0, max=108.0),
        cv.Optional(CONF_MAX_VALUE, default=108.0): cv.float_range(min=76.0, max=108.0),
        cv.Optional(CONF_STEP, default=0.05): cv.float_range(min=0.01, max=1.0),
    }
)

AM_FREQUENCY_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_MIN_VALUE, default=530.0): cv.float_range(
            min=500.0, max=1800.0
        ),
        cv.Optional(CONF_MAX_VALUE, default=1710.0): cv.float_range(
            min=500.0, max=1800.0
        ),
        cv.Optional(CONF_STEP, default=1.0): cv.float_range(min=1.0, max=10.0),
    }
)

_DISTANCE = {"unit_of_measurement": "cm", "entity_category": ENTITY_CATEGORY_CONFIG}

# YAML key -> (component setter, receiver function, number_schema keyword
# arguments, range schema). Entries without an entity_category are daily
# controls; the config entities are marked explicitly.
NUMBER_TYPES = {
    "volume_db": (
        "set_volume_number",
        NumberFunction.VOLUME,
        {
            "unit_of_measurement": UNIT_DECIBEL,
            "icon": "mdi:volume-high",
            "entity_category": ENTITY_CATEGORY_CONFIG,
        },
        NUMBER_EXT_SCHEMA,
    ),
    "zone2_volume_db": (
        "set_zone2_volume_number",
        NumberFunction.ZONE2_VOLUME,
        {"unit_of_measurement": UNIT_DECIBEL, "icon": "mdi:volume-high"},
        NUMBER_EXT_SCHEMA,
    ),
    "bass": (
        "set_bass_number",
        NumberFunction.BASS,
        {"icon": "mdi:knob", "unit_of_measurement": "%"},
        BASS_TREBLE_SCHEMA,
    ),
    "treble": (
        "set_treble_number",
        NumberFunction.TREBLE,
        {"icon": "mdi:knob", "unit_of_measurement": "%"},
        BASS_TREBLE_SCHEMA,
    ),
    "center_distance": (
        "set_center_distance_number",
        NumberFunction.CENTER_DISTANCE,
        {"icon": "mdi:speaker", **_DISTANCE},
        DISTANCE_SCHEMA,
    ),
    "front_left_distance": (
        "set_front_left_distance_number",
        NumberFunction.FRONT_LEFT_DISTANCE,
        {"icon": "mdi:speaker", **_DISTANCE},
        DISTANCE_SCHEMA,
    ),
    "front_right_distance": (
        "set_front_right_distance_number",
        NumberFunction.FRONT_RIGHT_DISTANCE,
        {"icon": "mdi:speaker", **_DISTANCE},
        DISTANCE_SCHEMA,
    ),
    "surround_left_distance": (
        "set_surround_left_distance_number",
        NumberFunction.SURROUND_LEFT_DISTANCE,
        {"icon": "mdi:speaker-wireless", **_DISTANCE},
        DISTANCE_SCHEMA,
    ),
    "surround_right_distance": (
        "set_surround_right_distance_number",
        NumberFunction.SURROUND_RIGHT_DISTANCE,
        {"icon": "mdi:speaker-wireless", **_DISTANCE},
        DISTANCE_SCHEMA,
    ),
    # mdi:subwoofer does not exist in the shipped icon set, so the subwoofer
    # reuses the plain speaker icon instead of rendering as a blank tile.
    "subwoofer_distance": (
        "set_subwoofer_distance_number",
        NumberFunction.SUBWOOFER_DISTANCE,
        {"icon": "mdi:speaker", **_DISTANCE},
        DISTANCE_SCHEMA,
    ),
    "dimmer": (
        "set_dimmer_number",
        NumberFunction.DIMMER,
        {
            "icon": "mdi:brightness-6",
            "unit_of_measurement": "%",
            "entity_category": ENTITY_CATEGORY_CONFIG,
        },
        DIMMER_SCHEMA,
    ),
    "tuner_fm_frequency": (
        "set_tuner_fm_frequency_number",
        NumberFunction.TUNER_FM_FREQUENCY,
        {"icon": "mdi:radio-fm", "unit_of_measurement": "MHz"},
        FM_FREQUENCY_SCHEMA,
    ),
    "tuner_am_frequency": (
        "set_tuner_am_frequency_number",
        NumberFunction.TUNER_AM_FREQUENCY,
        {"icon": "mdi:radio-am", "unit_of_measurement": "kHz"},
        AM_FREQUENCY_SCHEMA,
    ),
}

CONFIG_SCHEMA = YAMAHA_SERIAL_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(key): number.number_schema(YamahaNumber, **kwargs).extend(
            range_schema
        )
        for key, (_, _, kwargs, range_schema) in NUMBER_TYPES.items()
    }
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_YAMAHA_SERIAL_ID])

    for key, (setter, function, _, _) in NUMBER_TYPES.items():
        if conf := config.get(key):
            var = await number.new_number(
                conf,
                min_value=float(conf[CONF_MIN_VALUE]),
                max_value=float(conf[CONF_MAX_VALUE]),
                step=float(conf[CONF_STEP]),
            )
            cg.add(var.set_parent(paren))
            cg.add(var.set_function(function))
            cg.add(getattr(paren, setter)(var))
