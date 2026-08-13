# InoVolt BMS

## Card Home Assistant

Proiectul include acum un card Lovelace dedicat cu SOC, tensiune, curent, putere, 16 celule segmentate și indicatoare individuale de balansare. Vezi [tutorialul complet](docs/HOME_ASSISTANT_CARD.md) și [configurația exemplu](examples/home-assistant-card.yaml).

Local ESP32 gateway for up to four Bluetooth batteries advertised as `TP_...`.

## Product behaviour

1. On first boot the ESP32 creates Wi-Fi network `inovolt` with password `12345678`.
2. The local captive page scans customer Wi-Fi networks and nearby `TP_...` batteries.
3. The customer selects and names between one and four batteries.
4. The ESP32 stores the configuration locally, joins the customer LAN and reboots.
5. Selected batteries are read concurrently and exposed through the native ESPHome API.

Each selected battery exposes SOC, voltage, current, power, health and
temperature through Home Assistant. The friendly name chosen in the portal is
used as the entity-name prefix; unused slots are hidden.

The BLE runtime keeps up to four battery connections active, requests the complete frame
set independently for every slot and reconnects a slot when necessary. A missing response
times out without blocking Wi-Fi or the Home Assistant API.

No MQTT broker, cloud account or external web asset is required.

## Hardware profiles

- ESP32-S3 with PSRAM: recommended for one to four batteries.
- ESP32-C3: experimental support for one to four batteries using concurrent BLE polling.

### ESP32-C3 with integrated OLED

![ESP32-C3 test board with integrated OLED](docs/images/esp32-c3-oled-board.jpg)

For compact ESP32-C3 boards with an integrated SSD1306 72×40 OLED, use
`examples/inovolt-c3-oled.yaml`. The display alternates between the clock and
five-second pages for each configured battery. GPIO5/GPIO6 and address `0x3C`
match the current InoVolt test board. The safe default is 400 kHz I²C; 800 kHz
can be tested per board but is not required.

This clean implementation is under active development. Do not install it in a
customer system until the firmware build and real-battery verification checklist
in `docs/TESTING.md` is complete.
