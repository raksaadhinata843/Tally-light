# Tally Light System

A multi-unit wireless tally light system for video production, integrating with vMix via UDP multicast. Built on ESP32 and ESP8266 with WS2812B RGB LEDs.

---

## How It Works

```
vMix PC
  └── app.py (polls vMix API)
        └── Serial (USB) ──► ESP32 TX (UDP)
                                  └── WiFi Multicast 239.1.2.3:4210
                                        ├── ESP32 RX  (CAM 1)  ── WS2812B
                                        ├── ESP32 RX  (CAM 2)  ── WS2812B
                                        ├── ESP8266 RX (CAM 3) ── WS2812B
                                        └── ESP8266 RX (CAM 4) ── WS2812B
```

The transmitter reads tally status from vMix and broadcasts a 2-byte packet over WiFi multicast. Every receiver on the same network receives the packet and checks if its assigned camera (`CAM_ID`) is on Program or Preview.

### Tally Packet Structure

```
[ pgm_mask (1 byte) | pvw_mask (1 byte) ]
```

Each bit represents one camera:
- Bit 0 = Camera 0
- Bit 1 = Camera 1
- Bit 2 = Camera 2
- Bit 3 = Camera 3
- ... up to 8 cameras (bits 0–7)

---

## Hardware

### Transmitter (ESP32)
| Pin | Function |
|---|---|
| 12, 13, 25, 26 | PGM input (Camera 1–4, active LOW) |
| 27, 32, 33, 34 | PVW input (Camera 1–4, active LOW) |

Connect to switcher tally outputs via optocoupler or direct GPIO (3.3V logic).

### Receiver (ESP32 / ESP8266)
| Pin | Function |
|---|---|
| GPIO 5 (ESP32) | WS2812B data pin |
| D2 / GPIO 4 (ESP8266) | WS2812B data pin |

---

## LED Status

### During Boot
| LED | Meaning |
|---|---|
| Blinking red | Connecting to WiFi |
| Flashing white N times | Showing CAM_ID (see below) |
| Solid blue | Connected and ready |

### During Operation
| LED | Meaning |
|---|---|
| Red | Camera is LIVE (Program) |
| Green | Camera is on PREVIEW |
| Blue | Camera is IDLE |
| Blinking red | WiFi lost, reconnecting |

---

## CAM_ID Identification

At boot, after connecting to WiFi, the LED flashes **white** to tell you which camera ID is assigned to that unit — no serial monitor needed.

| CAM_ID | White flashes |
|---|---|
| CAM0 | 1 flash |
| CAM1 | 2 flashes |
| CAM2 | 3 flashes |
| CAM3 | 4 flashes |

---

## Project Setup

### 1. Install PlatformIO
Install the [PlatformIO extension](https://platformio.org/install/ide?install=vscode) in VS Code.

### 2. Clone / Open the Project
Open the project folder in VS Code. PlatformIO will detect `platformio.ini` automatically.

### 3. Configure WiFi Credentials
Copy `secrets.ini.example` and rename it to `secrets.ini`:

```ini
[wifi]
ssid = YourNetworkName
password = YourPassword
```

> `secrets.ini` is gitignored — your credentials will never be pushed to GitHub.

### 4. Configure vMix Bridge (`src/app.py`)
Edit the following lines to match your setup:

```python
ser = serial.Serial('COM4', 115200)   # your COM port
response = requests.get('http://192.168.1.100:58088/api/')  # your vMix IP
```

Run `app.py` on the Windows PC connected to the TX ESP32 via USB.

---

## Flashing Guide

### Transmitter

| Environment | Use when |
|---|---|
| `esp32UDP_TX` | TX reads from vMix via `app.py` over Serial |

Select the environment in the PlatformIO toolbar → click **Upload**.

### Receiver (WS2812B LED)

No code changes needed. Just select the right environment for each unit:

| Environment | Hardware | Camera |
|---|---|---|
| `esp32UDPWS_RX_CAM0` | ESP32 | Camera 1 |
| `esp32UDPWS_RX_CAM1` | ESP32 | Camera 2 |
| `esp32UDPWS_RX_CAM2` | ESP32 | Camera 3 |
| `esp32UDPWS_RX_CAM3` | ESP32 | Camera 4 |
| `esp8266UDPWS_RX_CAM0` | ESP8266 | Camera 1 |
| `esp8266UDPWS_RX_CAM1` | ESP8266 | Camera 2 |
| `esp8266UDPWS_RX_CAM2` | ESP8266 | Camera 3 |
| `esp8266UDPWS_RX_CAM3` | ESP8266 | Camera 4 |

**Workflow for flashing multiple units:**
1. Plug in ESP for Camera 1 → select `_CAM0` → Upload
2. Plug in ESP for Camera 2 → select `_CAM1` → Upload
3. Repeat for remaining units

---

## WiFi Reconnect Behavior

Both TX and RX handle WiFi drops automatically:

- **TX**: If WiFi drops, it stops sending and retries connection every 10 seconds.
- **RX**: If WiFi drops, the LED blinks red and calls `WiFi.reconnect()` in a loop. Once reconnected, it re-joins the multicast group and resumes receiving tally packets.

---

## Scaling

| Limit | Value | How to expand |
|---|---|---|
| Max cameras | 8 | Change `uint8_t` masks to `uint16_t` (16 cameras) |
| Max TX inputs | 4 | Add more pins to `PGM_PINS` / `PVW_PINS` arrays |
| Max RX units | Unlimited | Flash more units with different `_CAMx` environments |

---

## File Structure

```
├── src/
│   ├── main.cpp        # All firmware (TX + RX), separated by #ifdef
│   └── app.py          # Python bridge: vMix API → Serial → TX ESP32
├── platformio.ini      # Build environments for all hardware targets
├── secrets.ini         # WiFi credentials (gitignored, you create this)
├── secrets.ini.example # Template for secrets.ini
└── .gitignore
```
