# Arduino Weather Station

This repository contains an Arduino weather-station sketch and shared local libraries.

## Project Layout

- `weather-station/weather-station.ino` - main sketch (rain sensor + LCD + touch backlight control)
- `lib/Sensors/` - shared sensor abstractions (`AnalogSensor`, `DigitalSensor`, `TouchSensor`, `RainSensor`)
- `lib/Connectivity/` - WiFi connection handling shared by backends
- `lib/Logger/` - shared serial logger abstraction with log levels
- `Makefile` - bootstrap, build, upload, monitor, and CI workflow

## Hardware

- Arduino UNO R4 WiFi (FQBN: `arduino:renesas_uno:unor4wifi`)
- Rain sensor module
  - digital output to `D2`
  - analog output to `A0`
- TTP223 capacitive touch sensor output to `D7`
- SHT3X temperature/humidity sensor on I2C (`0x44` default address)
- BMP580 pressure sensor on I2C (`0x46` default address)
- Station elevation: `129 m` above sea level (used to adjust pressure to sea-level equivalent)
- I2C LCD (`0x27`, configured as `20x4`)

## Behavior

- Reports rain status and intensity every 2 seconds.
- Shows status on LCD:
  - row 1: rain raw value and sea-level pressure (`P0`) in `mmHg`
  - row 2: raining/dry status
  - row 3: temperature and humidity from SHT3X
  - row 4: rain classification (`Heavy Rain`, `Light Rain`, `No Rain`)
- Touching TTP223 toggles LCD backlight ON/OFF.
- Writes structured logs to serial at `115200` baud.
- Publishes temperature, humidity, rain intensity, and sea-level-adjusted pressure in `mmHg` to ThingSpeak when a backend is configured.

## Dependencies

Installed by `make deps` / `make bootstrap`:

- `LiquidCrystal I2C`
- `Adafruit Unified Sensor`
- `Adafruit BMP5xx Library`
- `ThingSpeak`

Board core:

- `arduino:renesas_uno`

## Makefile Workflow

```bash
# Install Arduino CLI (if missing), board core, and libraries
make bootstrap

# Build sketch
make build SKETCH=weather-station

# Upload sketch
make deploy SKETCH=weather-station PORT=/dev/ttyACM0

# Upload and open serial monitor
make deploy-live SKETCH=weather-station PORT=/dev/ttyACM0

# Monitor only
make monitor PORT=/dev/ttyACM0 BAUD=115200
```

For CI (no monitor/upload requirement):

```bash
make ci SKETCH=weather-station
```

## ThingSpeak MATLAB Analysis

Use [weather-station/thingspeak_rain_analysis.m](weather-station/thingspeak_rain_analysis.m) inside ThingSpeak MATLAB Analysis.

Important ThingSpeak limitation:

- Do not write full historical matrices back with `thingSpeakWrite`.
- Write one aggregated row per run (for example mean/max/ratio).
- Free tier channels typically require at least 15 seconds between updates.

If you see `Requests are too frequent`, reduce execution frequency and make sure your script performs a single write per run.
