#ifndef AX25_HPP
#define AX25_HPP

#include <Arduino.h>
#include "Tools/tools.hpp"

#define FLAG 0xE7
#define CONTROL_FIELD 0x03
#define PROTOCOL_ID 0xF0

class AX25
{
    private:
        char sourceAddress[8] = {0}; //7 + \n
        char destinationAdress[8] ={'G','P','S',0,0,0};
        unsigned long long db = 0;
        char icon[4] = {0};
        int txEnablePin; 
        int dataOutPin;
    public:
        AX25();
        char packet[332] = {0}; // Entire packet
        void begin(char * sourceAddress, char * icon, long baudRate, int txEnablePin, int dataOutPin);
        uint16_t checksum(const char * data, long size);
        void buildPacket(const char * information, bool debug = false);
        void shiftOut();
};

#endif
