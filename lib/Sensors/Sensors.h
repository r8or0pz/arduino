#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BMP5xx.h>

struct SensorReading {
    float value;
    bool valid;
    unsigned long timestampMs;
    const char* unit;
};

class ISensor {
public:
    virtual ~ISensor() {}
    virtual bool begin() = 0;
    virtual SensorReading read() = 0;
    virtual const char* name() const = 0;
};

class AnalogSensor : public ISensor {
public:
    AnalogSensor(const char* sensorName, uint8_t pin, const char* unit = "raw");
    bool begin() override;
    SensorReading read() override;
    const char* name() const override;

private:
    const char* _sensorName;
    uint8_t _pin;
    const char* _unit;
};

class DigitalSensor : public ISensor {
public:
    DigitalSensor(const char* sensorName, uint8_t pin, bool inputPullup = true, bool activeLow = true);
    bool begin() override;
    SensorReading read() override;
    const char* name() const override;

private:
    const char* _sensorName;
    uint8_t _pin;
    bool _inputPullup;
    bool _activeLow;
};

class TouchSensor : public ISensor {
public:
    TouchSensor(const char* sensorName, uint8_t pin, unsigned long debounceMs = 120, bool inputPullup = false, bool activeLow = false);
    bool begin() override;
    SensorReading read() override;
    const char* name() const override;
    bool wasPressed();

private:
    DigitalSensor _digital;
    unsigned long _debounceMs;
    unsigned long _lastPressAt;
    bool _lastActive;
};

enum RainLevel {
    RAIN_NONE,
    RAIN_LIGHT,
    RAIN_HEAVY
};

struct RainReading {
    SensorReading digital;
    SensorReading analog;
    bool valid;
    bool isRaining;
    int intensity;
    RainLevel level;
};

class RainSensor {
public:
    RainSensor(
        const char* digitalName,
        uint8_t digitalPin,
        const char* analogName,
        uint8_t analogPin,
        int heavyThreshold = 300,
        int lightThreshold = 700,
        bool inputPullup = true,
        bool activeLow = true
    );

    bool begin();
    RainReading read();
    static const char* levelToText(RainLevel level);
    static const char* levelToStatusText(RainLevel level);

private:
    DigitalSensor _digital;
    AnalogSensor _analog;
    int _heavyThreshold;
    int _lightThreshold;
};

enum Sht3xMetric {
    SHT3X_TEMPERATURE_C,
    SHT3X_HUMIDITY
};

class Sht3xSensor : public ISensor {
public:
    Sht3xSensor(const char* sensorName, uint8_t address, Sht3xMetric metric);
    bool begin() override;
    SensorReading read() override;
    const char* name() const override;

private:
    const char* _sensorName;
    uint8_t _address;
    Sht3xMetric _metric;
};

class Bmp580PressureSensor : public ISensor {
public:
    Bmp580PressureSensor(const char* sensorName, uint8_t address = BMP5XX_DEFAULT_ADDRESS);
    bool begin() override;
    SensorReading read() override;
    const char* name() const override;

private:
    const char* _sensorName;
    uint8_t _address;
    Adafruit_BMP5xx _bmp;
    bool _initialized;
};

#endif
