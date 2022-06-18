#include "ax25/ax25.hpp"

AX25::AX25()
{

}

AX25::AX25(char * sourceAddress, long baudRate, char * icon)
{
    log.Start("AX25", INFO);
    strcpy(this->sourceAddress, sourceAddress);
    this->baudRate = baudRate;
    log.Send(INFO, "AX25 Source Address -> ", this->sourceAddress);
}

void AX25::begin(char * sourceAddress, long baudRate, char * icon)
{
    log.Start("AX25", INFO);
    strcpy(this->sourceAddress, sourceAddress);
    this->baudRate = baudRate;
    log.Send(INFO, "AX25 Source Address -> ", this->sourceAddress);
}

uint16_t AX25::checksum(const char * data, long len)
{
    uint16_t crc = 0xFFFF;
    const uint16_t poly = 0x1021;

    for(long i = 0; i < len; i++)
    {
        crc ^= data[i] << 8;
        for(int j = 0; j < 8; j++)
        {
            if((crc & 0x8000) != 0)
            {
                crc <<= 1;
                crc = crc ^ poly;
            }
            else
                crc <<= 1;
        }
    }
    return crc ^ 0xFFFF;
}