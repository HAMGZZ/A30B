#ifndef AX25_HPP
#define AX25_HPP

#include <Arduino.h>
#include "Logger/Logger.hpp"

#define FLAG 0xE7
#define CONTROL_FIELD 0x03
#define PROTOCOL_ID 0xF0

class AX25
{
    private:
        char sourceAddress[8] = {0}; //7 + \n
        char packet[332] = {0}; // Entire packet
        long baudRate = 0;
        char icon[4] = {0};
        Logger log;
    public:
        AX25();
        AX25(char * sourceAddress, long baudRate, char * icon);
        void begin(char * sourceAddress, long baudRate, char * icon);
        uint16_t checksum(const char * data, long size);
        void buildPacket(char * information);
        void shiftOut(int txEnablePin, int dataOutPin);
};

#endif
