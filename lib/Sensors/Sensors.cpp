#include "Sensors.h"

namespace {

bool isSht3xCrcValid(const uint8_t* data, size_t len, uint8_t expectedCrc) {
    uint8_t crc = 0xFF;
    for (size_t index = 0; index < len; ++index) {
        crc ^= data[index];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (crc & 0x80) {
                crc = static_cast<uint8_t>((crc << 1) ^ 0x31);
            } else {
                crc <<= 1;
            }
        }
    }

    return crc == expectedCrc;
}

bool readSht3xMeasurement(uint8_t address, float& temperature, float& humidity) {
    Wire.beginTransmission(address);
    Wire.write(0x24);
    Wire.write(0x00);
    if (Wire.endTransmission() != 0) {
        return false;
    }

    delay(20);

    if (Wire.requestFrom(static_cast<int>(address), 6) != 6) {
        return false;
    }

    uint8_t buffer[6];
    for (uint8_t index = 0; index < 6; ++index) {
        buffer[index] = static_cast<uint8_t>(Wire.read());
    }

    if (!isSht3xCrcValid(buffer, 2, buffer[2]) || !isSht3xCrcValid(buffer + 3, 2, buffer[5])) {
        return false;
    }

    uint16_t rawTemperature = static_cast<uint16_t>((buffer[0] << 8) | buffer[1]);
    uint16_t rawHumidity = static_cast<uint16_t>((buffer[3] << 8) | buffer[4]);

    temperature = -45.0f + (175.0f * rawTemperature / 65535.0f);
    humidity = 100.0f * rawHumidity / 65535.0f;
    return true;
}

}

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

Sht3xSensor::Sht3xSensor(const char* sensorName, uint8_t address, Sht3xMetric metric)
    : _sensorName(sensorName), _address(address), _metric(metric) {}

bool Sht3xSensor::begin() {
    Wire.begin();
    return true;
}

SensorReading Sht3xSensor::read() {
    float value = NAN;
    const char* unit = "";
    float temperature = NAN;
    float humidity = NAN;

    bool measurementValid = readSht3xMeasurement(_address, temperature, humidity);

    if (measurementValid && _metric == SHT3X_TEMPERATURE_C) {
        value = temperature;
        unit = "C";
    } else if (measurementValid) {
        value = humidity;
        unit = "%";
    }

    SensorReading reading;
    reading.value = value;
    reading.valid = measurementValid && !isnan(value);
    reading.timestampMs = millis();
    reading.unit = unit;
    return reading;
}

const char* Sht3xSensor::name() const {
    return _sensorName;
}

Bmp580PressureSensor::Bmp580PressureSensor(const char* sensorName, uint8_t address)
    : _sensorName(sensorName), _address(address), _initialized(false) {}

bool Bmp580PressureSensor::begin() {
    Wire.begin();

    uint8_t addressesToTry[2] = {_address, _address};
    size_t addressCount = 1;

    if (_address == BMP5XX_DEFAULT_ADDRESS) {
        addressesToTry[1] = BMP5XX_ALTERNATIVE_ADDRESS;
        addressCount = 2;
    } else if (_address == BMP5XX_ALTERNATIVE_ADDRESS) {
        addressesToTry[1] = BMP5XX_DEFAULT_ADDRESS;
        addressCount = 2;
    }

    bool started = false;
    for (size_t index = 0; index < addressCount; ++index) {
        if (_bmp.begin(addressesToTry[index], &Wire)) {
            _address = addressesToTry[index];
            started = true;
            break;
        }
    }

    if (!started) {
        _initialized = false;
        return false;
    }

    _initialized = true;
    return true;
}

SensorReading Bmp580PressureSensor::read() {
    SensorReading reading;
    reading.value = NAN;
    reading.valid = false;
    reading.timestampMs = millis();
    reading.unit = "hPa";

    if (!_initialized && !begin()) {
        return reading;
    }

    unsigned long startedAt = millis();
    while (!_bmp.dataReady() && (millis() - startedAt) < 250) {
        delay(10);
    }

    if (!_bmp.performReading()) {
        return reading;
    }

    reading.value = _bmp.pressure;
    reading.valid = !isnan(reading.value);
    return reading;
}

const char* Bmp580PressureSensor::name() const {
    return _sensorName;
}
