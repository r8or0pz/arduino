#include <Wire.h>
#include <WeatherStationApp.h>

const int rainDigitalPin = 2;
const int rainAnalogPin = A0;
const int touchPin = 7;

WeatherStationApp app(rainDigitalPin, rainAnalogPin, touchPin);

void setup() {
  app.begin();
}

void loop() {
  app.tick();
}
