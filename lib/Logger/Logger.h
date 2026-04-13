#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

enum LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger {
public:
    static void begin(unsigned long baudRate = 115200);
    static void log(LogLevel level, const char* module, const char* message);
    static void debug(const char* module, const char* message);
    static void info(const char* module, const char* message);
    static void warn(const char* module, const char* message);
    static void error(const char* module, const char* message);

private:
    static const char* levelToString(LogLevel level);
};

#endif
