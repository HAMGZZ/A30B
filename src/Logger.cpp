#include "Logger.hpp"
#include "defines.hpp"

Logger::Logger()
{
    name = new (char[30]);
}

Logger::Logger(char const *systemName, Level minLogLevelSet)
{
    name = new (char[30]);
    strcpy(name, systemName);
    minLogLevel = minLogLevelSet;
}


char const *Logger::enum_to_string(Level type) {
   switch(type) {
      case DEBUG:
         return "DEBUG";
      case INFO:
         return "INFO";
      case WARNING:
         return "WARNING";
      case ERROR:
         return "ERROR";
      case FATAL:
         return "FATAL";
   }
   return "";
}

void Logger::Start(char const *systemName, Level minLogLevelSet)
{
    strcpy(name, systemName);
    minLogLevel = minLogLevelSet;
}

void Logger::Send(Level level, const char *message, float var1, float var2)
{
    
    if(level < minLogLevel)
        return;

    if(var1 == 0 && var2 == 0)
    {
        Serial.printf("LOG [%s %0.1lf][%s][%s]::  %s\n\r", PROG_NAME, VERSION, name, enum_to_string(level), message);
    }

    else if(var2 == 0)
    {
        Serial.printf("LOG [%s %0.1lf][%s][%s]::  %s %lf\n\r", PROG_NAME, VERSION, name, enum_to_string(level), message, var1);
    }
    else
    {
        Serial.printf("LOG [%s %0.1lf][%s][%s]::  %s %lf %lf\n\r", PROG_NAME, VERSION, name, enum_to_string(level), message, var1, var2);
    }
    
    if(level == FATAL)
    {
        Serial.printf("HALTING...\r\n");
        for(;;){}
    }

}

void Logger::Send(Level level, const char *message, bool var1)
{
    if(level < minLogLevel)
        return;

    Serial.printf("LOG [%s %0.1lf][%s][%s]::  %s %s\n\r", PROG_NAME, VERSION, name, enum_to_string(level), message, var1 ? "True" : "False");
    
    if(level == FATAL)
    {
        Serial.printf("HALTING...\r\n");
        for(;;){}
    }
}


Logger::~Logger()
{
    delete[] name;
}