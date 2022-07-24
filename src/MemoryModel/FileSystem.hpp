#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <Arduino.h>
#include <LittleFS.h>
#include "tools/tools.hpp"
#include "defines.hpp"



class FileSystem
{
private: 
    FS fs = LittleFS;

public:

    char Callsign[CALLSIGN_SIZE];
    char Icon[ICON_SIZE];
    unsigned long long CentreFrequency;
    unsigned long long FrequencyShift;
    unsigned long BaudRate;
    unsigned long TransmissionDelay;
    bool ShellColour;

    FileSystem();

    void Init();
    void Save();
    int Read();
};

#endif