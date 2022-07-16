import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, ble_client, sensor, binary_sensor
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    
    CONF_TEMPERATURE,
    CONF_BATTERY_LEVEL,
    
    CONF_ENTITY_CATEGORY,
    ENTITY_CATEGORY_DIAGNOSTIC,
    
    STATE_CLASS_MEASUREMENT,
    UNIT_PERCENT,
    UNIT_CELSIUS,
    
    CONF_DEVICE_CLASS,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_PROBLEM
)

CODEOWNERS = ["@dmitry-cherkas"]
DEPENDENCIES = ["ble_client"]
# load zero-configuration dependencies automatically
AUTO_LOAD = ["sensor", "binary_sensor", "esp32_ble_tracker"]

CONF_PIN_CODE = 'pin_code'
CONF_SECRET_KEY = 'secret_key'
CONF_PROBLEMS = 'problems'

eco_ns = cg.esphome_ns.namespace("danfoss_eco")
DanfossEco = eco_ns.class_(
    "Device", climate.Climate, ble_client.BLEClientNode, cg.PollingComponent
)

def validate_secret(value):
    value = cv.string_strict(value)
    if len(value) != 32:
        raise cv.Invalid("Secret key should be exactly 16 bytes (32 chars)")
    return value

def validate_pin(value):
    value = cv.string_strict(value)
    if len(value) != 4:
        raise cv.Invalid("PIN code should be exactly 4 chars")
    if not value.isnumeric():
        raise cv.Invalid("PIN code should be numeric")
    return value

CONFIG_SCHEMA = (
    climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(DanfossEco),
            cv.Optional(CONF_SECRET_KEY): validate_secret,
            cv.Optional(CONF_PIN_CODE): validate_pin,
            cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_BATTERY,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC
            ),
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_PROBLEMS): binary_sensor.BINARY_SENSOR_SCHEMA.extend({
                cv.Optional(CONF_NAME): cv.string,
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_DIAGNOSTIC): cv.entity_category,
                cv.Optional(CONF_DEVICE_CLASS, default=DEVICE_CLASS_PROBLEM): binary_sensor.validate_device_class
            })
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
    
    cg.add(var.set_secret_key(config.get(CONF_SECRET_KEY, "")))
    cg.add(var.set_pin_code(config.get(CONF_PIN_CODE, "")))
    
    if CONF_BATTERY_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_LEVEL])
        cg.add(var.set_battery_level(sens))
    if CONF_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature(sens))
    if CONF_PROBLEMS in config:
        b_sens = await binary_sensor.new_binary_sensor(config[CONF_PROBLEMS])
        cg.add(var.set_problems(b_sens))
    
