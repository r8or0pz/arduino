#include <Wire.h>
#include <WeatherStationApp.h>
#include <WiFiConnection.h>
#include <ThingSpeakBackend.h>
#include "secrets.h"

// Pin Definitions
const int rainDigitalPin = 2;
const int rainAnalogPin = A0;
const int touchPin = 7;
const uint8_t sht3xAddress = 0x44;
const uint8_t bmp580Address = BMP5XX_DEFAULT_ADDRESS;
const float stationElevationMeters = 129.0f;

WiFiConnection wifiConnection(ssid, password);
ThingSpeakBackend tsBackend(wifiConnection, channelID, writeAPIKey);

// Initialize App with Backend
WeatherStationApp app(
    rainDigitalPin,
    rainAnalogPin,
    touchPin,
    sht3xAddress,
    bmp580Address,
    stationElevationMeters,
    &tsBackend
);

void setup() {
  app.begin();
}

void loop() {
  app.tick();
}
