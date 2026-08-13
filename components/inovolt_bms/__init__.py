import esphome.codegen as cg
from esphome.components import binary_sensor, ble_client, esp32_ble_tracker, sensor, text_sensor
from esphome.components.inovolt_portal import InoVoltPortal
import esphome.config_validation as cv
from esphome.const import (
    CONF_DEVICE_ID, CONF_ID, CONF_NAME, DEVICE_CLASS_BATTERY, DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_POWER, DEVICE_CLASS_TEMPERATURE, DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT, UNIT_AMPERE, UNIT_CELSIUS, UNIT_PERCENT,
    UNIT_VOLT, UNIT_WATT,
)
from esphome.core.config import Device

CODEOWNERS = ["@andrexyx"]
DEPENDENCIES = ["esp32_ble_tracker", "ble_client", "inovolt_portal"]
AUTO_LOAD = ["binary_sensor", "sensor", "text_sensor"]

inovolt_bms_ns = cg.esphome_ns.namespace("inovolt_bms")
InoVoltBmsComponent = inovolt_bms_ns.class_(
    "InoVoltBmsComponent", cg.Component, esp32_ble_tracker.ESPBTDeviceListener
)
InoVoltSensor = inovolt_bms_ns.class_("InoVoltSensor", sensor.Sensor)
InoVoltBinarySensor = inovolt_bms_ns.class_("InoVoltBinarySensor", binary_sensor.BinarySensor)
InoVoltTextSensor = inovolt_bms_ns.class_("InoVoltTextSensor", text_sensor.TextSensor)

CONF_PORTAL_ID = "portal_id"
CONF_CLIENTS = "clients"
CONF_DEVICES = "devices"

METRICS = (
    ("soc", UNIT_PERCENT, 0, DEVICE_CLASS_BATTERY),
    ("voltage", UNIT_VOLT, 2, DEVICE_CLASS_VOLTAGE),
    ("current", UNIT_AMPERE, 2, DEVICE_CLASS_CURRENT),
    ("power", UNIT_WATT, 1, DEVICE_CLASS_POWER),
    ("charge_power", UNIT_WATT, 1, DEVICE_CLASS_POWER),
    ("discharge_power", UNIT_WATT, 1, DEVICE_CLASS_POWER),
    # SOH is a percentage measurement, not the device battery level shown by HA.
    ("health", UNIT_PERCENT, 0, ""),
    ("average_temperature", UNIT_CELSIUS, 1, DEVICE_CLASS_TEMPERATURE),
    ("ambient_temperature", UNIT_CELSIUS, 1, DEVICE_CLASS_TEMPERATURE),
    ("mosfet_temperature", UNIT_CELSIUS, 1, DEVICE_CLASS_TEMPERATURE),
    ("nominal_capacity", "Ah", 2, ""),
    ("remaining_capacity", "Ah", 2, ""),
    ("cycles", "", 0, ""),
    ("voltage_protection", "", 0, ""),
    ("current_protection", "", 0, ""),
    ("temperature_protection", "", 0, ""),
    ("alarms", "", 0, ""),
    ("switch_state", "", 0, ""),
    ("balancing_mask", "", 0, ""),
    ("min_cell_voltage", UNIT_VOLT, 3, DEVICE_CLASS_VOLTAGE),
    ("max_cell_voltage", UNIT_VOLT, 3, DEVICE_CLASS_VOLTAGE),
    ("average_cell_voltage", UNIT_VOLT, 3, DEVICE_CLASS_VOLTAGE),
    ("cell_delta", UNIT_VOLT, 3, DEVICE_CLASS_VOLTAGE),
    ("min_cell_number", "", 0, ""),
    ("max_cell_number", "", 0, ""),
    *((f"temperature_{index}", UNIT_CELSIUS, 1, DEVICE_CLASS_TEMPERATURE) for index in range(1, 9)),
    *((f"cell_{index}", UNIT_VOLT, 3, DEVICE_CLASS_VOLTAGE) for index in range(1, 25)),
)

ENTITY_SCHEMA = {}
BINARY_METRICS = ("online", "charging", "discharging", "limiting_current", "balancing", *((f"cell_{i}_balancing") for i in range(1, 17)))
TEXT_METRICS = ("software_version", "model", "voltage_protection_text", "current_protection_text", "temperature_protection_text", "errors")
for slot in range(4):
    for metric, unit, accuracy, device_class in METRICS:
        key = f"battery_{slot + 1}_{metric}"
        ENTITY_SCHEMA[cv.Optional(key, default={CONF_NAME: f"Battery {slot + 1} {metric.title()}"})] = sensor.sensor_schema(
            InoVoltSensor,
            unit_of_measurement=unit,
            accuracy_decimals=accuracy,
            device_class=device_class,
            state_class=STATE_CLASS_MEASUREMENT,
        )
    for metric in BINARY_METRICS:
        key = f"battery_{slot + 1}_{metric}"
        ENTITY_SCHEMA[cv.Optional(key, default={CONF_NAME: f"Battery {slot + 1} {metric.title()}"})] = binary_sensor.binary_sensor_schema(InoVoltBinarySensor)
    for metric in TEXT_METRICS:
        key = f"battery_{slot + 1}_{metric}"
        ENTITY_SCHEMA[cv.Optional(key, default={CONF_NAME: f"Battery {slot + 1} {metric.title()}"})] = text_sensor.text_sensor_schema(InoVoltTextSensor)

CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(): cv.declare_id(InoVoltBmsComponent),
        cv.Required(CONF_PORTAL_ID): cv.use_id(InoVoltPortal),
        cv.Required(CONF_CLIENTS): cv.All(cv.ensure_list(cv.use_id(ble_client.BLEClient)), cv.Length(min=4, max=4)),
        cv.Required(CONF_DEVICES): cv.All(cv.ensure_list(cv.use_id(Device)), cv.Length(min=4, max=4)),
        **ENTITY_SCHEMA,
    })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)
    portal = await cg.get_variable(config[CONF_PORTAL_ID])
    cg.add(var.set_portal(portal))
    for index, client_id in enumerate(config[CONF_CLIENTS]):
        client = await cg.get_variable(client_id)
        cg.add(var.set_client(index, client))
    devices = []
    for index, device_id in enumerate(config[CONF_DEVICES]):
        device = await cg.get_variable(device_id)
        devices.append(device_id)
        cg.add(var.set_device(index, device))
    for slot in range(4):
        for metric_index, (metric, _, _, _) in enumerate(METRICS):
            sensor_config = dict(config[f"battery_{slot + 1}_{metric}"])
            sensor_config[CONF_DEVICE_ID] = devices[slot]
            sens = await sensor.new_sensor(sensor_config)
            cg.add(var.set_sensor(slot, metric_index, sens))
        for metric_index, metric in enumerate(BINARY_METRICS):
            entity_config = dict(config[f"battery_{slot + 1}_{metric}"])
            entity_config[CONF_DEVICE_ID] = devices[slot]
            entity = await binary_sensor.new_binary_sensor(entity_config)
            cg.add(var.set_binary_sensor(slot, metric_index, entity))
        for metric_index, metric in enumerate(TEXT_METRICS):
            entity_config = dict(config[f"battery_{slot + 1}_{metric}"])
            entity_config[CONF_DEVICE_ID] = devices[slot]
            entity = await text_sensor.new_text_sensor(entity_config)
            cg.add(var.set_text_sensor(slot, metric_index, entity))
