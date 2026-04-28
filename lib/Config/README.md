# Weather Station Configuration

All shared, master, and slave configuration is now centralized in a single header file: **`lib/Config/WeatherStationConfig.h`**

This replaces scattered hardcoded values across multiple source files, making configuration changes simple and synchronized.

## Configuration Overview

### Shared Configuration (Both Master & Slave)

#### Radio Settings
- **Channel**: 100 (0-125, 2.4 GHz)
- **Address**: `0x57, 0x53, 0x54, 0x41, 0x54` (ASCII "WSTAT", 5 bytes)
- **Power Level**: -12 dBm (RF_SETUP = 0x02)
  - Alternative: 0 dBm with RF_SETUP = 0x06 (higher power consumption)
- **Payload Size**: 32 bytes (24-byte WeatherPacket + 8 bytes AES-128 padding)

#### Encryption
- **Key**: 16-byte AES-128 key defined in `ENCRYPTION_KEY[16]`
  - Current: `{0x00, 0x11, 0x22, 0x33, ..., 0xFF}` (placeholder)
  - ⚠️ **TODO**: Change to production key before deployment
  - Both devices must have identical keys for decryption to work

### Slave-Specific Configuration

#### Sensors
- **Temperature/Humidity (SHT3X)**: I2C address 0x44
- **Pressure (BMP580)**: I2C address 0x46
- **Rain Sensor**: Digital pin D2, Analog pin A0
- **Station Elevation**: 129.0 meters (affects barometric calculations)

#### Timing
- **Report Interval**: 2000 ms (how often slave transmits to master)
- **Climate Refresh**: 5000 ms (sensor read cache duration)
- **Debug Baud Rate**: 115200

#### Hardware Pins
- Radio CE (Chip Enable): Pin 9
- Radio CSN (Chip Select): Pin 10
- SPI: D11 (MOSI), D12 (MISO), D13 (SCK) — hardware fixed

### Master-Specific Configuration

#### Display
- **Brightness**: 255 (0-255, where 255 is maximum)
- **Offline Timeout**: 10000 ms (shows "OFFLINE" if no data after 10 seconds)
- **Power Toggle Pin**: D3 (capacitive touch sensor)

#### WiFi & Cloud
- Load credentials from `weather-station-master/secrets.h`
- Required: `ssid`, `password`, `channelID`, `writeAPIKey`

#### Hardware Pins
- Radio CE: Pin 9
- Radio CSN: Pin 10
- SPI: Hardware fixed (D11 MOSI, D12 MISO, D13 SCK)

## Making Configuration Changes

### To Update Radio Channel/Address/Power:
Edit `lib/Config/WeatherStationConfig.h` lines 13-28:
```cpp
const uint8_t RADIO_CHANNEL = 100;           // Change channel here
const uint8_t RADIO_ADDRESS[5] = {...};      // Change address here
const uint8_t RADIO_RF_SETUP = 0x02;         // 0x02 = -12dBm, 0x06 = 0dBm
```
Then recompile both master and slave.

### To Update Encryption Key:
Edit `lib/Config/WeatherStationConfig.h` line 32-35:
```cpp
const uint8_t ENCRYPTION_KEY[16] = {
    0x00, 0x11, ..., 0xFF  // Replace with your production key
};
```
⚠️ **Critical**: Both devices must be recompiled and uploaded with identical keys.

### To Update Sensor Timing (Slave):
Edit `lib/Config/WeatherStationConfig.h` lines 62-64:
```cpp
const unsigned long REPORT_INTERVAL_MS = 2000;          // transmission interval
const unsigned long CLIMATE_REFRESH_INTERVAL_MS = 5000; // sensor read cache
```

### To Update Display Settings (Master):
Edit `lib/Config/WeatherStationConfig.h` lines 86-88:
```cpp
const uint8_t DISPLAY_BRIGHTNESS = 255;
const unsigned long DISPLAY_OFFLINE_TIMEOUT_MS = 10000;
```

## Files That Reference This Configuration

- `lib/Radio/RadioEncryption.h/cpp` — Uses ENCRYPTION_KEY
- `lib/Radio/GT24DipReceiver.cpp` — Uses RADIO_CHANNEL, RADIO_ADDRESS, RADIO_PAYLOAD_LEN, RADIO_RF_SETUP
- `lib/Radio/ML01DP5Transmitter.cpp` — Uses RADIO_CHANNEL, RADIO_ADDRESS, RADIO_PAYLOAD_LEN, RADIO_RF_SETUP
- `weather-station-master/weather-station-master.ino` — Imports config automatically
- `weather-station-slave/weather-station-slave.ino` — Imports config automatically

## Compilation Profile Detection

The config file uses `#ifdef` guards to include device-specific settings:
- `#ifdef WEATHER_STATION_SLAVE_CONFIG` — Slave settings
- `#ifdef WEATHER_STATION_MASTER_CONFIG` — Master settings

Currently, these are optional (settings available regardless). Future enhancement: Add compiler flags to strictly validate device-specific config at compile time.

## Build & Deploy Workflow

After configuration changes:

```bash
# Recompile both sketches
make clean build-master build-slave

# Deploy master
make upload-master

# Deploy slave (reconnect Nano USB if previous upload failed)
make upload-slave
```

## Future Enhancements

1. **Runtime Configuration**: Load config from EEPROM to survive power cycles without recompile
2. **Configuration Validation**: Add compile-time checks to ensure master/slave configs are compatible
3. **Encryption Key Management**: Support OTA (over-the-air) key rotation with master controlling key updates
4. **Multi-Channel Support**: Store multiple radio addresses for mesh networking

