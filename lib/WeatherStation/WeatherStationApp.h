#ifndef WEATHER_STATION_APP_H
#define WEATHER_STATION_APP_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Sensors.h>

class WeatherStationApp {
public:
    WeatherStationApp(
        uint8_t rainDigitalPin,
        uint8_t rainAnalogPin,
        uint8_t touchPin,
        uint8_t lcdAddress = 0x27,
        uint8_t lcdCols = 20,
        uint8_t lcdRows = 4,
        unsigned long reportIntervalMs = 1000
    );

    void begin();
    void tick();

private:
    void handleTouchToggle();
    void handlePeriodicReport();
    void logRainReport(const RainReading& rain);
    void renderRainOnLcd(const RainReading& rain);

    unsigned long _lastReport;
    unsigned long _reportIntervalMs;
    bool _isDisplayOn;

    LiquidCrystal_I2C _lcd;
    RainSensor _rainSensor;
    TouchSensor _touchButton;
};

#endif