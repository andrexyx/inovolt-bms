# InoVolt BMS

Local ESP32 gateway for up to six Bluetooth batteries advertised as `TP_...`.

## Product behaviour

1. On first boot the ESP32 creates Wi-Fi network `inovolt` with password `12345678`.
2. The local captive page scans customer Wi-Fi networks and nearby `TP_...` batteries.
3. The customer selects and names between one and six batteries.
4. The ESP32 stores the configuration locally, joins the customer LAN and reboots.
5. Selected batteries are polled sequentially and exposed through the native ESPHome API.

Each selected battery is a separate Home Assistant device. Selecting two
batteries creates only `InoVolt Battery 1` and `InoVolt Battery 2`; slots 3–6 are
not registered or exposed.

The BLE scheduler connects to one battery at a time, requests the complete frame
set, disconnects, and advances to the next configured slot. A missing response
times out without blocking Wi-Fi or the Home Assistant API.

No MQTT broker, cloud account or external web asset is required.

## Hardware profiles

- ESP32-S3 with PSRAM: recommended for one to six batteries.
- ESP32-C3: supported for one or two batteries.

This clean implementation is under active development. Do not install it in a
customer system until the firmware build and real-battery verification checklist
in `docs/TESTING.md` is complete.
