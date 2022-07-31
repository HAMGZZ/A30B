#include "ax25/ax25.hpp"

AX25::AX25()
{

}

void AX25::begin(char * sourceAddress, char * icon, long baudRate, int txEnablePin, int dataOutPin)
{
    strcpy(this->sourceAddress, sourceAddress);
    strcpy(this->icon, icon);
    this->txEnablePin = txEnablePin;
    this->dataOutPin = dataOutPin;
    db = 1000000/baudRate;
}

void AX25::buildPacket(const char * information, bool debug)
{
  memset(packet, 0, 332);
  memset(destinationAdress+3, 0, 5);
  packet[0] = 0x7e;
  strcat(destinationAdress, icon);
  strcat(packet, destinationAdress);
  strcat(packet, sourceAddress);
  packet[14] = 0x03;
  packet[15] = 0xf0;
  strcat(packet, information);
  char subset[strlen(packet)] = {0};
  for(int i = 1; i < strlen(packet); i++)
  {
    subset[i-1] = packet[i];
  }
  uint16_t fcs = checksum((const char*)subset, strlen(subset));
  if(debug)
  {
    Serial.printf("\r\nSource address: \t%s\r\n", sourceAddress);
    Serial.printf("Icon          : \t%s\r\n", icon);
    Serial.printf("Dest adress   : \t%s\r\n", destinationAdress);
    Serial.printf("Subset String : \t%s\r\n", subset);
    Serial.printf("Calculated FCS: \t0x%04x\t", fcs);
    Tools::PrintBinary(&fcs, 16);
    Serial.printf("\r\n");
  }
  packet[strlen(packet)] = fcs & 0xff;
  packet[strlen(packet)] = (fcs>>8) & 0xff;
  packet[strlen(packet)] = 0x7e;
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

void AX25::shiftOut()
{
  int i, ii;
  digitalWrite(txEnablePin, 1);
  for ( i = 0; i < strlen(packet); i++)
  {
    for (ii = 0; ii < 8; ii++)
    {
      digitalWrite(dataOutPin, (packet[i] >> ii) & 0x01);
      delayMicroseconds(db);
    }
  }
  digitalWrite(txEnablePin, 0);
}
