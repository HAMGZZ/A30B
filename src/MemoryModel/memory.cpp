#include <Arduino.h>
#include "memory.hpp"

Memory::Memory()
{

}

void Memory::Init()
{
    log.Start("MEMORY", INFO);
    log.Send(INFO, "INITIALISING MEMORY...");
    EEPROM.begin(256);
    log.Send(INFO, "GETTING CALLSIGN");
    pullString(callsign, sizeof(callsign));
    log.Send(INFO, "GOT CALLSIGN > ", callsign);
    log.Send(INFO, "GETTING ZERO FREQUENCY");
    EEPROM.get(CALLSIGN_LOC, zeroFreq);
    log.Send(INFO, "GETTING ONE FREQUENCY");
    EEPROM.get(CALLSIGN_LOC + sizeof(unsigned long long), oneFreq);
    if( ((zeroFreq < 900000000ULL) || (oneFreq < 900000000ULL)) || 
        ((zeroFreq > 16000000000ULL) || (oneFreq > 16000000000ULL)))
    {
        // Wrong freqs stored, lets default...
        log.Send(WARNING, "zeroFreq & oneFreq out of range - reseting");
        zeroFreq = DEFAULT_ZERO;
        oneFreq = DEFAULT_ONE;
        log.Send(INFO, "0 & 1 FREQ RESET");
        saveData();
    }
}

void Memory::pullString(char * buffer, int size, int startLoc)
{
    for (int i = startLoc; i < size; i++)
    {
        buffer[i] = EEPROM.read(i);
    }
}

void Memory::saveData()
{
    EEPROM.commit();
    log.Send(INFO, "MEMORY SAVED");
}

void Memory::writeString(char * buffer, int startLoc)
{
    for (int i = CALLSIGN_LOC; i < sizeof(buffer); i++)
    {
        EEPROM.write(i, buffer[i]);
    }
    saveData();
}

void Memory::SetCallsign(char * callsign)
{
    log.Send(INFO, "WRITTING CALLSIGN > ", callsign);
    writeString(callsign, CALLSIGN_LOC);
}

void Memory::SetZeroFreq(unsigned long long freq)
{
    //log.Send(INFO, "WRITTING ZERO FREQ > ", freq);
    EEPROM.put(CALLSIGN_LOC, freq);
}

void Memory::SetOneFreq(unsigned long long freq)
{

    //log.Send(INFO, "WRITTING ONE FREQ > ", freq);
    EEPROM.put(CALLSIGN_LOC + sizeof(unsigned long long), freq);
}