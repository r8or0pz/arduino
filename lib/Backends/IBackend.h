#ifndef IBACKEND_H
#define IBACKEND_H

struct WeatherData {
    float temperature;
    float humidity;
    int rainIntensity;
    bool isRaining;
};

class IBackend {
public:
    virtual ~IBackend() {}
    virtual bool begin() = 0;
    virtual bool sendData(const WeatherData& data) = 0;
};

#endif
