#ifndef THINGSPEAK_BACKEND_H
#define THINGSPEAK_BACKEND_H

#include "IBackend.h"
#include <ThingSpeak.h>
#include <WiFiS3.h>

class ThingSpeakBackend : public IBackend {
public:
    ThingSpeakBackend(unsigned long channelId, const char* writeApiKey);
    bool begin() override;
    bool sendData(const WeatherData& data) override;

private:
    unsigned long _channelId;
    const char* _writeApiKey;
    WiFiClient _client;
};

#endif
