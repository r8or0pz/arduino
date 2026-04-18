#include "WiFiConnection.h"

#include <Logger.h>

WiFiConnection::WiFiConnection(
    const char* ssid,
    const char* password,
    unsigned long connectTimeoutMs,
    unsigned long retryDelayMs
) : _ssid(ssid),
    _password(password),
    _connectTimeoutMs(connectTimeoutMs),
    _retryDelayMs(retryDelayMs) {}

bool WiFiConnection::begin() {
    return ensureConnected();
}

bool WiFiConnection::ensureConnected() {
    if (isConnected()) {
        return true;
    }

    Logger::info("wifi", "Connecting to WiFi");
    WiFi.begin(_ssid, _password);

    unsigned long startedAt = millis();
    while (!isConnected() && (millis() - startedAt) < _connectTimeoutMs) {
        delay(_retryDelayMs);
    }

    if (isConnected()) {
        Logger::info("wifi", "WiFi connected");
        return true;
    }

    Logger::error("wifi", "WiFi connection failed");
    return false;
}

bool WiFiConnection::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

WiFiClient& WiFiConnection::client() {
    return _client;
}