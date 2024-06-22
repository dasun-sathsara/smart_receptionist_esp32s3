#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <cstdarg>

enum LogLevel {
    LOG_NONE = 0,   // No logging
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG
};

extern LogLevel currentLogLevel; // Declare as extern

extern void logger(LogLevel level, const char *tag, const char *format, ...); // Declare as extern

// Macros for convenience (similar to esp_log)
#define LOG_E(tag, format, ...) logger(LOG_ERROR, tag, format, ##__VA_ARGS__)
#define LOG_W(tag, format, ...) logger(LOG_WARN,  tag, format, ##__VA_ARGS__)
#define LOG_I(tag, format, ...) logger(LOG_INFO,  tag, format, ##__VA_ARGS__)
#define LOG_D(tag, format, ...) logger(LOG_DEBUG, tag, format, ##__VA_ARGS__)

#endif // LOGGER_H
