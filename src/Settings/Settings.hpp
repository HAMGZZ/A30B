#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <Arduino.h>
#include <LittleFS.h>
#include "tools/tools.hpp"
#include "defines.hpp"



class Settings
{
private: 
public:

    char Callsign[CALLSIGN_SIZE];
    char Icon;
    char Comment[COMMENT_SIZE];
    unsigned long long ZeroFreq;
    unsigned long long OneFreq;
    long long Offset;
    unsigned long BaudRate;
    unsigned long TransmissionDelay;
    bool ShellColour;

    int Init();
    int Write();
    int Read();
};

#endif
