#include "Logger.h"

void Logger::begin(unsigned long baudRate) {
    Serial.begin(baudRate);
    unsigned long started = millis();
    while (!Serial && (millis() - started < 1500)) {
    }
}

void Logger::log(LogLevel level, const char* module, const char* message) {
    Serial.print("[");
    Serial.print(millis());
    Serial.print("] [");
    Serial.print(levelToString(level));
    Serial.print("] [");
    Serial.print(module);
    Serial.print("] ");
    Serial.println(message);
}

void Logger::debug(const char* module, const char* message) { log(DEBUG, module, message); }
void Logger::info(const char* module, const char* message) { log(INFO, module, message); }
void Logger::warn(const char* module, const char* message) { log(WARN, module, message); }
void Logger::error(const char* module, const char* message) { log(ERROR, module, message); }

const char* Logger::levelToString(LogLevel level) {
    switch (level) {
        case DEBUG: return "DEBUG";
        case INFO:  return "INFO ";
        case WARN:  return "WARN ";
        case ERROR: return "ERROR";
        default:    return "UNKN ";
    }
}
