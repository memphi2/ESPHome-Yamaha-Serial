import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_DECIBEL,
    UNIT_HERTZ,
    UNIT_MILLISECOND,
)

from . import CONF_YAMAHA_SERIAL_ID, YAMAHA_SERIAL_COMPONENT_SCHEMA

DEPENDENCIES = ["yamaha_serial"]

CONF_VOLUME_DB = "volume_db"
CONF_ZONE2_VOLUME_DB = "zone2_volume_db"
CONF_SAMPLING_RATE = "sampling_rate"
CONF_LAST_RESPONSE_AGE = "last_response_age"
CONF_COMMANDS_SENT = "commands_sent"
CONF_RESPONSES_RECEIVED = "responses_received"
CONF_PARSE_ERRORS = "parse_errors"
CONF_TIMEOUTS = "timeouts"
CONF_QUEUE_DROPS = "queue_drops"

CONFIG_SCHEMA = YAMAHA_SERIAL_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(CONF_VOLUME_DB): sensor.sensor_schema(
            unit_of_measurement=UNIT_DECIBEL,
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:volume-high",
        ),
        cv.Optional(CONF_ZONE2_VOLUME_DB): sensor.sensor_schema(
            unit_of_measurement=UNIT_DECIBEL,
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:volume-high",
        ),
        cv.Optional(CONF_SAMPLING_RATE): sensor.sensor_schema(
            unit_of_measurement=UNIT_HERTZ,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:sine-wave",
        ),
        cv.Optional(CONF_LAST_RESPONSE_AGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_MILLISECOND,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:timer-outline",
        ),
        cv.Optional(CONF_COMMANDS_SENT): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:send",
        ),
        cv.Optional(CONF_RESPONSES_RECEIVED): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:download",
        ),
        cv.Optional(CONF_PARSE_ERRORS): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:alert-circle-outline",
        ),
        cv.Optional(CONF_TIMEOUTS): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:timer-off-outline",
        ),
        cv.Optional(CONF_QUEUE_DROPS): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:delete-alert-outline",
        ),
    }
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_YAMAHA_SERIAL_ID])

    if conf := config.get(CONF_VOLUME_DB):
        var = await sensor.new_sensor(conf)
        cg.add(paren.set_volume_db_sensor(var))

    if conf := config.get(CONF_ZONE2_VOLUME_DB):
        var = await sensor.new_sensor(conf)
        cg.add(paren.set_zone2_volume_db_sensor(var))

    if conf := config.get(CONF_SAMPLING_RATE):
        var = await sensor.new_sensor(conf)
        cg.add(paren.set_sampling_rate_sensor(var))

    if conf := config.get(CONF_LAST_RESPONSE_AGE):
        var = await sensor.new_sensor(conf)
        cg.add(paren.set_last_response_age_sensor(var))

    if conf := config.get(CONF_COMMANDS_SENT):
        var = await sensor.new_sensor(conf)
        cg.add(paren.set_commands_sent_sensor(var))

    if conf := config.get(CONF_RESPONSES_RECEIVED):
        var = await sensor.new_sensor(conf)
        cg.add(paren.set_responses_received_sensor(var))

    if conf := config.get(CONF_PARSE_ERRORS):
        var = await sensor.new_sensor(conf)
        cg.add(paren.set_parse_errors_sensor(var))

    if conf := config.get(CONF_TIMEOUTS):
        var = await sensor.new_sensor(conf)
        cg.add(paren.set_timeouts_sensor(var))

    if conf := config.get(CONF_QUEUE_DROPS):
        var = await sensor.new_sensor(conf)
        cg.add(paren.set_queue_drops_sensor(var))
