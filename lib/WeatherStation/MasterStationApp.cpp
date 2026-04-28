#include "MasterStationApp.h"

#include <Logger.h>
#include <WeatherStationConfig.h>

namespace {
const unsigned long TREND_TRIANGLE_HOLD_MS = 20000;
const float AUTOSCALE_PADDING_RATIO = 0.12f;
const float AUTOSCALE_MIN_PADDING = 0.5f;
const float AUTOSCALE_MAX_STEP_RATIO = 0.10f;
const float AUTOSCALE_MIN_STEP = 0.2f;

bool hasBacklightControl() {
    return DISPLAY_BACKLIGHT_PIN >= 0;
}

void driveBacklightPin(bool enabled) {
    if (!hasBacklightControl()) {
        return;
    }

    bool highLevel = (enabled == DISPLAY_BACKLIGHT_ACTIVE_HIGH);
    digitalWrite(DISPLAY_BACKLIGHT_PIN, highLevel ? HIGH : LOW);
}

void setBacklightEnabled(bool enabled) {
    driveBacklightPin(enabled);
}

void fadeBacklightOff() {
    if (!hasBacklightControl()) {
        return;
    }

    // Pin-agnostic fade: software PWM ramp, useful when backlight is on non-PWM pins.
    const uint8_t steps = 18;
    const uint8_t pulsesPerStep = 6;
    const uint16_t pwmPeriodUs = 1000;

    for (int step = steps; step >= 1; --step) {
        uint16_t onUs = static_cast<uint16_t>((static_cast<uint32_t>(pwmPeriodUs) * step) / steps);
        uint16_t offUs = static_cast<uint16_t>(pwmPeriodUs - onUs);

        for (uint8_t pulse = 0; pulse < pulsesPerStep; ++pulse) {
            if (onUs > 0) {
                driveBacklightPin(true);
                delayMicroseconds(onUs);
            }
            if (offUs > 0) {
                driveBacklightPin(false);
                delayMicroseconds(offUs);
            }
        }
    }

    driveBacklightPin(false);
}

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

int valueToGraphY(float value, float minValue, float maxValue, int topY, int bottomY) {
    if (maxValue <= minValue) {
        return bottomY;
    }

    float normalized = (value - minValue) / (maxValue - minValue);
    if (normalized < 0.0f) {
        normalized = 0.0f;
    } else if (normalized > 1.0f) {
        normalized = 1.0f;
    }

    float span = static_cast<float>(bottomY - topY);
    return bottomY - static_cast<int>(normalized * span + 0.5f);
}

bool computeHistoryRange(
    const float* history,
    const bool* valid,
    uint16_t head,
    int historySize,
    float& outMin,
    float& outMax
) {
    bool hasData = false;
    float minValue = 0.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < historySize; ++i) {
        int index = (static_cast<int>(head) + 1 + i) % historySize;
        if (!valid[index]) {
            continue;
        }

        float value = history[index];
        if (!hasData) {
            minValue = value;
            maxValue = value;
            hasData = true;
        } else {
            if (value < minValue) {
                minValue = value;
            }
            if (value > maxValue) {
                maxValue = value;
            }
        }
    }

    if (!hasData) {
        return false;
    }

    outMin = minValue;
    outMax = maxValue;
    return true;
}

float clampTowards(float current, float target, float maxStep) {
    if (target > current + maxStep) {
        return current + maxStep;
    }
    if (target < current - maxStep) {
        return current - maxStep;
    }
    return target;
}

void updateClampedAutoScale(
    const float* history,
    const bool* valid,
    uint16_t head,
    int historySize,
    float fallbackMin,
    float fallbackMax,
    bool& hasScale,
    float& scaleMin,
    float& scaleMax
) {
    float targetMin = fallbackMin;
    float targetMax = fallbackMax;
    if (computeHistoryRange(history, valid, head, historySize, targetMin, targetMax)) {
        float range = targetMax - targetMin;
        float padding = range * AUTOSCALE_PADDING_RATIO;
        if (padding < AUTOSCALE_MIN_PADDING) {
            padding = AUTOSCALE_MIN_PADDING;
        }
        targetMin -= padding;
        targetMax += padding;
    }

    if (!hasScale) {
        scaleMin = targetMin;
        scaleMax = targetMax;
        hasScale = true;
        return;
    }

    float currentRange = scaleMax - scaleMin;
    if (currentRange < 1.0f) {
        currentRange = 1.0f;
    }
    float maxStep = currentRange * AUTOSCALE_MAX_STEP_RATIO;
    if (maxStep < AUTOSCALE_MIN_STEP) {
        maxStep = AUTOSCALE_MIN_STEP;
    }

    scaleMin = clampTowards(scaleMin, targetMin, maxStep);
    scaleMax = clampTowards(scaleMax, targetMax, maxStep);

    if (scaleMax <= scaleMin) {
        scaleMax = scaleMin + 1.0f;
    }
}

bool sampleCompressedHistoryValue(
    const float* history,
    const bool* valid,
    uint16_t head,
    int historySize,
    int graphColumn,
    int graphWidth,
    float& outValue
) {
    int start = (graphColumn * historySize) / graphWidth;
    int end = ((graphColumn + 1) * historySize) / graphWidth;
    if (end <= start) {
        end = start + 1;
    }
    if (end > historySize) {
        end = historySize;
    }

    float intervalSum = 0.0f;
    int intervalCount = 0;
    for (int src = start; src < end; ++src) {
        int idx = (static_cast<int>(head) + 1 + src) % historySize;
        if (!valid[idx]) {
            continue;
        }
        intervalSum += history[idx];
        ++intervalCount;
    }

    if (intervalCount == 0) {
        return false;
    }

    // 24-point smoothing window mapped over full history.
    int smoothingRadius = historySize / (2 * MasterStationApp::METRIC_COARSE_POINTS);
    if (smoothingRadius < 1) {
        smoothingRadius = 1;
    }

    int center = (start + end - 1) / 2;
    int from = center - smoothingRadius;
    int to = center + smoothingRadius;
    if (from < 0) {
        from = 0;
    }
    if (to >= historySize) {
        to = historySize - 1;
    }

    float smoothSum = 0.0f;
    int smoothCount = 0;
    for (int src = from; src <= to; ++src) {
        int idx = (static_cast<int>(head) + 1 + src) % historySize;
        if (!valid[idx]) {
            continue;
        }
        smoothSum += history[idx];
        ++smoothCount;
    }

    if (smoothCount > 0) {
        outValue = smoothSum / smoothCount;
    } else {
        outValue = intervalSum / intervalCount;
    }
    return true;
}

void drawMetricHistoryLine(
    U8G2& display,
    int x,
    int baselineY,
    int graphWidth,
    const float* history,
    const bool* valid,
    uint16_t head,
    float minValue,
    float maxValue
) {
    if (graphWidth <= 0 || maxValue <= minValue) {
        return;
    }

    const int topY = baselineY - 9;
    const int bottomY = baselineY;
    const int historySize = MasterStationApp::METRIC_HISTORY_POINTS;

    bool hasPrevious = false;
    int previousX = 0;
    int previousY = 0;
    for (int col = 0; col < graphWidth; ++col) {
        float value = 0.0f;
        if (!sampleCompressedHistoryValue(history, valid, head, historySize, col, graphWidth, value)) {
            hasPrevious = false;
            continue;
        }

        int px = x + col;
        int y = valueToGraphY(value, minValue, maxValue, topY, bottomY);
        if (!hasPrevious) {
            display.drawPixel(px, y);
            hasPrevious = true;
            previousX = px;
            previousY = y;
            continue;
        }

        display.drawLine(previousX, previousY, px, y);
        previousX = px;
        previousY = y;
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
    _lastDisplayActivityAt(0),
    _reportIntervalMs(reportIntervalMs),
    _lastPacketReceivedAt(0),
    _offlineTimeoutMs(offlineTimeoutMs),
    _historyInitialized(false),
    _historyHead(0),
    _isDisplayOn(true),
    _hasPacket(false),
    _hasSequence(false),
    _hasTempTrend(false),
    _hasHumidityTrend(false),
    _hasPressureTrend(false),
    _hasDewPointTrend(false),
    _hasTempScale(false),
    _hasHumidityScale(false),
    _hasPressureScale(false),
    _hasDewPointScale(false),
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
    _tempScaleMin(-20.0f),
    _tempScaleMax(50.0f),
    _humidityScaleMin(0.0f),
    _humidityScaleMax(100.0f),
    _pressureScaleMin(720.0f),
    _pressureScaleMax(790.0f),
    _dewPointScaleMin(-20.0f),
    _dewPointScaleMax(35.0f),
    _lastPacket(),
    _display(U8G2_R0, dispClk, dispData, dispCS, dispDC, dispReset),
    _touchButton("DisplayTouch", touchPin, 120, false, false),
    _receiver(receiver),
    _publisher(backend, backendIntervalMs) {
    clearMetricHistory();
}

void MasterStationApp::clearMetricHistory() {
    for (uint16_t i = 0; i < METRIC_HISTORY_POINTS; ++i) {
        _tempHistory[i] = 0.0f;
        _humidityHistory[i] = 0.0f;
        _pressureHistory[i] = 0.0f;
        _dewPointHistory[i] = 0.0f;
        _tempHistoryValid[i] = false;
        _humidityHistoryValid[i] = false;
        _pressureHistoryValid[i] = false;
        _dewPointHistoryValid[i] = false;
    }
}

void MasterStationApp::updateMetricHistory(const WeatherPacket& packet, float dewPointC, bool dewPointValid) {
    if (!_historyInitialized) {
        clearMetricHistory();
        _historyInitialized = true;
        _historyHead = 0;
    } else {
        _historyHead = static_cast<uint16_t>((_historyHead + 1) % METRIC_HISTORY_POINTS);
        _tempHistoryValid[_historyHead] = false;
        _humidityHistoryValid[_historyHead] = false;
        _pressureHistoryValid[_historyHead] = false;
        _dewPointHistoryValid[_historyHead] = false;
    }

    if (packet.climateValid && !isnan(packet.temperature)) {
        _tempHistory[_historyHead] = packet.temperature;
        _tempHistoryValid[_historyHead] = true;
    }
    if (packet.climateValid && !isnan(packet.humidity)) {
        _humidityHistory[_historyHead] = packet.humidity;
        _humidityHistoryValid[_historyHead] = true;
    }
    if (packet.pressureValid && !isnan(packet.pressure)) {
        _pressureHistory[_historyHead] = packet.pressure;
        _pressureHistoryValid[_historyHead] = true;
    }
    if (dewPointValid) {
        _dewPointHistory[_historyHead] = dewPointC;
        _dewPointHistoryValid[_historyHead] = true;
    }
}

void MasterStationApp::begin() {
    Logger::begin(115200);
    Logger::info("master-station", "Master station initialization started");

    if (_receiver) {
        _receiver->begin();
    }

    _touchButton.begin();
    _publisher.begin();

    if (hasBacklightControl()) {
        pinMode(DISPLAY_BACKLIGHT_PIN, OUTPUT);
        setBacklightEnabled(true);
    } else {
        Logger::warn("master-station", "Display backlight pin not configured; LED cannot be switched off in software");
    }

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

    _lastDisplayActivityAt = millis();

    Logger::info("master-station", "Display initialized");
    delay(1500); // Welcome screen duration
}

void MasterStationApp::tick() {
    handleTouchToggle();
    handleDisplayAutoOffTimeout();
    handleRadioReceive();
    handlePeriodicReport();
    handleBackendUpdate();
}

void MasterStationApp::setDisplayPowerState(bool enabled, bool showToast) {
    if (_isDisplayOn == enabled) {
        return;
    }

    _isDisplayOn = enabled;

    if (_isDisplayOn) {
        _display.setPowerSave(0);
        delay(10);
        _display.sendF("c", 0xA6);
        setBacklightEnabled(true);
        _display.setContrast(DISPLAY_BRIGHTNESS);
        _display.setDrawColor(1);
        _display.setFontMode(0);

        _lastReport = 0; // Force immediate redraw on next tick.
        _lastDisplayActivityAt = millis();
        Logger::info("master-station", "Display toggled ON");
        return;
    }

    // Keep the last measured values in display RAM and switch only the backlight off.
    if (showToast) {
        delay(50);
    }

    fadeBacklightOff();
    // Keep controller awake to avoid rare ST7565 dark-wake behavior.
    _display.sendF("c", 0xA6);
    Logger::info("master-station", "Display toggled OFF");
}

void MasterStationApp::handleDisplayAutoOffTimeout() {
    if (!_isDisplayOn || DISPLAY_AUTO_OFF_TIMEOUT_MS == 0) {
        return;
    }

    if ((millis() - _lastDisplayActivityAt) < DISPLAY_AUTO_OFF_TIMEOUT_MS) {
        return;
    }

    setDisplayPowerState(false, false);
    Logger::info("master-station", "Display auto-off timeout reached");
}

void MasterStationApp::handleTouchToggle() {
    if (!_touchButton.wasPressed()) {
        return;
    }

    _lastDisplayActivityAt = millis();
    setDisplayPowerState(!_isDisplayOn, true);
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

    const int graphX = 72;
    const int graphW = 56;
    const int trendX = graphX - 8;
    const int tempY = 11;
    const int humidityY = 24;
    const int pressureY = 37;
    const int dewPointY = 50;

    if (packet.climateValid && !isnan(packet.temperature) && !isnan(packet.humidity)) {
        dewPointC = calculateDewPointC(packet.temperature, packet.humidity);
        dewPointValid = !isnan(dewPointC);
        if (dewPointValid && fabsf(dewPointC) < 0.05f) {
            dewPointC = 0.0f;
        }
    }

    updateMetricHistory(packet, dewPointC, dewPointValid);

    const int historySize = METRIC_HISTORY_POINTS;
    updateClampedAutoScale(_tempHistory, _tempHistoryValid, _historyHead, historySize, -20.0f, 50.0f, _hasTempScale, _tempScaleMin, _tempScaleMax);
    updateClampedAutoScale(_humidityHistory, _humidityHistoryValid, _historyHead, historySize, 0.0f, 100.0f, _hasHumidityScale, _humidityScaleMin, _humidityScaleMax);
    updateClampedAutoScale(_pressureHistory, _pressureHistoryValid, _historyHead, historySize, 720.0f, 790.0f, _hasPressureScale, _pressureScaleMin, _pressureScaleMax);
    updateClampedAutoScale(_dewPointHistory, _dewPointHistoryValid, _historyHead, historySize, -20.0f, 35.0f, _hasDewPointScale, _dewPointScaleMin, _dewPointScaleMax);

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

        snprintf(buf, sizeof(buf), "T: %.1fC ", (double)packet.temperature);
        _display.drawStr(0, tempY, buf);
        drawTrendTriangle(_display, trendX, tempY, tempTrend);
        drawMetricHistoryLine(_display, graphX, tempY, graphW, _tempHistory, _tempHistoryValid, _historyHead, _tempScaleMin, _tempScaleMax);

        snprintf(buf, sizeof(buf), "H: %.0f%% ", (double)packet.humidity);
        _display.drawStr(0, humidityY, buf);
        drawTrendTriangle(_display, trendX, humidityY, humidityTrend);
        drawMetricHistoryLine(_display, graphX, humidityY, graphW, _humidityHistory, _humidityHistoryValid, _historyHead, _humidityScaleMin, _humidityScaleMax);
    } else {
        snprintf(buf, sizeof(buf), "T: --");
        _display.drawStr(0, tempY, buf);
        drawMetricHistoryLine(_display, graphX, tempY, graphW, _tempHistory, _tempHistoryValid, _historyHead, _tempScaleMin, _tempScaleMax);

        snprintf(buf, sizeof(buf), "H: --");
        _display.drawStr(0, humidityY, buf);
        drawMetricHistoryLine(_display, graphX, humidityY, graphW, _humidityHistory, _humidityHistoryValid, _historyHead, _humidityScaleMin, _humidityScaleMax);
    }

    if (packet.pressureValid && !isnan(packet.pressure)) {
        pressureTrend = trendDirection(packet.pressure, _prevPressure, 0.1f, _hasPressureTrend);

        if (pressureTrend != 0) {
            _lastPressureTrendDirection = pressureTrend;
            _lastPressureTrendAt = now;
        } else if ((now - _lastPressureTrendAt) <= TREND_TRIANGLE_HOLD_MS) {
            pressureTrend = _lastPressureTrendDirection;
        }

        snprintf(buf, sizeof(buf), "P: %.1f", (double)packet.pressure);
    } else {
        snprintf(buf, sizeof(buf), "P: --");
    }
    _display.drawStr(0, pressureY, buf);
    drawTrendTriangle(_display, trendX, pressureY, pressureTrend);
    drawMetricHistoryLine(_display, graphX, pressureY, graphW, _pressureHistory, _pressureHistoryValid, _historyHead, _pressureScaleMin, _pressureScaleMax);

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
    _display.drawStr(0, dewPointY, buf);
    drawTrendTriangle(_display, trendX, dewPointY, dewPointTrend);
    drawMetricHistoryLine(_display, graphX, dewPointY, graphW, _dewPointHistory, _dewPointHistoryValid, _historyHead, _dewPointScaleMin, _dewPointScaleMax);

    snprintf(buf, sizeof(buf), "%s", shortRainLevelText(static_cast<RainLevel>(packet.rainLevel)));
    _display.drawStr(0, 62, buf);

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
