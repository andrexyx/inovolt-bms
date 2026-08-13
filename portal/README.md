# Portal development

The portal uses no external assets, fonts, analytics or cloud APIs. Firmware must
serve these files locally and implement the following JSON endpoints:

- `GET /api/wifi/scan` -> `[{"ssid":"...","rssi":-55,"secure":true}]`
- `GET /api/bms/scan` -> `[{"name":"TP_...","mac":"...","rssi":-60}]`
- `POST /api/config` -> accepts the validated configuration documented below

```json
{
  "wifi": { "ssid": "Customer LAN", "password": "not-logged" },
  "batteries": [
    {
      "slot": 1,
      "mac": "AA:BB:CC:DD:EE:01",
      "advertised_name": "TP_EXAMPLE",
      "friendly_name": "Battery rack 1"
    }
  ]
}
```

When the API is unavailable, the page enters an explicit development/demo mode
using built-in fictional devices. Demo persistence strips the Wi-Fi password.

Run the pure logic tests with:

```bash
node --test portal/core.test.mjs
```
