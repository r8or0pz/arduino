# Arduino Wi-Fi Lamp Control

This repository contains Arduino sketches for controlling a lamp over HTTP.

## Project Layout

- `thermo/thermo.ino` - Wi-Fi lamp API + DHT11 telemetry + LCD output.
- `control-button/main/main.ino` - Wi-Fi lamp API sketch (currently in-progress).

## Hardware

### thermo sketch

- Arduino board with WiFiS3 support (for example UNO R4 WiFi)
- DHT11 sensor
- I2C LCD 16x2 (address `0x27`)
- LED module or relay for lamp control

Pin usage in `thermo/thermo.ino`:

- `DHTPIN = 2`
- `ledPin = 3`

## Required Libraries

Install from Arduino Library Manager:

- ArduinoJson
- LiquidCrystal I2C
- DHT sensor library

Built-in/board-provided:

- WiFiS3
- Wire

## Wi-Fi Credentials

Create a file named `secrets.h` in each sketch folder that needs it (for example `thermo/secrets.h`):

```cpp
#pragma once

#define SECRET_SSID "YOUR_WIFI_NAME"
#define SECRET_PASS "YOUR_WIFI_PASSWORD"
```

## thermo API

The sketch starts an HTTP server on port `80`.

### Set lamp state

`POST /api/lamp`

Body:

```json
{"status":"on"}
```

or

```json
{"status":"off"}
```

Example:

```bash
curl -X POST "http://<DEVICE_IP>/api/lamp" \
  -H "Content-Type: application/json" \
  -d '{"status":"on"}'
```

Expected response:

```json
{"result":"OK"}
```

Possible error responses:

- `400 Bad Request`
- `400 Invalid Status`
- `400 Malformed JSON`
- `405 Method Not Allowed`
- `404 Not Found`

## Display Behavior (thermo)

- LCD row 1: local IP address
- LCD row 2: temperature, humidity, and lamp state (`L:ON` or `L:OFF`)
- Sensor display refresh interval: 2 seconds

## Upload Steps

1. Open the sketch in Arduino IDE.
2. Select your board and port.
3. Install missing libraries if prompted.
4. Add `secrets.h` with valid Wi-Fi credentials.
5. Upload.
6. Open Serial Monitor at `115200` baud to see startup logs and IP.

## Notes

- `thermo/thermo.ino` contains request timeout protection to avoid hanging on incomplete clients.
- `control-button/main/main.ino` currently appears to be under active editing and may require cleanup before compiling.
