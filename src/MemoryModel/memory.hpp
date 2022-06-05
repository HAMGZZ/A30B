#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <Arduino.h>
#include "Logger/Logger.hpp"
#include "defines.hpp"
#include <EEPROM.h>


class Memory
{
private:
    Logger log;
    
    void pullString(char * buffer, int size, int startLoc = 0);
    void writeString(char * buffer, int startLoc = 0);
    void saveData();

public:

    Memory();
    void Init();

    char callsign[CALLSIGN_SIZE] = {0};
    unsigned long long zeroFreq = 0;
    unsigned long long oneFreq = 0;
    
    // Setters to memory
    void SetCallsign(char * callsign);
    void SetZeroFreq(unsigned long long freq);
    void SetOneFreq(unsigned long long freq);
};




#endif