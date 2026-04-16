#include <Wire.h>
#include <WeatherStationApp.h>
#include <ThingSpeakBackend.h>

// Pin Definitions
const int rainDigitalPin = 2;
const int rainAnalogPin = A0;
const int touchPin = 7;
const int dhtPin = 4;

// WiFi & ThingSpeak Credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
unsigned long channelID = 1234567; // Your Channel ID
const char* writeKey = "YOUR_WRITE_API_KEY";

// Create Backend
ThingSpeakBackend tsBackend(ssid, password, channelID, writeKey);

// Initialize App with Backend
WeatherStationApp app(
    rainDigitalPin, 
    rainAnalogPin, 
    touchPin, 
    dhtPin, 
    &tsBackend
);

void setup() {
  app.begin();
}

void loop() {
  app.tick();
}
