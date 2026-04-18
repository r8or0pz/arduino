#ifndef THINGSPEAK_BACKEND_H
#define THINGSPEAK_BACKEND_H

#include "IBackend.h"
#include <WiFiConnection.h>
#include <ThingSpeak.h>

class ThingSpeakBackend : public IBackend {
public:
    ThingSpeakBackend(WiFiConnection& wifiConnection, unsigned long channelId, const char* writeApiKey);
    bool begin() override;
    bool sendData(const WeatherData& data) override;

private:
    WiFiConnection& _wifiConnection;
    unsigned long _channelId;
    const char* _writeApiKey;
};

#endif
