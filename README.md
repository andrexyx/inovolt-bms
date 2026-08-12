# InoVolt BMS

InoVolt BMS connects Tianpower battery-management systems advertised as `TP_...`
to Home Assistant through an ESP32 and Bluetooth Low Energy.

> **Development status:** the inherited ESPHome component, manual examples and
> embedded provisioning portal are available now. The portal can scan and persist
> one to six `TP_...` batteries. Creating the Home Assistant entities dynamically
> from that saved selection is still under development; do not sell this branch as
> finished firmware.

## Goals

- InoVolt setup access point: `inovolt`
- temporary onboarding password: `12345678`
- branded captive configuration page
- connection to the customer's local Wi-Fi network
- BLE discovery restricted to Tianpower devices (`TP_...`)
- select, name and persist up to six batteries
- one Home Assistant device per selected battery
- full pack, cell, temperature, balancing and protection entities
- OTA updates and recovery setup mode
- local-only Home Assistant reporting through the native ESPHome API

## Supported boards

| Board | Recommended battery count | Notes |
| --- | ---: | --- |
| ESP32-S3 with PSRAM | 1–6 | Recommended InoVolt target; batteries will be polled sequentially. |
| Classic ESP32 | 1–3 | Suitable for smaller systems. |
| ESP32-C3 | 1–2 | Wi-Fi and BLE share one core; intended for small installations and testing. |

Six simultaneous BLE connections are not required. The production firmware will
connect, collect and release batteries in a controlled polling cycle to reduce RAM
pressure and improve Wi-Fi reliability.

## Install the current ESPHome component

```yaml
external_components:
  - source: github://andrexyx/inovolt-bms@main
    refresh: 0s
```

Start with one of these files:

- `esp32-ble-example.yaml` for one battery
- `esp32-ble-example-multiple-devices.yaml` for two batteries
- `examples/inovolt-esp32-c3.yaml` for the anonymized InoVolt C3 example

Copy `secrets.example.yaml` to `secrets.yaml`, enter your own values and never
commit that file.

## Roadmap

- [x] Import the proven Tianpower BLE protocol implementation
- [x] Host the external component from the InoVolt repository
- [x] Add anonymized board profiles and architecture documentation
- [x] Add the responsive InoVolt portal UI and tested local API contract
- [x] Embed the portal and implement its ESP32 firmware endpoints
- [x] Add `TP_...` scanning, selection and friendly names
- [x] Add persistent configuration for up to six batteries
- [ ] Add sequential multi-battery polling
- [ ] Register only configured batteries in Home Assistant
- [ ] Produce factory binaries and a beginner installation guide

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the implementation contract.

## Privacy and security

Do not publish BLE MAC addresses, Wi-Fi credentials, Home Assistant API keys or
MQTT credentials. Setup mode will be temporary: after successful provisioning,
the `inovolt` access point will be disabled and reopened only through a documented
physical recovery action. Battery telemetry is sent directly over the customer's
LAN to Home Assistant; the planned firmware does not require an InoVolt cloud.

## Credits and license

The Tianpower BLE protocol component is derived from
[`syssi/esphome-tianpower-bms`](https://github.com/syssi/esphome-tianpower-bms).
Thanks to syssi, xpinguinx and the upstream contributors for the protocol work and
device testing. InoVolt modifications are maintained by `andrexyx`.

Licensed under Apache License 2.0. See [LICENSE](LICENSE) and [NOTICE](NOTICE).
