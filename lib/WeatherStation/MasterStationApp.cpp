#include "MasterStationApp.h"

#include <Logger.h>
#include <WeatherStationConfig.h>

namespace {
const unsigned long TREND_TRIANGLE_HOLD_MS = 20000;

int8_t trendDirection(float current, float previous, float epsilon, bool hasBaseline) {
    if (!hasBaseline) {
        return 0;
    }

    float delta = current - previous;
    if (delta > epsilon) {
        return 1;
    }
    if (delta < -epsilon) {
        return -1;
    }
    return 0;
}

float calculateDewPointC(float temperatureC, float humidityPercent) {
    if (isnan(temperatureC) || isnan(humidityPercent)) {
        return NAN;
    }

    if (humidityPercent < 1.0f) {
        humidityPercent = 1.0f;
    } else if (humidityPercent > 100.0f) {
        humidityPercent = 100.0f;
    }

    // Magnus formula constants for typical near-surface ambient weather conditions.
    const float a = 17.62f;
    const float b = 243.12f;
    float gamma = log(humidityPercent / 100.0f) + ((a * temperatureC) / (b + temperatureC));
    return (b * gamma) / (a - gamma);
}

void drawTrendTriangle(U8G2& display, int x, int baselineY, int8_t direction) {
    if (direction > 0) {
        // 4-row, 7px-wide symmetric up triangle.
        display.drawHLine(x,     baselineY - 5, 1);
        display.drawHLine(x - 1, baselineY - 4, 3);
        display.drawHLine(x - 2, baselineY - 3, 5);
        display.drawHLine(x - 3, baselineY - 2, 7);
    } else if (direction < 0) {
        // 4-row, 7px-wide symmetric down triangle.
        display.drawHLine(x - 3, baselineY - 5, 7);
        display.drawHLine(x - 2, baselineY - 4, 5);
        display.drawHLine(x - 1, baselineY - 3, 3);
        display.drawHLine(x,     baselineY - 2, 1);
    }
}

const char* shortRainLevelText(RainLevel level) {
    switch (level) {
    case RAIN_NONE:
        return "No rain";
    case RAIN_LIGHT:
        return "Light rain";
    case RAIN_HEAVY:
        return "Heavy rain";
    default:
        return "Unknown";
    }
}
}

MasterStationApp::MasterStationApp(
    uint8_t touchPin,
    IRadioReceiver* receiver,
    IBackend* backend,
    uint8_t dispClk,
    uint8_t dispData,
    uint8_t dispCS,
    uint8_t dispDC,
    uint8_t dispReset,
    unsigned long reportIntervalMs,
    unsigned long backendIntervalMs,
    unsigned long offlineTimeoutMs
) : _lastReport(0),
    _reportIntervalMs(reportIntervalMs),
    _lastPacketReceivedAt(0),
    _offlineTimeoutMs(offlineTimeoutMs),
    _isDisplayOn(true),
    _hasPacket(false),
    _hasSequence(false),
    _hasTempTrend(false),
    _hasHumidityTrend(false),
    _hasPressureTrend(false),
    _hasDewPointTrend(false),
    _lastTempTrendDirection(0),
    _lastHumidityTrendDirection(0),
    _lastPressureTrendDirection(0),
    _lastDewPointTrendDirection(0),
    _lastTempTrendAt(0),
    _lastHumidityTrendAt(0),
    _lastPressureTrendAt(0),
    _lastDewPointTrendAt(0),
    _lastSequence(0),
    _prevTemperature(0.0f),
    _prevHumidity(0.0f),
    _prevPressure(0.0f),
    _prevDewPoint(0.0f),
    _lastPacket(),
    _display(U8G2_R0, dispClk, dispData, dispCS, dispDC, dispReset),
    _touchButton("DisplayTouch", touchPin, 120, false, false),
    _receiver(receiver),
    _publisher(backend, backendIntervalMs) {}

void MasterStationApp::begin() {
    Logger::begin(115200);
    Logger::info("master-station", "Master station initialization started");

    if (_receiver) {
        _receiver->begin();
    }

    _touchButton.begin();
    _publisher.begin();

    // Initialize display with explicit settings
    _display.begin();
    delay(100); // Give display time to stabilize

    _display.setContrast(DISPLAY_BRIGHTNESS);
    delay(10);

    // Keep the panel in normal mode.
    _display.sendF("c", 0xA6);
    delay(10);

    // Ensure display is NOT in power save mode
    _display.setPowerSave(0);
    delay(10);

    // Set drawing parameters - draw white pixels (1 = on)
    _display.setDrawColor(1);
    _display.setFontMode(0); // Transparent mode for better visibility

    // Welcome screen (fills most of 128x64 area)
    _display.clearBuffer();
    _display.setDrawColor(1);
    _display.setFont(u8g2_font_9x15B_tr);
    _display.drawStr(10, 18, "Weather");
    _display.drawStr(14, 38, "Station");
    _display.setFont(u8g2_font_6x10_tf);
    _display.drawStr(0, 62, "Master v1.0");
    _display.sendBuffer();
    delay(100); // Ensure buffer is sent

    Logger::info("master-station", "Display initialized");
    delay(1500); // Welcome screen duration
}

void MasterStationApp::tick() {
    // Force display ON (temporary fix for black screen issue)
    if (!_isDisplayOn) {
        _isDisplayOn = true;
        _display.setPowerSave(0);
    }

    handleTouchToggle();
    handleRadioReceive();
    handlePeriodicReport();
    handleBackendUpdate();
}

void MasterStationApp::handleTouchToggle() {
    if (!_touchButton.wasPressed()) {
        return;
    }

    // Skip display toggle - keep display always ON
    // (Temporary: touch sensor seems to be causing issues)
    Logger::info("master-station", "Touch detected - display lock active");
}

void MasterStationApp::handleRadioReceive() {
    if (!_receiver) {
        return;
    }

    WeatherPacket packet;
    while (_receiver->receive(packet)) {
        if (_hasSequence) {
            uint8_t expected = static_cast<uint8_t>(_lastSequence + 1);
            if (packet.sequence != expected) {
                String msg = "Sequence gap expected=";
                msg += String(expected);
                msg += " got=";
                msg += String(packet.sequence);
                Logger::warn("master-station", msg.c_str());
            }
        }

        _lastPacket = packet;
        _lastPacketReceivedAt = millis();
        _hasPacket = true;
        _hasSequence = true;
        _lastSequence = packet.sequence;
    }
}

void MasterStationApp::handleBackendUpdate() {
    if (!_hasPacket) {
        return;
    }

    bool isOffline = (millis() - _lastPacketReceivedAt) > _offlineTimeoutMs;
    if (isOffline) {
        return;
    }

    _publisher.publishIfDue(_lastPacket);
}

void MasterStationApp::handlePeriodicReport() {
    if ((millis() - _lastReport) < _reportIntervalMs) {
        return;
    }

    _lastReport = millis();

    if (!_isDisplayOn) {
        return;
    }

    // Ensure display is awake and operational
    _display.setPowerSave(0);
    _display.setContrast(DISPLAY_BRIGHTNESS);

    if (!_hasPacket || (millis() - _lastPacketReceivedAt) > _offlineTimeoutMs) {
        _display.clearBuffer();
        _display.setDrawColor(1);
        _display.setFont(u8g2_font_9x15B_tr);
        _display.drawStr(12, 24, "OFFLINE");
        _display.setFont(u8g2_font_6x10_tf);
        _display.drawStr(0, 44, "Waiting for slave");
        _display.drawStr(0, 62, "check radio link");
        _display.sendBuffer();
        return;
    }

    renderPacketOnDisplay(_lastPacket);
}

void MasterStationApp::renderPacketOnDisplay(const WeatherPacket& packet) {
    char buf[28];
    int8_t tempTrend = 0;
    int8_t humidityTrend = 0;
    int8_t pressureTrend = 0;
    int8_t dewPointTrend = 0;
    unsigned long now = millis();
    bool dewPointValid = false;
    float dewPointC = NAN;

    _display.clearBuffer();
    _display.setDrawColor(1);
    _display.setFont(u8g2_font_6x10_tf);

    // Emphasize climate values with larger font.
    if (packet.climateValid && !isnan(packet.temperature) && !isnan(packet.humidity)) {
        tempTrend = trendDirection(packet.temperature, _prevTemperature, 0.05f, _hasTempTrend);
        humidityTrend = trendDirection(packet.humidity, _prevHumidity, 0.2f, _hasHumidityTrend);

        if (tempTrend != 0) {
            _lastTempTrendDirection = tempTrend;
            _lastTempTrendAt = now;
        } else if ((now - _lastTempTrendAt) <= TREND_TRIANGLE_HOLD_MS) {
            tempTrend = _lastTempTrendDirection;
        }

        if (humidityTrend != 0) {
            _lastHumidityTrendDirection = humidityTrend;
            _lastHumidityTrendAt = now;
        } else if ((now - _lastHumidityTrendAt) <= TREND_TRIANGLE_HOLD_MS) {
            humidityTrend = _lastHumidityTrendDirection;
        }

        dewPointC = calculateDewPointC(packet.temperature, packet.humidity);
        dewPointValid = !isnan(dewPointC);

        snprintf(buf, sizeof(buf), "T: %.1fC ", (double)packet.temperature);
        _display.drawStr(0, 10, buf);
        drawTrendTriangle(_display, 0 + _display.getStrWidth(buf) + 2, 10, tempTrend);

        snprintf(buf, sizeof(buf), "H: %.0f%% ", (double)packet.humidity);
        _display.drawStr(64, 10, buf);
        drawTrendTriangle(_display, 64 + _display.getStrWidth(buf) + 2, 10, humidityTrend);
    } else {
        snprintf(buf, sizeof(buf), "T: --");
        _display.drawStr(0, 10, buf);
        snprintf(buf, sizeof(buf), "H: --");
        _display.drawStr(64, 10, buf);
    }

    if (packet.pressureValid && !isnan(packet.pressure)) {
        pressureTrend = trendDirection(packet.pressure, _prevPressure, 0.1f, _hasPressureTrend);

        if (pressureTrend != 0) {
            _lastPressureTrendDirection = pressureTrend;
            _lastPressureTrendAt = now;
        } else if ((now - _lastPressureTrendAt) <= TREND_TRIANGLE_HOLD_MS) {
            pressureTrend = _lastPressureTrendDirection;
        }

        snprintf(buf, sizeof(buf), "P: %.1f mmHg ", (double)packet.pressure);
    } else {
        snprintf(buf, sizeof(buf), "P: --");
    }
    _display.drawStr(0, 22, buf);
    int pressureTrendX = _display.getStrWidth(buf) + 6;
    if (pressureTrendX > 124) {
        pressureTrendX = 124;
    }
    drawTrendTriangle(_display, pressureTrendX, 22, pressureTrend);

    if (dewPointValid) {
        dewPointTrend = trendDirection(dewPointC, _prevDewPoint, 0.05f, _hasDewPointTrend);

        if (dewPointTrend != 0) {
            _lastDewPointTrendDirection = dewPointTrend;
            _lastDewPointTrendAt = now;
        } else if ((now - _lastDewPointTrendAt) <= TREND_TRIANGLE_HOLD_MS) {
            dewPointTrend = _lastDewPointTrendDirection;
        }

        snprintf(buf, sizeof(buf), "DP: %.1fC", (double)dewPointC);
    } else {
        snprintf(buf, sizeof(buf), "DP: --");
    }
    _display.drawStr(0, 32, buf);
    int dewPointTrendX = _display.getStrWidth(buf) + 6;
    if (dewPointTrendX > 124) {
        dewPointTrendX = 124;
    }
    drawTrendTriangle(_display, dewPointTrendX, 32, dewPointTrend);

    snprintf(buf, sizeof(buf), "%s", shortRainLevelText(static_cast<RainLevel>(packet.rainLevel)));
    _display.drawStr(0, 42, buf);

    if (packet.climateValid && !isnan(packet.temperature) && !isnan(packet.humidity)) {
        _prevTemperature = packet.temperature;
        _prevHumidity = packet.humidity;
        _hasTempTrend = true;
        _hasHumidityTrend = true;
    }

    if (packet.pressureValid && !isnan(packet.pressure)) {
        _prevPressure = packet.pressure;
        _hasPressureTrend = true;
    }

    if (dewPointValid) {
        _prevDewPoint = dewPointC;
        _hasDewPointTrend = true;
    }

    _display.sendBuffer();
}
