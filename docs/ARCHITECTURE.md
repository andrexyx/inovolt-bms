# InoVolt firmware architecture

This document defines the target behaviour before portal implementation begins.

## Provisioning state machine

1. On first boot, start access point `inovolt` with password `12345678`.
2. Redirect captive-portal probes to the local InoVolt setup page.
3. Scan for local Wi-Fi networks and collect the customer's SSID and password.
4. Scan BLE advertisements and display only names beginning with `TP_`.
5. Allow one to six unique MAC addresses to be selected and named.
6. Store settings in ESP32 non-volatile storage and reboot.
7. Join the customer's LAN, advertise the gateway through mDNS and expose the
   native ESPHome API for Home Assistant discovery.
8. Disable the setup access point after a successful LAN connection.
9. Re-enter setup mode after the documented physical recovery action or repeated
   Wi-Fi boot failures.

The setup page must never display or log stored passwords after submission.

## Local network and Home Assistant

The `inovolt` access point is a temporary onboarding network, not the operational
network. Normal operation takes place entirely on the customer's local Wi-Fi:

- Home Assistant discovers the gateway through mDNS/zeroconf.
- Battery entities are transported through the native ESPHome API.
- The device remains usable when the internet connection is unavailable.
- No battery telemetry, Wi-Fi password or Home Assistant credential is sent to an
  InoVolt service or other cloud endpoint.

The production portal will create or provision a unique API encryption key per
gateway. The customer must be able to copy it during setup and recover it locally;
the repository must never contain a shared API key.

## Battery lifecycle

Each configured battery has a stable slot, friendly name and BLE MAC address.
The scheduler connects to batteries sequentially, retrieves the complete
Tianpower frame set, publishes the values, disconnects and advances to the next
slot. A failed or out-of-range battery must not block the remaining slots.

Target data includes:

- online, charging, discharging, current limiting and balancing state
- pack voltage, current, power, SOC, SOH and remaining/nominal capacity
- charging cycles and temperatures
- minimum, maximum, average and delta cell voltage
- individual voltages and balancing state for cells 1–16
- protection, alarm and error bitmasks plus readable descriptions
- hardware model and software version

## Home Assistant representation

Only configured batteries are registered. Each battery appears as a separate
device under one InoVolt gateway. Entity unique IDs are derived from the gateway
chip ID, battery slot and sensor key; the raw MAC address is not used in a visible
entity name.

The native ESPHome API over the customer's LAN is the required transport. Runtime entity creation
must happen before the API exposes the entity list. MQTT Discovery is the fallback
for development only if native dynamic registration proves unreliable across
supported ESPHome versions; it is not part of the default customer workflow.

## Board profiles

- `esp32-s3`: production/default profile, PSRAM recommended, up to six batteries
- `esp32`: standard profile, up to three batteries
- `esp32-c3`: compact profile, one or two batteries

Profiles may tune scan windows, connection slots and polling intervals. Selecting
more batteries than a profile supports must produce a clear validation error.
