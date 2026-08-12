import esphome.codegen as cg
from esphome.components import esp32_ble_tracker
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@andrexyx"]
DEPENDENCIES = ["esp32_ble_tracker"]
AUTO_LOAD = ["binary_sensor", "sensor", "text_sensor"]

inovolt_bms_ns = cg.esphome_ns.namespace("inovolt_bms")
InoVoltBmsComponent = inovolt_bms_ns.class_(
    "InoVoltBmsComponent", cg.Component, esp32_ble_tracker.ESPBTDeviceListener
)

CONFIG_SCHEMA = (
    cv.Schema({cv.GenerateID(): cv.declare_id(InoVoltBmsComponent)})
    .extend(cv.COMPONENT_SCHEMA)
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)
