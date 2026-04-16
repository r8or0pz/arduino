#include "WeatherStationApp.h"

#include <Logger.h>

WeatherStationApp::WeatherStationApp(
    uint8_t rainDigitalPin,
    uint8_t rainAnalogPin,
    uint8_t touchPin,
    uint8_t lcdAddress,
    uint8_t lcdCols,
    uint8_t lcdRows,
    unsigned long reportIntervalMs
) : _lastReport(0),
    _reportIntervalMs(reportIntervalMs),
    _isDisplayOn(true),
    _lcd(lcdAddress, lcdCols, lcdRows),
    _rainSensor("RainDigital", rainDigitalPin, "RainAnalog", rainAnalogPin),
    _touchButton("DisplayTouch", touchPin, 120, false, false) {}

void WeatherStationApp::begin() {
    Logger::begin(115200);
    _rainSensor.begin();
    _touchButton.begin();

    _lcd.init();
    _lcd.backlight();
    _lcd.setCursor(0, 0);
    _lcd.print("Weather Station");
    _lcd.setCursor(0, 1);
    _lcd.print("Initializing...");

    Logger::info("weather-station", "Rain sensors initialized");
}

void WeatherStationApp::tick() {
    handleTouchToggle();
    handlePeriodicReport();
}

void WeatherStationApp::handleTouchToggle() {
    if (!_touchButton.wasPressed()) {
        return;
    }

    _isDisplayOn = !_isDisplayOn;
    if (_isDisplayOn) {
        _lcd.backlight();
        Logger::info("weather-station", "Display: ON");
    } else {
        _lcd.noBacklight();
        Logger::info("weather-station", "Display: OFF");
    }
}

void WeatherStationApp::handlePeriodicReport() {
    if (millis() - _lastReport < _reportIntervalMs) {
        return;
    }

    _lastReport = millis();
    RainReading rain = _rainSensor.read();

    logRainReport(rain);
    renderRainOnLcd(rain);
    Logger::log(rain.level == RAIN_HEAVY ? WARN : INFO, "weather-station", RainSensor::levelToStatusText(rain.level));
}

void WeatherStationApp::logRainReport(const RainReading& rain) {
    String logLine = "Digital: ";
    logLine += (rain.isRaining ? "ACTIVE" : "IDLE");
    logLine += " | Raw Intensity: ";
    logLine += rain.intensity;
    Logger::info("weather-station", logLine.c_str());
}

void WeatherStationApp::renderRainOnLcd(const RainReading& rain) {
    _lcd.clear();
    _lcd.setCursor(0, 0);
    _lcd.print("RAIN SENSOR");

    _lcd.setCursor(0, 1);
    _lcd.print("Status: ");
    _lcd.print(rain.isRaining ? "Raining" : "Dry");

    _lcd.setCursor(0, 2);
    _lcd.print("Intensity: ");
    _lcd.print(rain.intensity);

    _lcd.setCursor(0, 3);
    _lcd.print(RainSensor::levelToText(rain.level));
}