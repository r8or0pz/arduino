#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

#include <Arduino.h>
#include <WiFiS3.h>

class WiFiConnection {
public:
    WiFiConnection(
        const char* ssid,
        const char* password,
        unsigned long connectTimeoutMs = 15000,
        unsigned long retryDelayMs = 500
    );

    bool begin();
    bool ensureConnected();
    bool isConnected() const;
    WiFiClient& client();

private:
    const char* _ssid;
    const char* _password;
    unsigned long _connectTimeoutMs;
    unsigned long _retryDelayMs;
    WiFiClient _client;
};

#endif