# Arduino Weather Station

This repository contains an Arduino weather-station sketch and shared local libraries.

## Project Layout

- `weather-station/weather-station.ino` - main sketch (rain sensor + LCD + touch backlight control)
- `lib/Sensors/` - shared sensor abstractions (`AnalogSensor`, `DigitalSensor`, `TouchSensor`, `RainSensor`)
- `lib/Logger/` - shared serial logger abstraction with log levels
- `Makefile` - bootstrap, build, upload, monitor, and CI workflow

## Hardware

- Arduino UNO R4 WiFi (FQBN: `arduino:renesas_uno:unor4wifi`)
- Rain sensor module
  - digital output to `D2`
  - analog output to `A0`
- TTP223 capacitive touch sensor output to `D7`
- I2C LCD (`0x27`, configured as `20x4`)

## Behavior

- Reports rain status and intensity every 1 second.
- Shows status on LCD:
  - row 1: title
  - row 2: raining/dry status
  - row 3: raw intensity
  - row 4: rain classification (`Heavy Rain`, `Light Rain`, `No Rain`)
- Touching TTP223 toggles LCD backlight ON/OFF.
- Writes structured logs to serial at `115200` baud.

## Dependencies

Installed by `make deps` / `make bootstrap`:

- `LiquidCrystal I2C`
- `DHT sensor library`
- `Adafruit Unified Sensor`

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
