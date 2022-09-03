#include "ax25/ax25.hpp"

AX25::AX25()
{

}

void AX25::begin(char * sourceAddress, char * icon, long baudRate, int txEnablePin, int dataOutPin)
{
    // Load in settings
    strcpy(this->sourceAddress, sourceAddress);
    strcpy(this->icon, icon);
    this->txEnablePin = txEnablePin;
    this->dataOutPin = dataOutPin;
    db = 10000000/baudRate;
}

void AX25::buildPacket(const char * information, bool debug)
{
  // Let's clear contents of previous packet
  memset(packet, 0, 332);
  memset(destinationAdress+3, 0, 5);

  // SOF byte
  packet[0] = FLAG;

  // Add icon do dest address
  strcat(destinationAdress, icon);
  
  // Add dest & source address to packet
  strcat(packet, destinationAdress);
  strcat(packet, sourceAddress);

  // Control and Protocol IDs
  packet[14] = CONTROL_FIELD;
  packet[15] = PROTOCOL_ID;

  // Let's add the payload to the packet
  strcat(packet, information);

  // Let's create a subset string to use for FCS
  char subset[strlen(packet)] = {0};
  for(int i = 1; i < strlen(packet); i++)
  {
    subset[i-1] = packet[i];
  }

  // Let's calculate the FCS
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

  // Let's append the FCS
  packet[strlen(packet)] = fcs & 0xff;
  packet[strlen(packet)] = (fcs>>8) & 0xff;

  // EOF byte
  packet[strlen(packet)] = FLAG;
}

// Calculate the checksum - CRC type.
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

// Shift the data out.
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
