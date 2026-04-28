#ifndef RADIO_ENCRYPTION_H
#define RADIO_ENCRYPTION_H

#include <Arduino.h>
#include <WeatherPacket.h>
#include <WeatherStationConfig.h>

// AES-128 encrypted payload: 24-byte WeatherPacket + 8 bytes padding = 32 bytes (2 AES blocks).
struct __attribute__((packed)) EncryptedWeatherPacket {
    uint8_t data[32];
};

class RadioEncryption {
public:
    // Encrypt WeatherPacket to EncryptedWeatherPacket (24 bytes -> 32 bytes with padding).
    static void encrypt(const WeatherPacket& plaintext, EncryptedWeatherPacket& ciphertext);

    // Decrypt EncryptedWeatherPacket back to WeatherPacket.
    static bool decrypt(const EncryptedWeatherPacket& ciphertext, WeatherPacket& plaintext);

private:
    // Encryption key is configured through WeatherStationConfig.h -> RadioSecrets.h.
};

#endif
