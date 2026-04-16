#include "WeatherStationApp.h"

#include <Logger.h>

WeatherStationApp::WeatherStationApp(
    uint8_t rainDigitalPin,
    uint8_t rainAnalogPin,
    uint8_t touchPin,
    uint8_t dhtPin,
    IBackend* backend,
    uint8_t lcdAddress,
    uint8_t lcdCols,
    uint8_t lcdRows,
    unsigned long reportIntervalMs,
    unsigned long backendIntervalMs
) : _lastReport(0),
    _lastBackendUpdate(0),
    _reportIntervalMs(reportIntervalMs),
    _backendIntervalMs(backendIntervalMs),
    _isDisplayOn(true),
    _lcd(lcdAddress, lcdCols, lcdRows),
    _rainSensor("RainDigital", rainDigitalPin, "RainAnalog", rainAnalogPin),
    _touchButton("DisplayTouch", touchPin, 120, false, false),
    _dht(dhtPin),
    _backend(backend) {}

void WeatherStationApp::begin() {
    Logger::begin(115200);
    Logger::info("weather-station", "Weather Station Initialization Started");
    
    _rainSensor.begin();
    _touchButton.begin();
    _dht.begin();

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
    if (!_backend) return;

    if (millis() - _lastBackendUpdate >= _backendIntervalMs) {
        _lastBackendUpdate = millis();

        WeatherData data;
        RainReading rain = _rainSensor.read();
        data.rainIntensity = rain.intensity;
        data.temp = _dht.readTemperature();
        data.humidity = _dht.readHumidity();

        if (isnan(data.temp) || isnan(data.humidity)) {
            Logger::error("weather-station", "DHT sensor read failed for backend update");
            return;
        }

        if (_backend->sendData(data)) {
            Logger::info("weather-station", "Backend update successful");
        } else {
            Logger::error("weather-station", "Backend update failed");
        }
    }
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
    float temp = _dht.readTemperature();
    float humidity = _dht.readHumidity();

    logRainReport(rain);
    if (_isDisplayOn) {
        renderRainOnLcd(rain);
        _lcd.setCursor(0, 2);
        _lcd.print("T: "); _lcd.print(temp, 1); _lcd.print("C ");
        _lcd.print("H: "); _lcd.print(humidity, 0); _lcd.print("%");
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

void WeatherStationApp::renderRainOnLcd(const RainReading& rain) {
    _lcd.setCursor(0, 0);
    _lcd.print("Rain: ");
    _lcd.print(rain.intensity);
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