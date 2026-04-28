#ifndef WEATHER_STATION_CONFIG_H
#define WEATHER_STATION_CONFIG_H

#include <stdint.h>
#include "RadioSecrets.h"

// ============================================================================
// SHARED CONFIGURATION (Both Master & Slave)
// ============================================================================

// --- Radio Configuration ---
// nRF24L01+ channel (0-125, 2.4 GHz)
const uint8_t RADIO_CHANNEL = 100;

// nRF24L01+ address (5 bytes, must be identical on both devices)
const uint8_t RADIO_ADDRESS[5] = {0x57, 0x53, 0x54, 0x41, 0x54}; // "WSTAT"

// RF_SETUP register value:
//   0x02 = 1 Mbps, -12 dBm (recommended for power efficiency)
//   0x06 = 1 Mbps,  0 dBm  (maximum power, higher current draw)
const uint8_t RADIO_RF_SETUP = 0x02;

// Payload size: 32 bytes (24-byte WeatherPacket + 8 bytes padding for AES-128)
const uint8_t RADIO_PAYLOAD_LEN = 32;

// --- Encryption Configuration ---
// AES-128 key is provided by local-only RadioSecrets.h.

// ============================================================================
// SLAVE-SPECIFIC CONFIGURATION
// ============================================================================

#ifdef WEATHER_STATION_SLAVE_CONFIG

// --- Sensor Configuration ---
// Sensor I2C addresses
const uint8_t SHT3X_ADDRESS = 0x44;  // Temperature/Humidity sensor
const uint8_t BMP580_ADDRESS = 0x46; // Pressure sensor

// Pin assignments
const int RAIN_DIGITAL_PIN = 2;      // Digital rain sensor pin
const int RAIN_ANALOG_PIN = A0;      // Analog rain sensor pin

// Station elevation in meters (for barometric altitude calculation)
const float STATION_ELEVATION_METERS = 129.0f;

// --- Timing Configuration (Slave) ---
// How often to transmit sensor data to master (milliseconds)
const unsigned long REPORT_INTERVAL_MS = 2000; // 2 seconds

// How often to refresh sensor readings internally (milliseconds)
// Set higher than REPORT_INTERVAL_MS to cache reads and reduce I2C traffic
const unsigned long CLIMATE_REFRESH_INTERVAL_MS = 5000; // 5 seconds

// Serial debug baud rate
const unsigned long DEBUG_BAUD_RATE = 115200;

// Radio pins (SPI: MOSI=D11, MISO=D12, SCK=D13 are hardware fixed)
const uint8_t RADIO_CE_PIN = 9;   // Chip Enable
const uint8_t RADIO_CSN_PIN = 10; // Chip Select Not

#endif // WEATHER_STATION_SLAVE_CONFIG

// ============================================================================
// MASTER-SPECIFIC CONFIGURATION
// ============================================================================

#ifdef WEATHER_STATION_MASTER_CONFIG

// --- Display Configuration ---
// ST7565 contrast. Higher values darken the full panel background.
const uint8_t DISPLAY_BRIGHTNESS = 145;

// Display update timeout if no radio data received (milliseconds)
// After this time, display shows "OFFLINE" instead of stale data
const unsigned long DISPLAY_OFFLINE_TIMEOUT_MS = 10000; // 10 seconds

// Display power toggle pin (capacitive touch sensor)
const int DISPLAY_POWER_PIN = 3;

// --- WiFi Configuration ---
// Load from secrets.h (see weather-station-master/secrets.h)
// Requires: ssid, password, channelID, writeAPIKey

// --- Timing Configuration (Master) ---
// Serial debug baud rate
const unsigned long DEBUG_BAUD_RATE = 115200;

// Radio pins (SPI: MOSI=D11, MISO=D12, SCK=D13 are hardware fixed)
const uint8_t RADIO_CE_PIN = 9;   // Chip Enable
const uint8_t RADIO_CSN_PIN = 10; // Chip Select Not

#endif // WEATHER_STATION_MASTER_CONFIG

#endif // WEATHER_STATION_CONFIG_H
