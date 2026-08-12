# Verification checklist

## Automated

- Decode captured 20-byte TP response frames.
- Reject invalid frame boundaries and response markers.
- Validate status, capacity, temperatures and 24 cell-voltage slots.
- Validate ESPHome configuration for ESP32-C3 and ESP32-S3.
- Compile both firmware profiles.
- Verify the embedded portal contains no remote requests.

## Hardware

- First boot opens `inovolt` / `12345678`.
- Android and iOS captive-page probes open the local page.
- Scan returns only devices whose advertised name begins with `TP_`.
- Save one, two and six battery configurations.
- Join a 2.4 GHz customer network and reconnect after power loss.
- Home Assistant discovers only the selected batteries.
- Compare voltage, current, state of charge, temperature and all cells with the battery display.
- Confirm sequential polling keeps Wi-Fi and native API responsive.
- Confirm OTA and physical recovery mode.
