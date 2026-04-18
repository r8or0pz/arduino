#include "ThingSpeakBackend.h"
#include <Logger.h>

ThingSpeakBackend::ThingSpeakBackend(WiFiConnection& wifiConnection, unsigned long channelId, const char* writeApiKey)
    : _wifiConnection(wifiConnection), _channelId(channelId), _writeApiKey(writeApiKey) {}

bool ThingSpeakBackend::begin() {
    if (!_wifiConnection.begin()) {
        Logger::error("ThingSpeak", "WiFi initialization failed");
        return false;
    }

    ThingSpeak.begin(_wifiConnection.client());
    return true;
}

bool ThingSpeakBackend::sendData(const WeatherData& data) {
    if (!_wifiConnection.ensureConnected()) {
        Logger::error("ThingSpeak", "WiFi not connected");
        return false;
    }

    if (isnan(data.temperature)) {
        ThingSpeak.setField(1, "");
    } else {
        ThingSpeak.setField(1, data.temperature);
    }

    if (isnan(data.humidity)) {
        ThingSpeak.setField(2, "");
    } else {
        ThingSpeak.setField(2, data.humidity);
    }

    ThingSpeak.setField(3, (float)data.rainIntensity);

    if (isnan(data.pressure)) {
        ThingSpeak.setField(4, "");
    } else {
        ThingSpeak.setField(4, data.pressure);
    }

    int status = ThingSpeak.writeFields(_channelId, _writeApiKey);

    if (status == 200) {
        Logger::info("ThingSpeak", "Channel update successful");
        return true;
    } else {
        String msg = "Problem updating channel. HTTP error code ";
        msg += String(status);
        Logger::error("ThingSpeak", msg.c_str());
        return false;
    }
}
