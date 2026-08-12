import esphome.codegen as cg
from esphome.components import esp32_ble_tracker, web_server_base, wifi
from esphome.components.web_server_base import CONF_WEB_SERVER_BASE_ID
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@andrexyx"]
DEPENDENCIES = ["wifi", "esp32_ble_tracker"]
AUTO_LOAD = ["json", "web_server_base"]

inovolt_portal_ns = cg.esphome_ns.namespace("inovolt_portal")
InoVoltPortal = inovolt_portal_ns.class_(
    "InoVoltPortal", cg.Component, esp32_ble_tracker.ESPBTDeviceListener
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(InoVoltPortal),
            cv.GenerateID(CONF_WEB_SERVER_BASE_ID): cv.use_id(
                web_server_base.WebServerBase
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
)


async def to_code(config):
    base = await cg.get_variable(config[CONF_WEB_SERVER_BASE_ID])
    var = cg.new_Pvariable(config[CONF_ID], base)
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)
    wifi.request_wifi_scan_results_lock()
