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
        uint8_t sht3xAddress = 0x44,
        uint8_t bmp580Address = BMP5XX_DEFAULT_ADDRESS,
        float stationElevationMeters = 0.0f,
        IBackend* backend = nullptr,
        uint8_t lcdAddress = 0x27,
        uint8_t lcdCols = 20,
        uint8_t lcdRows = 4,
        unsigned long reportIntervalMs = 2000,
        unsigned long backendIntervalMs = 15000
    );

    void begin();
    void tick();

private:
    void handleTouchToggle();
    void handlePeriodicReport();
    void handleBackendUpdate();
    bool readClimate(float& temperature, float& humidity, float& pressure);
    float adjustPressureToSeaLevel(float stationPressureHpa) const;
    float convertPressureToMmHg(float pressureHpa) const;
    void logRainReport(const RainReading& rain);
    void renderRainOnLcd(const RainReading& rain, float pressure);

    unsigned long _lastReport;
    unsigned long _lastBackendUpdate;
    unsigned long _lastEnvironmentRead;
    unsigned long _environmentRefreshIntervalMs;
    unsigned long _reportIntervalMs;
    unsigned long _backendIntervalMs;
    bool _isDisplayOn;
    bool _hasEnvironmentReading;
    float _lastTemperature;
    float _lastHumidity;
    float _lastPressure;
    float _stationElevationMeters;

    LiquidCrystal_I2C _lcd;
    RainSensor _rainSensor;
    TouchSensor _touchButton;
    Sht3xSensor _temperatureSensor;
    Sht3xSensor _humiditySensor;
    Bmp580PressureSensor _pressureSensor;
    IBackend* _backend;
};

#endif