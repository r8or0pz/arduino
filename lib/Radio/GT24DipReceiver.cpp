#include "GT24DipReceiver.h"
#include "RadioEncryption.h"
#include <WeatherStationConfig.h>
#include <string.h>
#include <Arduino.h>

// nRF24L01+ register addresses
#define NRF_CONFIG   0x00
#define EN_AA        0x01
#define EN_RXADDR    0x02
#define SETUP_AW     0x03
#define RF_CH        0x05
#define RF_SETUP     0x06
#define NRF_STATUS   0x07
#define RX_ADDR_P1   0x0B
#define RX_PW_P1     0x12
#define FIFO_STATUS  0x17

// nRF24L01+ commands
#define R_REGISTER   0x00
#define W_REGISTER   0x20
#define R_RX_PAYLOAD 0x61
#define FLUSH_RX     0xE2
#define NOP_NRF      0xFF

// Radio parameters from WeatherStationConfig.h — synchronized with transmitter
// (RADIO_CHANNEL, RADIO_ADDRESS, RADIO_PAYLOAD_LEN, RADIO_RF_SETUP are defined in config)

GT24DipReceiver::GT24DipReceiver(uint8_t cePin, uint8_t csnPin,
                                 uint8_t sckPin, uint8_t mosiPin, uint8_t misoPin)
    : _ce(cePin), _csn(csnPin), _sck(sckPin), _mosi(mosiPin), _miso(misoPin) {}

// --- Software SPI ---

uint8_t GT24DipReceiver::spiXfer(uint8_t b) {
    uint8_t out = 0;
    for (int8_t i = 7; i >= 0; i--) {
        digitalWrite(_mosi, (b >> i) & 1);
        digitalWrite(_sck, HIGH);
        out = (out << 1) | digitalRead(_miso);
        digitalWrite(_sck, LOW);
    }
    return out;
}

uint8_t GT24DipReceiver::readReg(uint8_t reg) {
    digitalWrite(_csn, LOW);
    spiXfer(R_REGISTER | (reg & 0x1F));
    uint8_t val = spiXfer(NOP_NRF);
    digitalWrite(_csn, HIGH);
    return val;
}

void GT24DipReceiver::writeReg(uint8_t reg, uint8_t val) {
    digitalWrite(_csn, LOW);
    spiXfer(W_REGISTER | (reg & 0x1F));
    spiXfer(val);
    digitalWrite(_csn, HIGH);
}

void GT24DipReceiver::writeRegBuf(uint8_t reg, const uint8_t* buf, uint8_t len) {
    digitalWrite(_csn, LOW);
    spiXfer(W_REGISTER | (reg & 0x1F));
    for (uint8_t i = 0; i < len; i++) { spiXfer(buf[i]); }
    digitalWrite(_csn, HIGH);
}

void GT24DipReceiver::readPayload(uint8_t* buf, uint8_t len) {
    digitalWrite(_csn, LOW);
    spiXfer(R_RX_PAYLOAD);
    for (uint8_t i = 0; i < len; i++) { buf[i] = spiXfer(NOP_NRF); }
    digitalWrite(_csn, HIGH);
}

void GT24DipReceiver::flushRx() {
    digitalWrite(_csn, LOW);
    spiXfer(FLUSH_RX);
    digitalWrite(_csn, HIGH);
}

// --- IRadioReceiver ---

bool GT24DipReceiver::begin() {
    pinMode(_ce,   OUTPUT); digitalWrite(_ce,   LOW);
    pinMode(_csn,  OUTPUT); digitalWrite(_csn,  HIGH);
    pinMode(_sck,  OUTPUT); digitalWrite(_sck,  LOW);
    pinMode(_mosi, OUTPUT); digitalWrite(_mosi, LOW);
    pinMode(_miso, INPUT);

    delay(100); // nRF24L01+ power-on-reset

    writeReg(NRF_CONFIG,  0x0F); // EN_CRC | CRCO(2-byte) | PWR_UP | PRIM_RX
    writeReg(EN_AA,       0x00); // disable auto-ack
    writeReg(EN_RXADDR,   0x02); // enable pipe 1
    writeReg(SETUP_AW,    0x03); // 5-byte address width
    writeReg(RF_CH,       RADIO_CHANNEL);
    writeReg(RF_SETUP,    RADIO_RF_SETUP);
    writeRegBuf(RX_ADDR_P1, (uint8_t *)RADIO_ADDRESS, 5);
    writeReg(RX_PW_P1,    RADIO_PAYLOAD_LEN);
    flushRx();
    writeReg(NRF_STATUS,  0x70); // clear RX_DR | TX_DS | MAX_RT

    digitalWrite(_ce, HIGH);     // start listening
    delayMicroseconds(150);

    return true;
}

bool GT24DipReceiver::receive(WeatherPacket& packet) {
    uint8_t status = readReg(NRF_STATUS);
    if (!(status & (1 << 6))) {  // RX_DR bit
        return false;
    }

    uint8_t buf[RADIO_PAYLOAD_LEN];
    readPayload(buf, RADIO_PAYLOAD_LEN);
    writeReg(NRF_STATUS, 1 << 6); // clear RX_DR

    // Flush if more data in FIFO
    if (!(readReg(FIFO_STATUS) & 0x01)) {
        flushRx();
    }

    // Decrypt payload
    EncryptedWeatherPacket encrypted;
    memcpy(&encrypted.data[0], buf, RADIO_PAYLOAD_LEN);
    RadioEncryption::decrypt(encrypted, packet);
    return true;
}
