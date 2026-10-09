import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC

from . import CONF_YAMAHA_SERIAL_ID, YAMAHA_SERIAL_COMPONENT_SCHEMA

DEPENDENCIES = ["yamaha_serial"]

CONF_POWER_STATE = "power_state"
CONF_INPUT_SOURCE = "input_source"
CONF_ZONE2_INPUT_SOURCE = "zone2_input_source"
CONF_PROGRAM = "program"
CONF_PLAYBACK_FORMAT = "playback_format"
CONF_SAMPLING_RATE = "sampling_rate"
CONF_MAIN_VOLUME_TEXT = "main_volume_text"
CONF_AUDIO_SELECT_STATE = "audio_select_state"
CONF_NIGHT_MODE_STATE = "night_mode_state"
CONF_LAST_ERROR = "last_error"
CONF_LAST_PARSE_ERROR = "last_parse_error"
CONF_CONNECTION_STATE = "connection_state"
CONF_RECEIVER_MODEL = "receiver_model"

CONFIG_SCHEMA = YAMAHA_SERIAL_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(CONF_POWER_STATE): text_sensor.text_sensor_schema(
            icon="mdi:power"
        ),
        cv.Optional(CONF_INPUT_SOURCE): text_sensor.text_sensor_schema(
            icon="mdi:audio-input-stereo-minijack"
        ),
        cv.Optional(CONF_ZONE2_INPUT_SOURCE): text_sensor.text_sensor_schema(
            icon="mdi:audio-input-stereo-minijack"
        ),
        cv.Optional(CONF_PROGRAM): text_sensor.text_sensor_schema(
            icon="mdi:music-circle-outline"
        ),
        cv.Optional(CONF_PLAYBACK_FORMAT): text_sensor.text_sensor_schema(
            icon="mdi:music-note-outline"
        ),
        cv.Optional(CONF_SAMPLING_RATE): text_sensor.text_sensor_schema(
            icon="mdi:waveform",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_MAIN_VOLUME_TEXT): text_sensor.text_sensor_schema(
            icon="mdi:volume-high"
        ),
        cv.Optional(CONF_AUDIO_SELECT_STATE): text_sensor.text_sensor_schema(
            icon="mdi:audio-input-stereo-minijack"
        ),
        cv.Optional(CONF_NIGHT_MODE_STATE): text_sensor.text_sensor_schema(
            icon="mdi:weather-night"
        ),
        cv.Optional(CONF_LAST_ERROR): text_sensor.text_sensor_schema(
            icon="mdi:alert-circle-outline",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_LAST_PARSE_ERROR): text_sensor.text_sensor_schema(
            icon="mdi:file-search-outline",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_CONNECTION_STATE): text_sensor.text_sensor_schema(
            icon="mdi:lan-connect",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_RECEIVER_MODEL): text_sensor.text_sensor_schema(
            icon="mdi:information-outline",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_YAMAHA_SERIAL_ID])

    if conf := config.get(CONF_POWER_STATE):
        var = await text_sensor.new_text_sensor(conf)
        cg.add(paren.set_power_state_text_sensor(var))

    if conf := config.get(CONF_INPUT_SOURCE):
        var = await text_sensor.new_text_sensor(conf)
        cg.add(paren.set_input_source_text_sensor(var))

    if conf := config.get(CONF_ZONE2_INPUT_SOURCE):
        var = await text_sensor.new_text_sensor(conf)
        cg.add(paren.set_zone2_input_source_text_sensor(var))

    if conf := config.get(CONF_PROGRAM):
        var = await text_sensor.new_text_sensor(conf)
        cg.add(paren.set_program_text_sensor(var))

    if conf := config.get(CONF_PLAYBACK_FORMAT):
        var = await text_sensor.new_text_sensor(conf)
        cg.add(paren.set_playback_format_text_sensor(var))

    if conf := config.get(CONF_SAMPLING_RATE):
        var = await text_sensor.new_text_sensor(conf)
        cg.add(paren.set_sampling_rate_text_sensor(var))

    if conf := config.get(CONF_MAIN_VOLUME_TEXT):
        var = await text_sensor.new_text_sensor(conf)
        cg.add(paren.set_main_volume_text_sensor(var))

    if conf := config.get(CONF_AUDIO_SELECT_STATE):
        var = await text_sensor.new_text_sensor(conf)
        cg.add(paren.set_audio_select_text_sensor(var))

    if conf := config.get(CONF_NIGHT_MODE_STATE):
        var = await text_sensor.new_text_sensor(conf)
        cg.add(paren.set_night_mode_text_sensor(var))

    if conf := config.get(CONF_LAST_ERROR):
        var = await text_sensor.new_text_sensor(conf)
        cg.add(paren.set_last_error_text_sensor(var))

    if conf := config.get(CONF_LAST_PARSE_ERROR):
        var = await text_sensor.new_text_sensor(conf)
        cg.add(paren.set_last_parse_error_text_sensor(var))

    if conf := config.get(CONF_CONNECTION_STATE):
        var = await text_sensor.new_text_sensor(conf)
        cg.add(paren.set_connection_state_text_sensor(var))

    if conf := config.get(CONF_RECEIVER_MODEL):
        var = await text_sensor.new_text_sensor(conf)
        cg.add(paren.set_receiver_model_text_sensor(var))
