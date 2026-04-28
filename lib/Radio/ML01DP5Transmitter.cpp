#include "ML01DP5Transmitter.h"
#include "RadioEncryption.h"
#include <WeatherStationConfig.h>
#include <SPI.h>
#include <string.h>

// nRF24L01+ register addresses
#define NRF_CONFIG   0x00
#define EN_AA        0x01
#define EN_RXADDR    0x02
#define SETUP_AW     0x03
#define SETUP_RETR   0x04
#define RF_CH        0x05
#define RF_SETUP     0x06
#define NRF_STATUS   0x07
#define TX_ADDR      0x10

// nRF24L01+ commands
#define W_REGISTER   0x20
#define W_TX_PAYLOAD 0xA0
#define FLUSH_TX     0xE1
#define NOP_NRF      0xFF

// STATUS bits
#define TX_DS  5
#define MAX_RT 4

// Radio parameters from WeatherStationConfig.h — synchronized with receiver
// (RADIO_CHANNEL, RADIO_ADDRESS, RADIO_PAYLOAD_LEN, RADIO_RF_SETUP are defined in config)

static const SPISettings NRF_SPI(4000000, MSBFIRST, SPI_MODE0);

ML01DP5Transmitter::ML01DP5Transmitter(uint8_t cePin, uint8_t csnPin)
    : _ce(cePin), _csn(csnPin), _lastDebug() {
    memset(&_lastDebug, 0, sizeof(_lastDebug));
}

// --- Hardware SPI helpers ---

uint8_t ML01DP5Transmitter::spiXfer(uint8_t b) {
    return SPI.transfer(b);
}

uint8_t ML01DP5Transmitter::readReg(uint8_t reg) {
    SPI.beginTransaction(NRF_SPI);
    digitalWrite(_csn, LOW);
    SPI.transfer(0x00 | (reg & 0x1F));
    uint8_t val = SPI.transfer(NOP_NRF);
    digitalWrite(_csn, HIGH);
    SPI.endTransaction();
    return val;
}

void ML01DP5Transmitter::writeReg(uint8_t reg, uint8_t val) {
    SPI.beginTransaction(NRF_SPI);
    digitalWrite(_csn, LOW);
    SPI.transfer(W_REGISTER | (reg & 0x1F));
    SPI.transfer(val);
    digitalWrite(_csn, HIGH);
    SPI.endTransaction();
}

void ML01DP5Transmitter::writeRegBuf(uint8_t reg, const uint8_t* buf, uint8_t len) {
    SPI.beginTransaction(NRF_SPI);
    digitalWrite(_csn, LOW);
    SPI.transfer(W_REGISTER | (reg & 0x1F));
    for (uint8_t i = 0; i < len; i++) { SPI.transfer(buf[i]); }
    digitalWrite(_csn, HIGH);
    SPI.endTransaction();
}

void ML01DP5Transmitter::flushTx() {
    SPI.beginTransaction(NRF_SPI);
    digitalWrite(_csn, LOW);
    SPI.transfer(FLUSH_TX);
    digitalWrite(_csn, HIGH);
    SPI.endTransaction();
}

// --- IRadioTransmitter ---

bool ML01DP5Transmitter::begin() {
    pinMode(_ce,  OUTPUT); digitalWrite(_ce,  LOW);
    pinMode(_csn, OUTPUT); digitalWrite(_csn, HIGH);

    SPI.begin();
    delay(100); // nRF24L01+ power-on-reset

    writeReg(NRF_CONFIG,  0x0E); // EN_CRC | CRCO(2-byte) | PWR_UP | PRIM_TX
    writeReg(EN_AA,       0x00); // disable auto-ack
    writeReg(EN_RXADDR,   0x00); // no RX pipes needed
    writeReg(SETUP_AW,    0x03); // 5-byte address width
    writeReg(SETUP_RETR,  0x00); // no retransmit
    writeReg(RF_CH,       RADIO_CHANNEL);
    writeReg(RF_SETUP,    RADIO_RF_SETUP);
    writeRegBuf(TX_ADDR, (uint8_t *)RADIO_ADDRESS, 5);
    flushTx();
    writeReg(NRF_STATUS,  0x70); // clear RX_DR | TX_DS | MAX_RT
    return true;
}

bool ML01DP5Transmitter::send(const WeatherPacket& packet) {
    _lastDebug.sendOk      = false;
    _lastDebug.sequence    = packet.sequence;
    _lastDebug.sentAtMs    = millis();
    _lastDebug.payloadSize = RADIO_PAYLOAD_LEN;

    // Encrypt payload
    EncryptedWeatherPacket encrypted;
    RadioEncryption::encrypt(packet, encrypted);

    // Write TX payload
    SPI.beginTransaction(NRF_SPI);
    digitalWrite(_csn, LOW);
    SPI.transfer(W_TX_PAYLOAD);
    for (uint8_t i = 0; i < RADIO_PAYLOAD_LEN; i++) { SPI.transfer(encrypted.data[i]); }
    digitalWrite(_csn, HIGH);
    SPI.endTransaction();

    // Pulse CE to trigger transmission (>10 µs)
    digitalWrite(_ce, HIGH);
    delayMicroseconds(15);
    digitalWrite(_ce, LOW);

    // Poll STATUS for TX_DS or MAX_RT (up to 20 ms)
    unsigned long deadline = millis() + 20;
    uint8_t status = 0;
    do {
        SPI.beginTransaction(NRF_SPI);
        digitalWrite(_csn, LOW);
        status = SPI.transfer(NOP_NRF);
        digitalWrite(_csn, HIGH);
        SPI.endTransaction();
    } while (!(status & ((1 << TX_DS) | (1 << MAX_RT))) && millis() < deadline);

    writeReg(NRF_STATUS, 0x70); // clear flags
    if (status & (1 << MAX_RT)) { flushTx(); }

    _lastDebug.sendOk = (status & (1 << TX_DS)) != 0;
    return _lastDebug.sendOk;
}

const ML01DP5Transmitter::TxDebugInfo& ML01DP5Transmitter::lastDebug() const {
    return _lastDebug;
}
