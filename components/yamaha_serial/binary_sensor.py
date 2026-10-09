import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import DEVICE_CLASS_CONNECTIVITY, ENTITY_CATEGORY_DIAGNOSTIC

from . import CONF_YAMAHA_SERIAL_ID, YAMAHA_SERIAL_COMPONENT_SCHEMA

DEPENDENCIES = ["yamaha_serial"]

CONF_AVAILABILITY = "availability"

CONFIG_SCHEMA = YAMAHA_SERIAL_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(CONF_AVAILABILITY): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_CONNECTIVITY,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:lan-connect",
        )
    }
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_YAMAHA_SERIAL_ID])

    if conf := config.get(CONF_AVAILABILITY):
        var = await binary_sensor.new_binary_sensor(conf)
        cg.add(paren.set_availability_binary_sensor(var))
