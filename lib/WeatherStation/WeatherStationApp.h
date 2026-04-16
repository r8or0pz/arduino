#ifndef WEATHER_STATION_APP_H
#define WEATHER_STATION_APP_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Sensors.h>
#include <IBackend.h>

class WeatherStationApp {
public:
    WeatherStationApp(
        uint8_t rainDigitalPin,
        uint8_t rainAnalogPin,
        uint8_t touchPin,
        uint8_t dhtPin,
        IBackend* backend = nullptr,
        uint8_t lcdAddress = 0x27,
        uint8_t lcdCols = 20,
        uint8_t lcdRows = 4,
        unsigned long reportIntervalMs = 1000,
        unsigned long backendIntervalMs = 20000
    );

    void begin();
    void tick();

private:
    void handleTouchToggle();
    void handlePeriodicReport();
    void handleBackendUpdate();
    void logRainReport(const RainReading& rain);
    void renderRainOnLcd(const RainReading& rain);

    unsigned long _lastReport;
    unsigned long _lastBackendUpdate;
    unsigned long _reportIntervalMs;
    unsigned long _backendIntervalMs;
    bool _isDisplayOn;

    LiquidCrystal_I2C _lcd;
    RainSensor _rainSensor;
    TouchSensor _touchButton;
    DhtSensor _dht;
    IBackend* _backend;
};

#endif