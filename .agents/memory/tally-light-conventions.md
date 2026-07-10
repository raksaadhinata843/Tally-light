---
name: Tally-light firmware conventions
description: Design decisions for WS2812B RX modes, OTA, and the Tally Arbiter mode in the tally-light PlatformIO project.
---

## LED library
All WS2812B RX modes use Adafruit NeoPixel, not FastLED. Keep this consistent across ESP32 and ESP8266 WS variants.

**Why:** user standardized on NeoPixel for simpler brightness/color API across both chip families.

## ArduinoOTA placement
`ArduinoOTA.handle()` must be the first line of `loop()`, never inside `recon()`/reconnect helpers and never placed after an early `return`.

**Why:** OTA has to keep responding even during WiFi reconnect loops or a stuck early-return would make the device unflashable without USB.

## Per-device identity via build flags
For the vMix UDP WS RX modes, `CAM_ID` and WiFi credentials are injected via PlatformIO build flags (`secrets.ini`, per-CAM_ID environments) — never hardcoded in `main.cpp`. This lets the same source flash to N devices without code edits.

## Tally Arbiter mode (MODE_RX_ARBITER_ESP32 / MODE_RX_ARBITER_ESP8266)
A second, independent RX mode added alongside the vMix UDP modes (not a replacement). Uses Socket.IO/WebSocket connection to a Tally Arbiter server instead of UDP multicast from vMix.

- WiFi + Tally Arbiter server IP/port are configured via WiFiManager captive portal at first boot, not via secrets.ini/build flags — this mode is meant to be flashed once and reconfigured entirely over WiFi/OTA afterward.
- Device identity (deviceId/deviceName/taHost/taPort) persistence: ESP32 uses `Preferences`; ESP8266 has no `Preferences` library, so a small EEPROM-backed key/value helper (fixed-offset string slots) was written instead. Any future device settings added to this mode need a new fixed EEPROM offset for ESP8266.
- Libraries: `tzapu/WiFiManager`, `links2004/WebSockets` (provides both `WebSocketsClient.h` and `SocketIOclient.h`), `arduino-libraries/Arduino_JSON`, `adafruit/Adafruit NeoPixel`.
