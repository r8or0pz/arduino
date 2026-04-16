#include "ThingSpeakBackend.h"
#include <Logger.h>

ThingSpeakBackend::ThingSpeakBackend(unsigned long channelId, const char* writeApiKey)
    : _channelId(channelId), _writeApiKey(writeApiKey) {}

bool ThingSpeakBackend::begin() {
    ThingSpeak.begin(_client);
    return true;
}

bool ThingSpeakBackend::sendData(const WeatherData& data) {
    ThingSpeak.setField(1, data.temperature);
    ThingSpeak.setField(2, data.humidity);
    ThingSpeak.setField(3, (float)data.rainIntensity);
    
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
