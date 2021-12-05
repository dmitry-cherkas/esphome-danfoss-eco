import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, esp32_ble_tracker
from esphome.const import CONF_ID

AUTO_LOAD = ["esp32_ble_tracker"]

CONF_READ_SECRET = 'read_secret'

scanner_ns = cg.esphome_ns.namespace("danfoss_eco_scanner")
DanfossEcoScanner = scanner_ns.class_(
    "DanfossEcoScanner", cg.Component, esp32_ble_tracker.ESPBTDeviceListener
)

CONFIG_SCHEMA = cv.All(
    cv.ENTITY_BASE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(DanfossEcoScanner),
            cv.Optional(CONF_READ_SECRET, default=False): cv.boolean,
        }
    )
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)

    if CONF_READ_SECRET in config:
        cg.add(var.set_read_secret(config[CONF_READ_SECRET]))
