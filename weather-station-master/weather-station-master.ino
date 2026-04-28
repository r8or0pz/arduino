#include <MasterStationApp.h>
#include <WiFiConnection.h>
#include <ThingSpeakBackend.h>
#include <GT24DipReceiver.h>
#define WEATHER_STATION_MASTER_CONFIG
#include <WeatherStationConfig.h>
#include "secrets.h"

// Configuration is now centralized in lib/Config/WeatherStationConfig.h

WiFiConnection wifiConnection(ssid, password);
ThingSpeakBackend tsBackend(wifiConnection, channelID, writeAPIKey);
GT24DipReceiver radioReceiver; // CE=5, CSN=11, SCK=13, MOSI=12, MISO=4 (defaults)

MasterStationApp app(
    DISPLAY_POWER_PIN,
    &radioReceiver,
    &tsBackend
);

void setup() {
  app.begin();
}

void loop() {
  app.tick();
}
