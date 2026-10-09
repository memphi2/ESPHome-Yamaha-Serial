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

CONF_VOLUME_DB = "volume_db"
CONF_ZONE2_VOLUME_DB = "zone2_volume_db"
CONF_BASS = "bass"
CONF_TREBLE = "treble"
CONF_CENTER_DISTANCE = "center_distance"
CONF_FRONT_LEFT_DISTANCE = "front_left_distance"
CONF_FRONT_RIGHT_DISTANCE = "front_right_distance"
CONF_SURROUND_LEFT_DISTANCE = "surround_left_distance"
CONF_SURROUND_RIGHT_DISTANCE = "surround_right_distance"
CONF_SUBWOOFER_DISTANCE = "subwoofer_distance"
CONF_DIMMER = "dimmer"
CONF_TUNER_FM_FREQUENCY = "tuner_fm_frequency"
CONF_TUNER_AM_FREQUENCY = "tuner_am_frequency"

YamahaVolumeNumber = yamaha_serial_ns.class_("YamahaVolumeNumber", number.Number)
YamahaZone2VolumeNumber = yamaha_serial_ns.class_(
    "YamahaZone2VolumeNumber", number.Number
)
YamahaBassNumber = yamaha_serial_ns.class_("YamahaBassNumber", number.Number)
YamahaTrebleNumber = yamaha_serial_ns.class_("YamahaTrebleNumber", number.Number)
YamahaCenterDistanceNumber = yamaha_serial_ns.class_(
    "YamahaCenterDistanceNumber", number.Number
)
YamahaFrontLeftDistanceNumber = yamaha_serial_ns.class_(
    "YamahaFrontLeftDistanceNumber", number.Number
)
YamahaFrontRightDistanceNumber = yamaha_serial_ns.class_(
    "YamahaFrontRightDistanceNumber", number.Number
)
YamahaSurroundLeftDistanceNumber = yamaha_serial_ns.class_(
    "YamahaSurroundLeftDistanceNumber", number.Number
)
YamahaSurroundRightDistanceNumber = yamaha_serial_ns.class_(
    "YamahaSurroundRightDistanceNumber", number.Number
)
YamahaSubwooferDistanceNumber = yamaha_serial_ns.class_(
    "YamahaSubwooferDistanceNumber", number.Number
)
YamahaDimmerNumber = yamaha_serial_ns.class_("YamahaDimmerNumber", number.Number)
YamahaTunerFmFrequencyNumber = yamaha_serial_ns.class_(
    "YamahaTunerFmFrequencyNumber", number.Number
)
YamahaTunerAmFrequencyNumber = yamaha_serial_ns.class_(
    "YamahaTunerAmFrequencyNumber", number.Number
)

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
        cv.Optional(CONF_MAX_VALUE, default=100.0): cv.float_range(
            min=0.0, max=100.0
        ),
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
        cv.Optional(CONF_MAX_VALUE, default=100.0): cv.float_range(
            min=0.0, max=100.0
        ),
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
        cv.Optional(CONF_MIN_VALUE, default=530.0): cv.float_range(min=500.0, max=1800.0),
        cv.Optional(CONF_MAX_VALUE, default=1710.0): cv.float_range(min=500.0, max=1800.0),
        cv.Optional(CONF_STEP, default=1.0): cv.float_range(min=1.0, max=10.0),
    }
)

CONFIG_SCHEMA = YAMAHA_SERIAL_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(CONF_VOLUME_DB): number.number_schema(
            YamahaVolumeNumber,
            unit_of_measurement=UNIT_DECIBEL,
            icon="mdi:volume-high",
            entity_category=ENTITY_CATEGORY_CONFIG,
        ).extend(NUMBER_EXT_SCHEMA),
        cv.Optional(CONF_ZONE2_VOLUME_DB): number.number_schema(
            YamahaZone2VolumeNumber,
            unit_of_measurement=UNIT_DECIBEL,
            icon="mdi:volume-high",
        ).extend(NUMBER_EXT_SCHEMA),
        cv.Optional(CONF_BASS): number.number_schema(
            YamahaBassNumber,
            icon="mdi:knob",
            unit_of_measurement="%",
        ).extend(BASS_TREBLE_SCHEMA),
        cv.Optional(CONF_TREBLE): number.number_schema(
            YamahaTrebleNumber,
            icon="mdi:knob",
            unit_of_measurement="%",
        ).extend(BASS_TREBLE_SCHEMA),
        cv.Optional(CONF_CENTER_DISTANCE): number.number_schema(
            YamahaCenterDistanceNumber,
            icon="mdi:speaker",
            unit_of_measurement="cm",
            entity_category=ENTITY_CATEGORY_CONFIG,
        ).extend(DISTANCE_SCHEMA),
        cv.Optional(CONF_FRONT_LEFT_DISTANCE): number.number_schema(
            YamahaFrontLeftDistanceNumber,
            icon="mdi:speaker",
            unit_of_measurement="cm",
            entity_category=ENTITY_CATEGORY_CONFIG,
        ).extend(DISTANCE_SCHEMA),
        cv.Optional(CONF_FRONT_RIGHT_DISTANCE): number.number_schema(
            YamahaFrontRightDistanceNumber,
            icon="mdi:speaker",
            unit_of_measurement="cm",
            entity_category=ENTITY_CATEGORY_CONFIG,
        ).extend(DISTANCE_SCHEMA),
        cv.Optional(CONF_SURROUND_LEFT_DISTANCE): number.number_schema(
            YamahaSurroundLeftDistanceNumber,
            icon="mdi:speaker-wireless",
            unit_of_measurement="cm",
            entity_category=ENTITY_CATEGORY_CONFIG,
        ).extend(DISTANCE_SCHEMA),
        cv.Optional(CONF_SURROUND_RIGHT_DISTANCE): number.number_schema(
            YamahaSurroundRightDistanceNumber,
            icon="mdi:speaker-wireless",
            unit_of_measurement="cm",
            entity_category=ENTITY_CATEGORY_CONFIG,
        ).extend(DISTANCE_SCHEMA),
        cv.Optional(CONF_SUBWOOFER_DISTANCE): number.number_schema(
            YamahaSubwooferDistanceNumber,
            icon="mdi:speaker",
            unit_of_measurement="cm",
            entity_category=ENTITY_CATEGORY_CONFIG,
        ).extend(DISTANCE_SCHEMA),
        cv.Optional(CONF_DIMMER): number.number_schema(
            YamahaDimmerNumber,
            icon="mdi:brightness-6",
            unit_of_measurement="%",
            entity_category=ENTITY_CATEGORY_CONFIG,
        ).extend(DIMMER_SCHEMA),
        cv.Optional(CONF_TUNER_FM_FREQUENCY): number.number_schema(
            YamahaTunerFmFrequencyNumber,
            icon="mdi:radio-fm",
            unit_of_measurement="MHz",
        ).extend(FM_FREQUENCY_SCHEMA),
        cv.Optional(CONF_TUNER_AM_FREQUENCY): number.number_schema(
            YamahaTunerAmFrequencyNumber,
            icon="mdi:radio-am",
            unit_of_measurement="kHz",
        ).extend(AM_FREQUENCY_SCHEMA),
    }
)


async def _build_number(conf, paren, setter_name):
    var = await number.new_number(
        conf,
        min_value=float(conf[CONF_MIN_VALUE]),
        max_value=float(conf[CONF_MAX_VALUE]),
        step=float(conf[CONF_STEP]),
    )
    cg.add(var.set_parent(paren))
    cg.add(getattr(paren, setter_name)(var))


async def to_code(config):
    paren = await cg.get_variable(config[CONF_YAMAHA_SERIAL_ID])

    if conf := config.get(CONF_VOLUME_DB):
        await _build_number(conf, paren, "set_volume_number")

    if conf := config.get(CONF_ZONE2_VOLUME_DB):
        await _build_number(conf, paren, "set_zone2_volume_number")

    if conf := config.get(CONF_BASS):
        await _build_number(conf, paren, "set_bass_number")

    if conf := config.get(CONF_TREBLE):
        await _build_number(conf, paren, "set_treble_number")

    if conf := config.get(CONF_CENTER_DISTANCE):
        await _build_number(conf, paren, "set_center_distance_number")

    if conf := config.get(CONF_FRONT_LEFT_DISTANCE):
        await _build_number(conf, paren, "set_front_left_distance_number")

    if conf := config.get(CONF_FRONT_RIGHT_DISTANCE):
        await _build_number(conf, paren, "set_front_right_distance_number")

    if conf := config.get(CONF_SURROUND_LEFT_DISTANCE):
        await _build_number(conf, paren, "set_surround_left_distance_number")

    if conf := config.get(CONF_SURROUND_RIGHT_DISTANCE):
        await _build_number(conf, paren, "set_surround_right_distance_number")

    if conf := config.get(CONF_SUBWOOFER_DISTANCE):
        await _build_number(conf, paren, "set_subwoofer_distance_number")

    if conf := config.get(CONF_DIMMER):
        await _build_number(conf, paren, "set_dimmer_number")

    if conf := config.get(CONF_TUNER_FM_FREQUENCY):
        await _build_number(conf, paren, "set_tuner_fm_frequency_number")

    if conf := config.get(CONF_TUNER_AM_FREQUENCY):
        await _build_number(conf, paren, "set_tuner_am_frequency_number")
