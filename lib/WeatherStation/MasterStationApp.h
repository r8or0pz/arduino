#ifndef MASTER_STATION_APP_H
#define MASTER_STATION_APP_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <Sensors.h>
#include <IBackend.h>
#include <CloudPublisher.h>
#include <IRadioReceiver.h>
#define WEATHER_STATION_MASTER_CONFIG
#include <WeatherStationConfig.h>

class MasterStationApp {
public:
    // Display wiring (GMG12864-06D via software SPI):
    //   dispClk  = SCL = D7
    //   dispData = SI  = D6
    //   dispCS   = CS  = D10
    //   dispDC   = RS  = D8
    //   dispReset= RSE = D9
    MasterStationApp(
        uint8_t touchPin,
        IRadioReceiver* receiver,
        IBackend* backend          = nullptr,
        uint8_t dispClk            = 7,
        uint8_t dispData           = 6,
        uint8_t dispCS             = 10,
        uint8_t dispDC             = 8,
        uint8_t dispReset          = 9,
        unsigned long reportIntervalMs  = 2000,
        unsigned long backendIntervalMs = 15000,
        unsigned long offlineTimeoutMs  = 60000
    );

    void begin();
    void tick();

private:
    void handleTouchToggle();
    void handleRadioReceive();
    void handleBackendUpdate();
    void handlePeriodicReport();
    void renderPacketOnDisplay(const WeatherPacket& packet);

    unsigned long _lastReport;
    unsigned long _reportIntervalMs;
    unsigned long _lastPacketReceivedAt;
    unsigned long _offlineTimeoutMs;
    bool _isDisplayOn;
    bool _hasPacket;
    bool _hasSequence;
    bool _hasTempTrend;
    bool _hasHumidityTrend;
    bool _hasPressureTrend;
    bool _hasDewPointTrend;
    int8_t _lastTempTrendDirection;
    int8_t _lastHumidityTrendDirection;
    int8_t _lastPressureTrendDirection;
    int8_t _lastDewPointTrendDirection;
    unsigned long _lastTempTrendAt;
    unsigned long _lastHumidityTrendAt;
    unsigned long _lastPressureTrendAt;
    unsigned long _lastDewPointTrendAt;
    uint8_t _lastSequence;
    float _prevTemperature;
    float _prevHumidity;
    float _prevPressure;
    float _prevDewPoint;
    WeatherPacket _lastPacket;

    U8G2_ST7565_JLX12864_F_4W_SW_SPI _display;
    TouchSensor _touchButton;
    IRadioReceiver* _receiver;
    CloudPublisher _publisher;
};

#endif
