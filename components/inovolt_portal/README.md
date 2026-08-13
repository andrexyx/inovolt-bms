# InoVolt provisioning portal

This ESPHome component embeds the dependency-free UI from `portal/` and exposes
its local API through `web_server_base`.

## Endpoints

- `GET /api/wifi/scan` starts a Wi-Fi scan and returns visible networks.
- `GET /api/bms/scan` starts BLE scanning and returns devices named `TP_...`.
- `POST /api/config` validates the customer Wi-Fi details and one to four unique
  battery selections, stores the battery metadata in ESP preferences, saves the
  Wi-Fi station credentials through ESPHome and schedules a reboot.

The Wi-Fi password is passed directly to ESPHome and is not copied into the
component's own preferences record. The portal contains no cloud calls or remote
assets.

## Rebuild embedded assets

After changing a file in `portal/`, regenerate the committed header:

```bash
python scripts/build_portal_header.py
```

The saved selection configures four reusable BLE client slots at boot. The BMS
runtime polls configured batteries sequentially and hides unused Home Assistant
entities.
