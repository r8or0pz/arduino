#include <Sensors.h>
#include <WeatherPacket.h>
#include <ML01DP5Transmitter.h>
#define WEATHER_STATION_SLAVE_CONFIG
#include <WeatherStationConfig.h>

namespace {

// Configuration is now centralized in lib/Config/WeatherStationConfig.h
// All constants (sensor addresses, pin assignments, timing, etc.) are defined there.

RainSensor rainSensor("RainDigital", RAIN_DIGITAL_PIN, "RainAnalog", RAIN_ANALOG_PIN);
Sht3xSensor temperatureSensor("Temperature", SHT3X_ADDRESS, SHT3X_TEMPERATURE_C);
Sht3xSensor humiditySensor("Humidity", SHT3X_ADDRESS, SHT3X_HUMIDITY);
Bmp580PressureSensor pressureSensor("Pressure", BMP580_ADDRESS);
ML01DP5Transmitter radio(RADIO_CE_PIN, RADIO_CSN_PIN);
unsigned long lastReportAt = 0;
unsigned long lastClimateAt = 0;
float lastTemperature = NAN;
float lastHumidity = NAN;
float lastPressure = NAN;
bool hasClimate = false;
bool pressureValid = false;
uint8_t sequence = 0;

void debugLog(const __FlashStringHelper* message) {
    Serial.println(message);
}

void debugPacket(const WeatherPacket& packet, const ML01DP5Transmitter::TxDebugInfo& tx) {
    Serial.print(F("seq="));
    Serial.print(packet.sequence);
    Serial.print(F(" ts="));
    Serial.print(packet.sourceTimestampMs);
    Serial.print(F(" climate="));
    Serial.print(packet.climateValid ? F("ok") : F("fail"));
    Serial.print(F(" temp="));
    Serial.print(packet.temperature, 2);
    Serial.print(F(" hum="));
    Serial.print(packet.humidity, 2);
    Serial.print(F(" pressure="));
    Serial.print(packet.pressure, 2);
    Serial.print(F(" pressureValid="));
    Serial.print(packet.pressureValid ? F("yes") : F("no"));
    Serial.print(F(" rainInt="));
    Serial.print(packet.rainIntensity);
    Serial.print(F(" rainState="));
    Serial.print(packet.isRaining ? F("wet") : F("dry"));
    Serial.print(F(" send="));
    Serial.print(tx.sendOk ? F("ok") : F("fail"));
    Serial.print(F(" payload="));
    Serial.println(tx.payloadSize);
}

float adjustPressureToSeaLevel(float stationPressureHpa) {
    if (isnan(stationPressureHpa) || STATION_ELEVATION_METERS <= 0.0f) {
        return stationPressureHpa;
    }

    return stationPressureHpa / pow(1.0f - (STATION_ELEVATION_METERS / 44330.0f), 5.255f);
}

float convertPressureToMmHg(float pressureHpa) {
    if (isnan(pressureHpa)) {
        return pressureHpa;
    }

    return pressureHpa * 0.75006156f;
}

bool readClimate(float& temperature, float& humidity, float& pressure, bool& pressureOk) {
    unsigned long now = millis();
    if (hasClimate && (now - lastClimateAt) < CLIMATE_REFRESH_INTERVAL_MS) {
        temperature = lastTemperature;
        humidity = lastHumidity;
        pressure = lastPressure;
        pressureOk = pressureValid;
        debugLog(F("climate cache hit"));
        return true;
    }

    SensorReading temperatureReading = temperatureSensor.read();
    SensorReading humidityReading = humiditySensor.read();
    SensorReading pressureReading = pressureSensor.read();

    if (!temperatureReading.valid || !humidityReading.valid) {
        Serial.print(F("climate read invalid t="));
        Serial.print(temperatureReading.valid ? F("ok") : F("fail"));
        Serial.print(F(" h="));
        Serial.println(humidityReading.valid ? F("ok") : F("fail"));

        if (hasClimate) {
            temperature = lastTemperature;
            humidity = lastHumidity;
            pressure = lastPressure;
            pressureOk = pressureValid;
            debugLog(F("climate fallback to cache"));
            return true;
        }

        temperature = NAN;
        humidity = NAN;
        pressureOk = pressureReading.valid;
        pressure = pressureOk ? convertPressureToMmHg(adjustPressureToSeaLevel(pressureReading.value)) : NAN;
        return false;
    }

    lastTemperature = temperatureReading.value;
    lastHumidity = humidityReading.value;
    pressureValid = pressureReading.valid;
    lastPressure = pressureValid ? convertPressureToMmHg(adjustPressureToSeaLevel(pressureReading.value)) : NAN;
    lastClimateAt = now;
    hasClimate = true;

    Serial.print(F("climate fresh t="));
    Serial.print(lastTemperature, 2);
    Serial.print(F(" h="));
    Serial.print(lastHumidity, 2);
    Serial.print(F(" p="));
    Serial.println(lastPressure, 2);

    temperature = lastTemperature;
    humidity = lastHumidity;
    pressure = lastPressure;
    pressureOk = pressureValid;
    return true;
}

}

void setup() {
    Serial.begin(DEBUG_BAUD_RATE);
    debugLog(F("slave boot"));
    Serial.print(F("radio CE="));
    Serial.print(RADIO_CE_PIN);
    Serial.print(F(" CSN="));
    Serial.println(RADIO_CSN_PIN);

    // Temporary: disable rain sensor hardware invocation while keeping implementation in code.
    // rainSensor.begin();
    temperatureSensor.begin();
    humiditySensor.begin();
    pressureSensor.begin();
    radio.begin();

    debugLog(F("sensors initialized"));
    debugLog(F("radio initialized"));
}

void loop() {
    unsigned long now = millis();
    if ((now - lastReportAt) < REPORT_INTERVAL_MS) {
        return;
    }

    lastReportAt = now;

    // Temporary: disable rain sensor hardware invocation while keeping implementation in code.
    // RainReading rain = rainSensor.read();

    float temperature = NAN;
    float humidity = NAN;
    float pressure = NAN;
    bool currentPressureValid = false;
    bool climateValid = readClimate(temperature, humidity, pressure, currentPressureValid);

    WeatherPacket packet;
    packet.version = 1;
    packet.sequence = sequence++;
    packet.sourceTimestampMs = now;
    packet.temperature = temperature;
    packet.humidity = humidity;
    packet.pressure = pressure;
    packet.rainIntensity = 0;
    packet.rainLevel = static_cast<uint8_t>(RAIN_NONE);
    packet.isRaining = false;
    packet.climateValid = climateValid;
    packet.pressureValid = currentPressureValid;

    radio.send(packet);
    debugPacket(packet, radio.lastDebug());
}
