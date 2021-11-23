import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, ble_client, sensor
from esphome.const import (
    CONF_ID,
    CONF_UNIT_OF_MEASUREMENT,
    DEVICE_CLASS_BATTERY,
    CONF_BATTERY_LEVEL,
    UNIT_PERCENT,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT
)

CODEOWNERS = ["@dmitry-cherkas"]
DEPENDENCIES = ["ble_client"]

CONF_PIN_CODE = 'pin_code'
CONF_SECRET_KEY = 'secret_key'

eco_ns = cg.esphome_ns.namespace("danfoss_eco")
DanfossEco = eco_ns.class_(
    "Device", climate.Climate, ble_client.BLEClientNode, cg.PollingComponent
)

def validate_secret(value):
    value = cv.string_strict(value)
    if len(value) != 32:
        raise cv.Invalid("Secret key should be exactly 16 bytes (32 chars)")
    return value

CONFIG_SCHEMA = (
    climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(DanfossEco),
            cv.Required(CONF_SECRET_KEY): validate_secret,
            cv.Optional(CONF_PIN_CODE): cv.string_strict,
            cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_BATTERY,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC
            )
        }
    )
    .extend(ble_client.BLE_CLIENT_SCHEMA)
    .extend(cv.polling_component_schema("60s"))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    await ble_client.register_ble_node(var, config)
    cg.add(var.set_secret_key(config[CONF_SECRET_KEY]))
    cg.add(var.set_pin_code(config.get(CONF_PIN_CODE, "")))
    if CONF_BATTERY_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_LEVEL])
        cg.add(var.set_battery_level(sens))
