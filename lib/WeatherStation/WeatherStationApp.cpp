#include "WeatherStationApp.h"

#include <Logger.h>

WeatherStationApp::WeatherStationApp(
    uint8_t rainDigitalPin,
    uint8_t rainAnalogPin,
    uint8_t touchPin,
    uint8_t sht3xAddress,
    uint8_t bmp580Address,
    float stationElevationMeters,
    IBackend* backend,
    uint8_t lcdAddress,
    uint8_t lcdCols,
    uint8_t lcdRows,
    unsigned long reportIntervalMs,
    unsigned long backendIntervalMs
) : _lastReport(0),
    _lastBackendUpdate(0),
    _lastEnvironmentRead(0),
    _environmentRefreshIntervalMs(5000),
    _reportIntervalMs(reportIntervalMs),
    _backendIntervalMs(backendIntervalMs),
    _isDisplayOn(true),
    _hasEnvironmentReading(false),
    _lastTemperature(NAN),
    _lastHumidity(NAN),
    _lastPressure(NAN),
    _stationElevationMeters(stationElevationMeters),
    _lcd(lcdAddress, lcdCols, lcdRows),
    _rainSensor("RainDigital", rainDigitalPin, "RainAnalog", rainAnalogPin),
    _touchButton("DisplayTouch", touchPin, 120, false, false),
    _temperatureSensor("Temperature", sht3xAddress, SHT3X_TEMPERATURE_C),
    _humiditySensor("Humidity", sht3xAddress, SHT3X_HUMIDITY),
    _pressureSensor("Pressure", bmp580Address),
    _backend(backend) {}

void WeatherStationApp::begin() {
    Logger::begin(115200);
    Logger::info("weather-station", "Weather Station Initialization Started");

    _rainSensor.begin();
    _touchButton.begin();
    _temperatureSensor.begin();
    _humiditySensor.begin();
    if (_pressureSensor.begin()) {
        Logger::info("weather-station", "BMP580 pressure sensor initialized");
    } else {
        Logger::warn("weather-station", "BMP580 pressure sensor unavailable at startup");
    }

    if (_backend) {
        _backend->begin();
    }

    _lcd.init();
    _lcd.backlight();
    _lcd.setCursor(0, 0);
    _lcd.print("Weather Station");
    _lcd.setCursor(0, 1);
    _lcd.print("Initializing...");

    Logger::info("weather-station", "Sensors initialized");
    delay(1000);
    _lcd.clear();
}

void WeatherStationApp::tick() {
    handleTouchToggle();
    handlePeriodicReport();
    handleBackendUpdate();
}

void WeatherStationApp::handleBackendUpdate() {
    if (!_backend) {
        return;
    }

    if (millis() - _lastBackendUpdate >= _backendIntervalMs) {
        _lastBackendUpdate = millis();

        WeatherData data;
        RainReading rain = _rainSensor.read();
        data.temperature = NAN;
        data.humidity = NAN;
        data.pressure = NAN;
        data.rainIntensity = rain.intensity;
        data.isRaining = rain.isRaining;

        if (!readClimate(data.temperature, data.humidity, data.pressure)) {
            Logger::warn("weather-station", "SHT3X/BMP580 climate read failed, sending cached or empty climate fields");
        }

        if (_backend->sendData(data)) {
            Logger::info("weather-station", "Backend update successful");
        } else {
            Logger::error("weather-station", "Backend update failed");
        }
    }
}

bool WeatherStationApp::readClimate(float& temperature, float& humidity, float& pressure) {
    unsigned long now = millis();
    if (_hasEnvironmentReading && (now - _lastEnvironmentRead) < _environmentRefreshIntervalMs) {
        temperature = _lastTemperature;
        humidity = _lastHumidity;
        pressure = _lastPressure;
        return !isnan(temperature) && !isnan(humidity);
    }

    SensorReading temperatureReading = _temperatureSensor.read();
    SensorReading humidityReading = _humiditySensor.read();
    SensorReading pressureReading = _pressureSensor.read();

    if (!temperatureReading.valid || !humidityReading.valid) {
        if (_hasEnvironmentReading) {
            temperature = _lastTemperature;
            humidity = _lastHumidity;
            pressure = _lastPressure;
            Logger::warn("weather-station", "SHT3X read failed, reusing last valid climate reading");
            return true;
        }

        temperature = NAN;
        humidity = NAN;
        pressure = pressureReading.valid ? convertPressureToMmHg(adjustPressureToSeaLevel(pressureReading.value)) : NAN;
        return false;
    }

    _lastTemperature = temperatureReading.value;
    _lastHumidity = humidityReading.value;
    _lastPressure = pressureReading.valid ? convertPressureToMmHg(adjustPressureToSeaLevel(pressureReading.value)) : NAN;
    _lastEnvironmentRead = now;
    _hasEnvironmentReading = true;

    temperature = _lastTemperature;
    humidity = _lastHumidity;
    pressure = _lastPressure;

    String environmentLog = "SHT3X ok T=";
    environmentLog += String(temperature, 1);
    environmentLog += "C H=";
    environmentLog += String(humidity, 0);
    environmentLog += "%";
    if (!isnan(pressure)) {
        environmentLog += " P0=";
        environmentLog += String(pressure, 1);
        environmentLog += "mmHg";
    } else {
        Logger::warn("weather-station", "BMP580 pressure read failed for current sample");
    }
    Logger::info("weather-station", environmentLog.c_str());

    return true;
}

float WeatherStationApp::adjustPressureToSeaLevel(float stationPressureHpa) const {
    if (isnan(stationPressureHpa) || _stationElevationMeters <= 0.0f) {
        return stationPressureHpa;
    }

    return stationPressureHpa / pow(1.0f - (_stationElevationMeters / 44330.0f), 5.255f);
}

float WeatherStationApp::convertPressureToMmHg(float pressureHpa) const {
    if (isnan(pressureHpa)) {
        return pressureHpa;
    }

    return pressureHpa * 0.75006156f;
}

void WeatherStationApp::handleTouchToggle() {
    if (!_touchButton.wasPressed()) {
        return;
    }

    _isDisplayOn = !_isDisplayOn;
    if (_isDisplayOn) {
        _lcd.backlight();
        _lcd.display();
        Logger::info("weather-station", "Display: ON");
    } else {
        _lcd.noBacklight();
        _lcd.noDisplay();
        Logger::info("weather-station", "Display: OFF");
    }
}

void WeatherStationApp::handlePeriodicReport() {
    if (millis() - _lastReport < _reportIntervalMs) {
        return;
    }

    _lastReport = millis();
    RainReading rain = _rainSensor.read();
    float temperature = NAN;
    float humidity = NAN;
    float pressure = NAN;
    bool hasClimate = readClimate(temperature, humidity, pressure);

    logRainReport(rain);
    if (_isDisplayOn) {
        renderRainOnLcd(rain, pressure);
        _lcd.setCursor(0, 2);
        if (hasClimate) {
            _lcd.print("T: "); _lcd.print(temperature, 1); _lcd.print("C ");
            _lcd.print("H: "); _lcd.print(humidity, 0); _lcd.print("%");
        } else {
            _lcd.print("T/H unavailable     ");
        }
    }

    Logger::log(rain.level == RAIN_HEAVY ? WARN : INFO, "weather-station", RainSensor::levelToStatusText(rain.level));
}

void WeatherStationApp::logRainReport(const RainReading& rain) {
    String logLine = "Digital: ";
    logLine += (rain.isRaining ? "ACTIVE" : "IDLE");
    logLine += " | Raw Intensity: ";
    logLine += rain.intensity;
    Logger::info("weather-station", logLine.c_str());
}

void WeatherStationApp::renderRainOnLcd(const RainReading& rain, float pressure) {
    _lcd.setCursor(0, 0);
    _lcd.print("Rain: ");
    _lcd.print(rain.intensity);
    if (!isnan(pressure)) {
        _lcd.print(" P:");
        _lcd.print(pressure, 3);
    }
    _lcd.print("      "); // Clear remaining

    _lcd.setCursor(0, 1);
    if (rain.isRaining) {
        _lcd.print("Status: Raining  ");
    } else {
        _lcd.print("Status: Clear    ");
    }

    _lcd.setCursor(0, 3);
    _lcd.print(RainSensor::levelToText(rain.level));
    _lcd.print("          ");
}