# Tally-light

A multi-platform wireless tally light system for video production environments, integrating with vMix.

## Project Type

This is an **embedded firmware project** — it compiles C++ code for microcontrollers (ESP32, ESP8266, Arduino Nano) using PlatformIO. There is no web frontend or backend server. The code cannot be run directly in Replit; it must be compiled and flashed onto physical hardware.

## Tech Stack

- **Build System:** PlatformIO
- **Languages:** C++ (Arduino framework), Python
- **Hardware Targets:** ESP32, ESP8266 (NodeMCU), Arduino Nano
- **Wireless Protocols:** ESP-NOW, WiFi UDP, nRF24L01, ESP-Mesh

## Project Structure

- `platformio.ini` — PlatformIO configuration with all build environments
- `src/main.cpp` — Monolithic firmware using `#ifdef` to switch between TX/RX modes
- `src/app.py` — Python bridge script: polls vMix HTTP API and sends tally data over Serial to hardware
- `include/` — PlatformIO headers directory
- `lib/` — PlatformIO local libraries directory

## Build Environments (platformio.ini)

| Environment | Platform | Role |
|---|---|---|
| `esp32VMIX_TX` | ESP32 | Transmitter reading from vMix via Serial |
| `esp32SWITCHER_TX` | ESP32 | Transmitter reading from hardware switcher pins |
| `esp32UDP_TX` | ESP32 | Transmitter broadcasting via UDP |
| `esp32UDP_RX` | ESP32 | Receiver via UDP |
| `esp32MESH_TX/RX` | ESP32 | Mesh network TX/RX |
| `nano_TX/RX` | Arduino Nano | TX/RX using nRF24L01 radio |
| `esp8266_RX` | ESP8266 | Receiver |
| `esp8266UDPWS_RX` | ESP8266 | Receiver via UDP with WS2812 LEDs |

## How to Use

1. **Install PlatformIO** (VS Code extension or CLI)
2. **Open the project** in VS Code with PlatformIO
3. **Select the environment** matching your hardware and use case
4. **Build and flash** to your microcontroller
5. **For vMix integration:** Configure `src/app.py` with your vMix IP and COM port, then run it on a Windows PC connected to the ESP32 transmitter via USB

## User Preferences
