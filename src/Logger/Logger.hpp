#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <Arduino.h>


typedef enum
{
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
} Level;

class Logger
{
private:
    char *name;
    Level minLogLevel;
    char const *enum_to_string(Level type);
    //time_t getTeensyTime();
public:
    Logger();
    Logger(char const *SystemName, Level minLogLevelSet);
    void Start(char const *SystemName, Level minLogLevelSet);
    void Send(Level level, char const *message, float var1 = (0.0F), float var2 = (0.0F));
    void Send(Level level, char const *message, unsigned long long var1);
    void Send(Level level, char const *message, bool var1);
    void Send(Level level, char const *message, char *message2);
    ~Logger();
};



#endif