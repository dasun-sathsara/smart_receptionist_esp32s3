#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <MqttLogger.h>
#include <PubSubClient.h>

enum LogLevel {
    LOG_NONE = 0,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG
};

class Logger {
public:
    Logger();

    void begin(PubSubClient &client);

    void log(LogLevel level, const char *tag, const char *format, ...);

    static LogLevel currentLogLevel;

private:
    MqttLogger *mqttLogger;
    bool mqttInitialized;
    static const size_t LOG_BUFFER_SIZE = 256;
    char logBuffer[LOG_BUFFER_SIZE];
};

extern Logger logger;

#define LOG_E(tag, format, ...) logger.log(LOG_ERROR, tag, format, ##__VA_ARGS__)
#define LOG_W(tag, format, ...) logger.log(LOG_WARN,  tag, format, ##__VA_ARGS__)
#define LOG_I(tag, format, ...) logger.log(LOG_INFO,  tag, format, ##__VA_ARGS__)
#define LOG_D(tag, format, ...) logger.log(LOG_DEBUG, tag, format, ##__VA_ARGS__)

#endif // LOGGER_H