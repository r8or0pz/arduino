#include "Sensors.h"

AnalogSensor::AnalogSensor(const char* sensorName, uint8_t pin, const char* unit)
    : _sensorName(sensorName), _pin(pin), _unit(unit) {}

bool AnalogSensor::begin() {
    pinMode(_pin, INPUT);
    return true;
}

SensorReading AnalogSensor::read() {
    SensorReading reading;
    reading.value = analogRead(_pin);
    reading.valid = true;
    reading.timestampMs = millis();
    reading.unit = _unit;
    return reading;
}

const char* AnalogSensor::name() const {
    return _sensorName;
}

DigitalSensor::DigitalSensor(const char* sensorName, uint8_t pin, bool inputPullup, bool activeLow)
    : _sensorName(sensorName), _pin(pin), _inputPullup(inputPullup), _activeLow(activeLow) {}

bool DigitalSensor::begin() {
    pinMode(_pin, _inputPullup ? INPUT_PULLUP : INPUT);
    return true;
}

SensorReading DigitalSensor::read() {
    int raw = digitalRead(_pin);
    bool active = _activeLow ? (raw == LOW) : (raw == HIGH);

    SensorReading reading;
    reading.value = active ? 1.0f : 0.0f;
    reading.valid = true;
    reading.timestampMs = millis();
    reading.unit = "bool";
    return reading;
}

const char* DigitalSensor::name() const {
    return _sensorName;
}

TouchSensor::TouchSensor(const char* sensorName, uint8_t pin, unsigned long debounceMs, bool inputPullup, bool activeLow)
    : _digital(sensorName, pin, inputPullup, activeLow), _debounceMs(debounceMs), _lastPressAt(0), _lastActive(false) {}

bool TouchSensor::begin() {
    return _digital.begin();
}

SensorReading TouchSensor::read() {
    return _digital.read();
}

const char* TouchSensor::name() const {
    return _digital.name();
}

bool TouchSensor::wasPressed() {
    SensorReading reading = _digital.read();
    bool active = reading.valid && (reading.value > 0.5f);
    unsigned long now = millis();
    bool pressed = active && !_lastActive && (now - _lastPressAt >= _debounceMs);

    if (pressed) {
        _lastPressAt = now;
    }

    _lastActive = active;
    return pressed;
}

RainSensor::RainSensor(
    const char* digitalName,
    uint8_t digitalPin,
    const char* analogName,
    uint8_t analogPin,
    int heavyThreshold,
    int lightThreshold,
    bool inputPullup,
    bool activeLow
) : _digital(digitalName, digitalPin, inputPullup, activeLow),
    _analog(analogName, analogPin, "raw"),
    _heavyThreshold(heavyThreshold),
    _lightThreshold(lightThreshold) {}

bool RainSensor::begin() {
    return _digital.begin() && _analog.begin();
}

RainReading RainSensor::read() {
    RainReading reading;
    reading.digital = _digital.read();
    reading.analog = _analog.read();

    reading.isRaining = reading.digital.valid && (reading.digital.value > 0.5f);
    reading.intensity = reading.analog.valid ? static_cast<int>(reading.analog.value) : 0;
    reading.valid = reading.digital.valid && reading.analog.valid;

    if (reading.intensity < _heavyThreshold) {
        reading.level = RAIN_HEAVY;
    } else if (reading.intensity < _lightThreshold) {
        reading.level = RAIN_LIGHT;
    } else {
        reading.level = RAIN_NONE;
    }

    return reading;
}

const char* RainSensor::levelToText(RainLevel level) {
    switch (level) {
        case RAIN_HEAVY:
            return "Heavy Rain";
        case RAIN_LIGHT:
            return "Light Rain";
        case RAIN_NONE:
        default:
            return "No Rain";
    }
}

const char* RainSensor::levelToStatusText(RainLevel level) {
    switch (level) {
        case RAIN_HEAVY:
            return "Status: Heavy Rain";
        case RAIN_LIGHT:
            return "Status: Light Rain";
        case RAIN_NONE:
        default:
            return "Status: Dry";
    }
}

DhtSensor::DhtSensor(const char* sensorName, uint8_t pin, uint8_t dhtType, DhtMetric metric)
    : _sensorName(sensorName), _dht(pin, dhtType), _metric(metric) {}

bool DhtSensor::begin() {
    _dht.begin();
    return true;
}

SensorReading DhtSensor::read() {
    float value = NAN;
    const char* unit = "";

    if (_metric == DHT_TEMPERATURE_C) {
        value = _dht.readTemperature();
        unit = "C";
    } else {
        value = _dht.readHumidity();
        unit = "%";
    }

    SensorReading reading;
    reading.value = value;
    reading.valid = !isnan(value);
    reading.timestampMs = millis();
    reading.unit = unit;
    return reading;
}

const char* DhtSensor::name() const {
    return _sensorName;
}
