import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, ble_client
from esphome.const import CONF_ID, CONF_UNIT_OF_MEASUREMENT

CODEOWNERS = ["@dmitry-cherkas"]
DEPENDENCIES = ["ble_client"]

CONF_SECRET_KEY = 'secret_key'

eco_ns = cg.esphome_ns.namespace("danfoss_eco")
DanfossEco = eco_ns.class_(
    "Device", climate.Climate, ble_client.BLEClientNode, cg.PollingComponent
)

def validate_password(value):
    value = cv.string_strict(value)
    if len(value) != 32:
        raise cv.Invalid("Secret key should be exactly 16 bytes (32 chars)")
    return value

CONFIG_SCHEMA = (
    climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(DanfossEco),
            cv.Required(CONF_SECRET_KEY): validate_password,
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
